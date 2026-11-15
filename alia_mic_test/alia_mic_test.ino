/*
 * alia_mic_test - Microphone check for the Alia Blinky "sound" PCB
 *
 * Based on alia_blinky.ino (v1.2.0). Same board auto-detect and pin map.
 *
 * PURPOSE: Answer one question - can the board hear sounds (clap, talking,
 * music)? Not hi-fi, just "is there a signal above the background".
 *
 * HOW TO USE:
 *   1. Upload this sketch (same board settings as alia_blinky.ino)
 *   2. Tools -> Serial Plotter, set baud to 115200
 *   3. Stay quiet ~5 s while it learns the noise floor, then clap / talk
 *
 * DETECTOR: STA/LTA (short-term average / long-term average)
 *   level  - raw loudness: RMS of the mic signal (DC removed) over a 20 ms window
 *   sta    - short-term average of level (~0.1 s) - follows syllables/claps
 *   lta    - long-term average of level (~5 s)    - the learned noise floor
 *   thresh - lta * TRIGGER_RATIO + MIN_MARGIN      - floats just above the floor
 *   Sound is "detected" when sta > thresh. While detected, the LTA is updated
 *   much more slowly so talking doesn't immediately raise the noise floor
 *   (but a permanent change, like a fan turning on, is still learned).
 *
 * FEEDBACK:
 *   - Nav lights (nose/port/starboard) turn on while sound is detected
 *   - Prop LEDs show how far sta is above the floor (green -> yellow -> red)
 *   - One dim tail LED = sketch is running
 * Set LED_METER to false to rule out LED power noise leaking into the mic.
 *
 * All other debug text is suppressed so the plotter only sees numbers.
 *
 * HARDWARE: MSM381ACP003 analog MEMS mic (U42), output -> A3 pad on the
 * QT Py / XIAO footprint (physical pin 4).
 */

#include <Adafruit_NeoPixel.h>
#include <math.h>

// ===== SETTINGS - EDIT ME =====
#define LED_METER true         // Show loudness on the prop LEDs
#define PLOT_LEVEL false        // Also plot the raw (unsmoothed) level - noisier trace
#define WINDOW_MS 20           // Loudness measurement window (ms) -> ~50 plot points/s
#define brightness 50          // LED brightness (0-255), same as alia_blinky

// STA/LTA detector
const float STA_TIME_MS = 100.0;        // Short-term average time constant
const float LTA_TIME_MS = 5000.0;       // Long-term average (noise floor) time constant
const float LTA_TIME_TRIGGERED_MS = 30000.0;  // Floor adapts this slowly while sound is detected
const float TRIGGER_RATIO = 1.2;        // Detect when sta > lta * this ... (lower = more sensitive)
const float MIN_MARGIN = 1.0;           // ... plus this many ADC counts RMS (stops chatter in very quiet rooms)
const float RELEASE_RATIO = 1.05;       // Detection ends when sta < lta * this + MIN_MARGIN (must be < TRIGGER_RATIO)
const float METER_FULL_RATIO = 3.0;     // sta/lta ratio that lights all 9 LEDs on each prop

// ===== PINS (same auto-detect as alia_blinky.ino) =====
#if defined(ARDUINO_ADAFRUIT_QTPY_ESP32S3_NOPSRAM) || defined(ARDUINO_ADAFRUIT_QTPY_ESP32S3)
  #define starPin 5
  #define portPin 16
  #define nosePin 36
  #define ledPin 37
#elif defined(ARDUINO_ADAFRUIT_QTPY_RP2040)
  #define starPin 20
  #define portPin 5
  #define nosePin 6
  #define ledPin 4
#elif defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  #define starPin 43
  #define portPin 44
  #define nosePin 8
  #define ledPin 7
#else  // Seeed XIAO RP2040
  #define starPin 0
  #define portPin 1
  #define nosePin 2
  #define ledPin 4
#endif

// The mic is wired to the A3 pad. Every supported board core defines A3
// as the correct GPIO for that pad, so we just use the name.
#define micPin A3

// ===== LEDS =====
#define LED_COUNT 41
#define PROP_LEDS 9
#define TAIL_START 36
Adafruit_NeoPixel strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800);

// ===== DETECTOR STATE =====
float sta = 0, lta = 0;
float staAlpha, ltaAlpha, ltaAlphaTriggered;  // Smoothing factors, computed in setup()
bool detected = false;
bool primed = false;  // false until the first window seeds the averages

// Converts a time constant into a per-window smoothing factor (exponential moving average)
float alphaFor(float timeMs) {
  return 1.0 - expf(-(float)WINDOW_MS / timeMs);
}

void setup() {
  pinMode(nosePin, OUTPUT);
  pinMode(portPin, OUTPUT);
  pinMode(starPin, OUTPUT);
  pinMode(micPin, INPUT);

  analogReadResolution(12);  // 0-4095 on both RP2040 and ESP32-S3

  Serial.begin(115200);
  // Short wait for USB serial; no text is printed so the plotter stays clean
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) { }

  strip.setBrightness(brightness);
  strip.begin();
  strip.clear();
  strip.show();

  staAlpha = alphaFor(STA_TIME_MS);
  ltaAlpha = alphaFor(LTA_TIME_MS);
  ltaAlphaTriggered = alphaFor(LTA_TIME_TRIGGERED_MS);
}

// Light the props as a bar graph based on how far sta is above the noise floor
void showMeter(float ratio) {
  int lit = 0;
  if (ratio > 1.0) {
    lit = (int)((ratio - 1.0) / (METER_FULL_RATIO - 1.0) * PROP_LEDS + 0.5);
    if (lit > PROP_LEDS) lit = PROP_LEDS;
  }
  strip.clear();
  for (int prop = 0; prop < 4; prop++) {
    for (int i = 0; i < lit; i++) {
      uint32_t c;
      if (i < 5)      c = strip.Color(0, 255, 0);    // green
      else if (i < 7) c = strip.Color(255, 160, 0);  // yellow
      else            c = strip.Color(255, 0, 0);    // red
      strip.setPixelColor(prop * PROP_LEDS + i, c);
    }
  }
  // Tail: dim white "alive" marker so you know the sketch is running
  strip.setPixelColor(TAIL_START + 2, strip.Color(60, 60, 60));
  strip.show();
}

void loop() {
  // ----- Sample the mic as fast as possible for WINDOW_MS -----
  // Accumulate sum and sum-of-squares so we can get RMS in one pass.
  // 64-bit sums: thousands of samples x 4095^2 overflows 32 bits.
  uint64_t sum = 0, sumSq = 0;
  uint32_t n = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < WINDOW_MS) {
    uint32_t s = analogRead(micPin);
    sum += s;
    sumSq += (uint64_t)s * s;
    n++;
  }
  // RMS of the AC part (DC bias removed): sqrt(mean(x^2) - mean(x)^2).
  // Averages every sample in the window, so single noise spikes matter far
  // less than with peak-to-peak.
  double mean = (double)sum / n;
  double variance = (double)sumSq / n - mean * mean;
  float level = variance > 0 ? sqrt(variance) : 0;  // RMS = loudness

  // ----- STA/LTA -----
  if (!primed) {
    sta = lta = level;  // Seed both averages with the first reading
    primed = true;
  }
  sta += staAlpha * (level - sta);
  // Learn the floor normally when quiet; much more slowly while sound is detected
  lta += (detected ? ltaAlphaTriggered : ltaAlpha) * (sta - lta);

  float thresh = lta * TRIGGER_RATIO + MIN_MARGIN;
  float release = lta * RELEASE_RATIO + MIN_MARGIN;
  if (!detected && sta > thresh) detected = true;
  else if (detected && sta < release) detected = false;

  // ----- Plot -----
  if (PLOT_LEVEL) {
    Serial.print("level:");
    Serial.print(level, 1);
    Serial.print(",");
  }
  Serial.print("sta:");
  Serial.print(sta, 1);
  Serial.print(",lta:");
  Serial.print(lta, 1);
  Serial.print(",thresh:");
  Serial.print(thresh, 1);
  Serial.print(",detect:");
  Serial.println(detected ? thresh * 1.2 : 0, 1);  // Step trace, drawn just above thresh when on

  // ----- Visual feedback -----
  digitalWrite(nosePin, detected);
  digitalWrite(portPin, detected);
  digitalWrite(starPin, detected);

  if (LED_METER) {
    showMeter(lta > 0 ? sta / lta : 0);
  } else {
    static bool cleared = false;
    if (!cleared) { strip.clear(); strip.show(); cleared = true; }
  }
}
