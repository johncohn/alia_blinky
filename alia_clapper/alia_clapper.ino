/*
 * alia_clapper - Clap / button controlled Alia LED patterns for the "sound" PCB
 *
 * Based on alia_blinky.ino (v1.2.0) and alia_mic_test.ino. Same board
 * auto-detect, pin map, and LED layout. Needs the sound PCB (MSM381ACP003
 * mic on the A3 pad).
 *
 * ========== MODES ==========
 *   NORMAL - (at power-up) the light patterns, each on a timer
 *   SOUND  - only the sound-reactive patterns, each on a timer, reacting to
 *            talking, music, etc.
 *
 * ========== CONTROLS ==========
 *   BOOT button ("B" on the XIAO) -> switch NORMAL <-> SOUND mode
 *                                    (also turns the lights on if they were off)
 *   Double clap                   -> lights OFF / ON (back on in the same mode)
 *
 *   The two claps must be less than CLAP_GROUP_MS apart. Single claps and
 *   3+ claps are ignored (the tail still flashes so you can see it heard them).
 *   Don't press the other button ("R") - that resets the board.
 *
 * FEEDBACK:
 *   - Every clap the board hears flashes the tail white
 *   - The lights always come on at power-up, in NORMAL mode
 *   - Entering SOUND mode flashes the props blue, NORMAL mode green
 *   - Serial Monitor (115200) prints every action. Typing + or - + Enter
 *     makes the sound effects more / less sensitive; v + Enter toggles a
 *     live readout of the mic levels. Typing 2 + Enter acts like a double
 *     clap, b + Enter like a button press (handy for testing).
 *
 * ========== PATTERNS ==========
 *   NORMAL mode:
 *   TAKE OFF     - parked on the ground, props spool up, then hover
 *   CTOL         - transition: props spin down to parked, tail fast (cruise)
 *   LANDING      - props spin up, hold, spin down, then rest on the ground
 *   SLOW RAINBOW - rainbow across the whole aircraft
 *   FAST WHITE   - running white sine wave
 *   RAINBOW PROPS- theater-chase rainbow
 *   PROP COMETS  - a comet with a fading tail circles each prop (counter-rotating)
 *   SPARKLE      - random twinkles that fade out
 *   SOUND mode:
 *   (every LED is always lit; sound changes the WHOLE pattern: brightness,
 *    speed, color and direction)
 *   SOUND SWIRL  - two-tone swirls turn in each prop; sound speeds them up and
 *                  each new sound flips their direction
 *   SOUND COLORS - the whole aircraft shimmers in one color; each new sound
 *                  jumps it to a new color
 *   SOUND SPARKLE- a flowing color wash with sparkles; sound = more sparkles
 *                  and a burst whenever a new sound starts
 *   SOUND RAINBOW- turning rainbow rings; sound spins them faster
 *   In SOUND mode the overall brightness also follows the sound: dim when
 *   quiet, bright when loud.
 *
 * Each pattern runs for its time in patternMs[], then the next one in the
 * same mode starts.
 *
 * All patterns are non-blocking: the mic is checked every ~20 ms, so claps
 * are heard whatever is running.
 */

#include <Adafruit_NeoPixel.h>
#include <math.h>

// ===== SETTINGS - EDIT ME =====
#define brightness 50           // LED brightness (0-255), same as alia_blinky
// SOUND mode: overall brightness follows the sound, from very dim when
// quiet to bright when loud. Lower SOUND_MAX_BRIGHTNESS on weak USB power
// (USB-A) or if loud moments cause resets.
#define SOUND_MIN_BRIGHTNESS 15
#define SOUND_MAX_BRIGHTNESS 110

// Clap detection (see alia_mic_test.ino for the detector; tune it there)
const float CLAP_RATIO = 5.0;          // A clap's raw level must exceed noise floor x this ...
const float CLAP_JUMP = 2.0;           // ... AND the recent average x this (ignores steady talk/music)
const unsigned long CLAP_REFRACTORY_MS = 150;  // Ignore echoes right after a clap
const unsigned long CLAP_GROUP_MS = 600;       // Max gap between claps in a double/triple clap

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
#define micPin A3               // Mic output on the A3 pad

// ===== LED LAYOUT (same as alia_blinky.ino) =====
#define LED_COUNT 41
#define PROP1_START 0           // Front left,  CCW
#define PROP2_START 9           // Front right, CW
#define PROP3_START 18          // Rear left,   CW
#define PROP4_START 27          // Rear right,  CCW
#define TAIL_START 36           // Tail: LEDs 36-40
#define TAIL_END 40
#define TAIL_LEDS 5

Adafruit_NeoPixel strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800);
uint32_t normColor;             // White used by the flight patterns (same as alia_blinky)

// ===== PATTERN LIST =====
#define NUM_PATTERNS 12
const char* patternNames[NUM_PATTERNS] = {
  "TAKE OFF", "CTOL", "LANDING", "SLOW RAINBOW", "FAST WHITE",
  "RAINBOW PROPS", "PROP COMETS", "SPARKLE",
  "SOUND SWIRL", "SOUND COLORS", "SOUND SPARKLE", "SOUND RAINBOW"
};
// How long each pattern runs before the next one starts (ms)
const unsigned long patternMs[NUM_PATTERNS] = {
  12000, 12000, 18000, 10000, 10000,
  10000, 10000, 10000,
  15000, 15000, 15000, 15000
};
// Which patterns belong to each mode (first index, how many)
#define NORMAL_FIRST 0
#define NORMAL_COUNT 8
#define SOUND_FIRST 8
#define SOUND_COUNT 4

// ===== STATE =====
int currentPattern = 0;
unsigned long patternStart = 0;
bool patternStarting = true;    // true on the first frame of a pattern
bool lightsOn = true;
bool soundMode = false;         // false = NORMAL patterns, true = SOUND patterns
int lastNormalPattern = NORMAL_FIRST;  // Where to resume each mode
int lastSoundPattern = SOUND_FIRST;
unsigned long tailFlashUntil = 0;   // Clap heard
unsigned long modeFlashUntil = 0;   // Mode changed
uint32_t modeFlashColor = 0;

// Nav light modes
#define NAV_OFF 0
#define NAV_ON 1
#define NAV_BLINK 2
#define NAV_SOUND 3
int navMode = NAV_ON;

// ============================================================================
// ===== MICROPHONE: STA/LTA loudness + clap detection =====
// ============================================================================
#define MIC_WINDOW_MS 20             // One frame = one mic window (~45 frames/s)
const float MIC_STA_MS = 100.0;      // Short-term average (follows syllables / claps)
const float MIC_LTA_MS = 5000.0;     // Long-term average = learned noise floor
const float MIC_LTA_TRIGGERED_MS = 30000.0;  // Floor learns this slowly while sound is detected
const float MIC_LEARN_LTA_MS = 250.0;        // Fast floor learning after a change
const float MIC_TRIGGER_RATIO = 1.2; // Sound detected when sta > floor x this + margin
const float MIC_RELEASE_RATIO = 1.05;
const float MIC_MIN_MARGIN = 1.0;    // ADC counts RMS

// Loudness for the sound EFFECTS (separate from clap detection, which
// stays strict). The effects compare against a QUIET-ROOM floor that drops
// quickly but rises only very slowly (QUIET_FLOOR_RISE_MS), so steady
// sound like music keeps counting as sound instead of becoming the new
// "background" the way it does for the clap detector.
// Effects start reacting at LOUD_START_RATIO x the quiet floor and are full
// at loudFullRatio x. Lower loudFullRatio = more sensitive. Adjust live by
// typing + or - in the Serial Monitor, then copy the printed value here.
const float LOUD_START_RATIO = 1.03; // Effects start reacting here
float loudFullRatio = 1.4;           // ... and are at full here
const float QUIET_FLOOR_RISE_MS = 120000.0;  // Floor creeps up this slowly
// Small fixed margin (ADC counts RMS) so the effects don't react to the
// ADC's own noise. The mic signal is tiny (quiet room ~0.7 counts RMS), so
// keep this well under 1.
const float EFFECT_MARGIN = 0.15;
bool showLevels = false;             // "v" in the Serial Monitor toggles a live readout
float quietFloor = 0;

float micSta = 0, micLta = 0;
bool micDetected = false;
float micLoudness = 0;               // 0.0 (at threshold or below) .. 1.0 (very loud)
bool micClap = false;                // true for exactly one frame per clap
bool micPrimed = false;
unsigned long micLearnUntil = 0;

float micAlpha(float timeMs) {
  return 1.0 - exp(-(float)MIC_WINDOW_MS / timeMs);
}

// Re-learn the noise floor quickly and ignore claps for a moment. Called on
// every pattern / lights change, since the LEDs' power draw shifts the
// mic's background noise.
void micRelearn(unsigned long ms) {
  micLearnUntil = millis() + ms;
}

// Listens for MIC_WINDOW_MS and updates all the mic variables
void micUpdate() {
  uint64_t sum = 0, sumSq = 0;
  uint32_t n = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < MIC_WINDOW_MS) {
    uint32_t s = analogRead(micPin);
    sum += s;
    sumSq += (uint64_t)s * s;
    n++;
  }
  double mean = (double)sum / n;
  double variance = (double)sumSq / n - mean * mean;
  float level = variance > 0 ? sqrt(variance) : 0;  // RMS, DC removed

  if (!micPrimed) {
    micSta = micLta = quietFloor = level;
    micPrimed = true;
    micRelearn(1000);
  }
  bool learning = (long)(millis() - micLearnUntil) < 0;

  // Clap = sharp jump above both the floor and the recent average.
  // Checked before sta absorbs this window.
  static unsigned long lastClap = 0;
  static bool clapArmed = true;      // Level must drop back between claps
  float clapLevel = micLta * CLAP_RATIO + MIC_MIN_MARGIN;
  micClap = false;
  if (!learning && clapArmed && level > clapLevel && level > micSta * CLAP_JUMP &&
      millis() - lastClap > CLAP_REFRACTORY_MS) {
    micClap = true;
    lastClap = millis();
    clapArmed = false;
  }
  if (level < clapLevel * 0.5) clapArmed = true;

  micSta += micAlpha(MIC_STA_MS) * (level - micSta);
  float ltaTime = learning ? MIC_LEARN_LTA_MS : (micDetected ? MIC_LTA_TRIGGERED_MS : MIC_LTA_MS);
  micLta += micAlpha(ltaTime) * (micSta - micLta);

  float thresh = micLta * MIC_TRIGGER_RATIO + MIC_MIN_MARGIN;
  float release = micLta * MIC_RELEASE_RATIO + MIC_MIN_MARGIN;
  if (learning) micDetected = false;
  else if (!micDetected && micSta > thresh) micDetected = true;
  else if (micDetected && micSta < release) micDetected = false;

  // Quiet-room floor: follows quiet moments down quickly, creeps up slowly
  if (micSta < quietFloor) quietFloor += 0.2 * (micSta - quietFloor);
  else quietFloor += micAlpha(QUIET_FLOOR_RISE_MS) * (micSta - quietFloor);

  // Effects loudness: 0 just above the quiet floor .. 1 at loudFullRatio x,
  // then a square-root curve so soft sounds already give a clear response
  float loudStart = quietFloor * LOUD_START_RATIO + EFFECT_MARGIN;
  float loudFull = quietFloor * loudFullRatio + EFFECT_MARGIN * 2;
  float x = learning ? 0 : (micSta - loudStart) / (loudFull - loudStart);
  if (x < 0) x = 0;
  if (x > 1) x = 1;
  micLoudness = sqrt(x);
}

// ============================================================================
// ===== HELPERS =====
// ============================================================================
void updateNavLights() {
  bool on;
  switch (navMode) {
    case NAV_ON:    on = true; break;
    case NAV_BLINK: on = (millis() / 1500) % 2 == 0; break;
    case NAV_SOUND: on = micDetected; break;
    default:        on = false; break;
  }
  digitalWrite(nosePin, on);
  digitalWrite(portPin, on);
  digitalWrite(starPin, on);
}

// LED for ring position pos (any int, wraps) on prop p (0-3), stepping in
// that prop's spin direction: props 1 & 4 CCW, 2 & 3 CW (same as FLIGHT)
int propLed(int p, int pos) {
  pos = ((pos % 9) + 9) % 9;
  if (p == 0 || p == 3) pos = (9 - pos) % 9;
  return p * 9 + pos;
}

// Dim a pixel by fadeValue/256 (used by SPARKLE)
void fadePixel(int i, uint8_t fadeValue) {
  uint32_t c = strip.getPixelColor(i);
  uint8_t r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff;
  r = (r <= 10) ? 0 : r - (r * fadeValue / 256);
  g = (g <= 10) ? 0 : g - (g * fadeValue / 256);
  b = (b <= 10) ? 0 : b - (b * fadeValue / 256);
  strip.setPixelColor(i, r, g, b);
}

// ============================================================================
// ===== FLIGHT PATTERNS (TAKE OFF / CTOL / LANDING) =====
// ============================================================================
// Same look and speed curve as alia_blinky's FLIGHT pattern, split into
// three patterns that each end in a steady state.
const int minPropDelay = 10;    // ms between prop steps at full speed
const int maxPropDelay = 500;   // ms between prop steps when slowest (= parked)

int propStep = 0;
unsigned long lastPropStep = 0, lastTailStep = 0;
uint8_t tailPos = 0;

// progress 0 = slow (parked), 1 = full speed. Squared curve like FLIGHT.
int propDelayFor(float progress) {
  if (progress < 0) progress = 0;
  if (progress > 1) progress = 1;
  return maxPropDelay - (progress * progress * (maxPropDelay - minPropDelay));
}

// Advance props/tail by however many steps the elapsed time allows, then draw
void drawFlight(int propDelay, int tailDelay) {
  unsigned long now = millis();
  bool spinning = propDelay < maxPropDelay;

  if (spinning) {
    if (now - lastPropStep > 1000) lastPropStep = now;  // No backlog
    while (now - lastPropStep >= (unsigned long)propDelay) {
      lastPropStep += propDelay;
      propStep = (propStep + 1) % 9;
    }
  } else {
    lastPropStep = now;
  }
  if (now - lastTailStep >= (unsigned long)tailDelay) {
    lastTailStep = now;
    tailPos = (tailPos + 1) % TAIL_LEDS;
  }

  strip.clear();
  if (spinning) {
    for (int p = 0; p < 4; p++) {   // 2-blade props, counter-rotating
      strip.setPixelColor(propLed(p, propStep), normColor);
      strip.setPixelColor(propLed(p, propStep + 4), normColor);
    }
  } else {
    // Parked blade positions, same as FLIGHT
    strip.setPixelColor(PROP1_START + 0, normColor);
    strip.setPixelColor(PROP1_START + 4, normColor);
    strip.setPixelColor(PROP2_START + 8, normColor);
    strip.setPixelColor(PROP2_START + 4, normColor);
    strip.setPixelColor(PROP3_START + 0, normColor);
    strip.setPixelColor(PROP3_START + 4, normColor);
    strip.setPixelColor(PROP4_START + 8, normColor);
    strip.setPixelColor(PROP4_START + 4, normColor);
  }
  strip.setPixelColor(TAIL_START + tailPos, normColor);
  navMode = NAV_ON;  // Constant nav lights, like FLIGHT
}

// TAKE OFF: 2 s parked on the ground, 5 s spool up, then hover (full speed)
void takeOffPattern(unsigned long t) {
  int propDelay;
  if (t < 2000)      propDelay = maxPropDelay;
  else if (t < 7000) propDelay = propDelayFor((t - 2000) / 5000.0);
  else               propDelay = minPropDelay;
  drawFlight(propDelay, 400);  // Tail very slow during lift
}

// CTOL: 5 s spin down from hover to parked with tail speeding up, then cruise
void ctolPattern(unsigned long t) {
  if (t < 5000) drawFlight(propDelayFor(1.0 - t / 5000.0), 50);
  else          drawFlight(maxPropDelay, 30);  // Props parked, tail fast
}

// LANDING: 5 s spin up, 3 s hold, 5 s spin down (tail slowing), then parked
void landingPattern(unsigned long t) {
  if (t < 5000)       drawFlight(propDelayFor(t / 5000.0), 50 + (int)(t * 150 / 13000));
  else if (t < 8000)  drawFlight(minPropDelay, 50 + (int)(t * 150 / 13000));
  else if (t < 13000) drawFlight(propDelayFor(1.0 - (t - 8000) / 5000.0), 50 + (int)(t * 150 / 13000));
  else                drawFlight(maxPropDelay, 400);  // On the ground
}

// ============================================================================
// ===== COLOR PATTERNS (non-blocking versions of alia_blinky's) =====
// ============================================================================

// SLOW RAINBOW: rainbow across the whole aircraft, ~0.4 turns of the color wheel per second
void slowRainbowPattern(unsigned long t) {
  strip.rainbow((uint16_t)(t * 25));
  navMode = NAV_BLINK;
}

// FAST WHITE: white sine wave running along the string, one step per 50 ms
void fastWhitePattern(unsigned long t) {
  int position = t / 50;
  for (int i = 0; i < LED_COUNT; i++) {
    uint8_t level = (sin(i + position) * 127 + 128);
    strip.setPixelColor(i, level, level, level);
  }
  navMode = NAV_BLINK;
}

// RAINBOW PROPS: theater-chase rainbow, one frame per 50 ms
void rainbowPropsPattern(unsigned long t) {
  unsigned long frame = t / 50;
  int b = frame % 3;
  uint16_t firstHue = frame * (65536 / 90);
  strip.clear();
  for (int c = b; c < LED_COUNT; c += 3) {
    uint16_t hue = firstHue + c * 65536L / LED_COUNT;
    strip.setPixelColor(c, strip.gamma32(strip.ColorHSV(hue)));
  }
  navMode = NAV_BLINK;
}

// PROP COMETS: a comet with a fading tail circles each prop, one step per 60 ms
void propCometsPattern(unsigned long t) {
  unsigned long frame = t / 60;
  int head = frame % 9;
  uint16_t hue = frame * 300;  // Colors slowly drift
  strip.clear();
  for (int p = 0; p < 4; p++) {
    uint16_t propHue = hue + p * 16384;  // Each prop a quarter of the wheel apart
    for (int k = 0; k < 5; k++) {        // Head + 4 fading tail LEDs
      uint8_t v = 255 >> k;
      strip.setPixelColor(propLed(p, head - k), strip.gamma32(strip.ColorHSV(propHue, 255, v)));
    }
  }
  strip.setPixelColor(TAIL_START + frame % TAIL_LEDS, normColor);
  navMode = NAV_BLINK;
}

// SPARKLE: random twinkles that fade out
void sparklePattern(unsigned long t, bool start) {
  if (start) strip.clear();
  for (int i = 0; i < LED_COUNT; i++) fadePixel(i, 40);
  for (int n = 0; n < 2; n++) {
    if (random(100) < 60) {
      uint32_t c = (random(4) == 0) ? strip.gamma32(strip.ColorHSV(random(65536)))
                                    : strip.Color(255, 255, 255);
      strip.setPixelColor(random(LED_COUNT), c);
    }
  }
  navMode = NAV_BLINK;
}

// ============================================================================
// ===== SOUND PATTERNS =====
// ============================================================================

// Design rule for the sound patterns: EVERY LED is always lit with strong
// color and the pattern is always moving. Sound changes the whole pattern
// at once:
//   - loudness  -> overall brightness (loop() ramps it from
//                  SOUND_MIN_BRIGHTNESS to SOUND_MAX_BRIGHTNESS) and speed
//   - new sound -> a "beat": jump to a new color or flip direction

// Slowly drifting base hue shared by the sound patterns (one turn per ~30 s)
uint16_t driftHue() {
  return (uint16_t)(millis() * 2);
}

// Loudness with a fast attack and a short fade, so sound "flashes" up
// and then settles back instead of flickering. 0 quiet .. 1 loud.
float glowLevel = 0;
// true for one frame when a new sound starts (talking starts, a beat hits)
bool soundBeat = false;

// Called once per frame from loop()
void updateSoundFeatures() {
  static float slowLoudness = 0;
  static unsigned long lastBeat = 0;

  if (micLoudness > glowLevel) glowLevel = micLoudness;
  else glowLevel *= 0.90;

  // Beat = loudness jumps well above its recent average
  soundBeat = micLoudness > slowLoudness + 0.2 && millis() - lastBeat > 250;
  if (soundBeat) lastBeat = millis();
  slowLoudness += 0.1 * (micLoudness - slowLoudness);
}

// Overall LED brightness for SOUND mode
uint8_t soundBrightness() {
  return SOUND_MIN_BRIGHTNESS + glowLevel * (SOUND_MAX_BRIGHTNESS - SOUND_MIN_BRIGHTNESS);
}

// SOUND SWIRL: each prop ring is a two-tone swirl (a band of color with a
// brightness wave through it) that keeps turning. Sound speeds it up, and
// every new sound flips the direction. Colors drift slowly.
void soundSwirlPattern(bool start) {
  static float angle = 0;
  static int dir = 1;
  if (start) dir = 1;

  if (soundBeat) dir = -dir;              // New sound: reverse!
  angle += dir * (0.06 + glowLevel * 0.7); // LEDs per frame: calm .. fast
  while (angle >= 9) angle -= 9;
  while (angle < 0) angle += 9;

  uint16_t hue = driftHue();
  for (int p = 0; p < 4; p++) {
    for (int i = 0; i < 9; i++) {
      float pos = i + angle;                         // Position in the swirl
      float wave = 0.5 + 0.5 * sin(pos * 2 * PI / 9); // 0..1 around the ring
      uint16_t h = hue + p * 8192 + (uint16_t)(wave * 21845);  // 2 tones, a third of the wheel apart
      uint8_t v = 150 + wave * 105;                   // Every LED lit, brighter on the crest
      strip.setPixelColor(propLed(p, i), strip.gamma32(strip.ColorHSV(h, 255, v)));
    }
  }
  for (int i = 0; i < TAIL_LEDS; i++) {
    float wave = 0.5 + 0.5 * sin((i + angle) * 2 * PI / TAIL_LEDS);
    strip.setPixelColor(TAIL_START + i, strip.gamma32(strip.ColorHSV(hue + (uint16_t)(wave * 21845), 255, 150 + wave * 105)));
  }
  navMode = NAV_SOUND;  // Nav lights = sound detected
}

// SOUND COLORS: the whole aircraft is one color with a gentle shimmer moving
// through it. Every new sound jumps to a new color (with a quick blend);
// louder makes the shimmer move faster.
void soundColorsPattern(bool start) {
  static float hue = 0, targetHue = 0;
  static float phase = 0;
  if (start) hue = targetHue = driftHue();

  if (soundBeat) targetHue += 16384 + random(32768);  // Jump a quarter to 3/4 of the wheel
  hue += (targetHue - hue) * 0.15;                    // Quick blend to the new color
  targetHue += 3;                                     // Slow drift between beats
  hue += 3;
  if (hue > 65536 && targetHue > 65536) { hue -= 65536; targetHue -= 65536; }

  phase += 0.08 + glowLevel * 0.5;                    // Shimmer speed
  for (int i = 0; i < LED_COUNT; i++) {
    float shimmer = sin(phase + i * 0.5);
    uint16_t h = (uint16_t)(uint32_t)hue + (int16_t)(shimmer * 3000);  // Small hue ripple
    uint8_t v = 205 + shimmer * 50;                          // and brightness ripple
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(h, 255, v)));
  }
  navMode = NAV_SOUND;
}

// SOUND SPARKLE: a flowing color wash over the whole aircraft with sparkles
// twinkling on top. Sound = more sparkles; each new sound throws out a burst.
void soundSparklePattern(bool start) {
  static float spark[LED_COUNT];       // Sparkle brightness 0..1, fades each frame
  static uint16_t sparkHue[LED_COUNT];
  if (start) {
    for (int i = 0; i < LED_COUNT; i++) spark[i] = 0;
  }

  for (int i = 0; i < LED_COUNT; i++) spark[i] *= 0.85;

  // New sparkles this frame: louder = more (fractional part = odds)
  float want = 0.3 + micLoudness * 5.0;
  if (soundBeat) want += 15;           // New sound: burst!
  while (want > 0) {
    if (want >= 1 || random(1000) < want * 1000) {
      int i = random(LED_COUNT);
      spark[i] = 1.0;
      sparkHue[i] = random(65536);
    }
    want -= 1;
  }

  uint16_t base = driftHue() * 2;
  for (int i = 0; i < LED_COUNT; i++) {
    if (spark[i] > 0.3) {
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(sparkHue[i], 120, 255)));  // Pale, bright
    } else {
      uint16_t h = base + i * 1600;    // Wash: a slow color wave along the string
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(h, 255, 180)));
    }
  }
  navMode = NAV_SOUND;
}

// SOUND RAINBOW: each prop ring (and the tail) shows a full turning rainbow.
// Sound spins it faster (and the overall brightness rises with it).
void soundRainbowPattern(bool start) {
  static float hue = 0;
  (void)start;

  hue += 120 + glowLevel * 2500;      // Always turning; much faster with sound
  while (hue >= 65536) hue -= 65536;

  for (int p = 0; p < 4; p++) {
    for (int i = 0; i < 9; i++) {
      uint16_t h = (uint16_t)hue + i * (65536 / 9) + p * 4096;
      strip.setPixelColor(propLed(p, i), strip.gamma32(strip.ColorHSV(h)));
    }
  }
  for (int i = 0; i < TAIL_LEDS; i++) {
    uint16_t h = (uint16_t)hue + i * (65536 / TAIL_LEDS);
    strip.setPixelColor(TAIL_START + i, strip.gamma32(strip.ColorHSV(h)));
  }
  navMode = NAV_SOUND;
}

// ============================================================================
// ===== PATTERN DISPATCH AND CLAP ACTIONS =====
// ============================================================================
void runPattern(int i, unsigned long t, bool start) {
  switch (i) {
    case 0: takeOffPattern(t); break;
    case 1: ctolPattern(t); break;
    case 2: landingPattern(t); break;
    case 3: slowRainbowPattern(t); break;
    case 4: fastWhitePattern(t); break;
    case 5: rainbowPropsPattern(t); break;
    case 6: propCometsPattern(t); break;
    case 7: sparklePattern(t, start); break;
    case 8: soundSwirlPattern(start); break;
    case 9: soundColorsPattern(start); break;
    case 10: soundSparklePattern(start); break;
    case 11: soundRainbowPattern(start); break;
  }
}

void startPattern(int i) {
  currentPattern = i;
  if (i >= SOUND_FIRST) lastSoundPattern = i;
  else lastNormalPattern = i;
  patternStart = millis();
  patternStarting = true;
  micRelearn(500);
  Serial.print("-> pattern: ");
  Serial.println(patternNames[i]);
}

// Next pattern, wrapping around within the current mode's patterns
void nextPattern() {
  int first = soundMode ? SOUND_FIRST : NORMAL_FIRST;
  int count = soundMode ? SOUND_COUNT : NORMAL_COUNT;
  startPattern(first + (currentPattern - first + 1) % count);
}

// Turn the lights on in NORMAL (sound = false) or SOUND (sound = true) mode,
// resuming where that mode left off. Flashes green (NORMAL) or blue (SOUND).
void lightsOnInMode(bool sound) {
  lightsOn = true;
  soundMode = sound;
  modeFlashColor = sound ? strip.Color(0, 0, 255) : strip.Color(0, 255, 0);
  modeFlashUntil = millis() + 600;
  Serial.println(sound ? "-> SOUND mode (reacts to talking, music...)" : "-> NORMAL mode");
  startPattern(sound ? lastSoundPattern : lastNormalPattern);
}

void lightsOff() {
  lightsOn = false;
  micRelearn(500);
  Serial.println("-> lights OFF (double clap or BOOT button to turn on)");
}

// BOOT button: flip between NORMAL and SOUND (and make sure the lights are on)
void handleButton() {
  Serial.println("BOOT button");
  lightsOnInMode(!soundMode);
}

// Is the BOOT button held down?
bool bootButtonPressed() {
#if defined(ARDUINO_ARCH_RP2040)
  return BOOTSEL;              // RP2040: read via the core's BOOTSEL helper
#else
  return digitalRead(0) == LOW;  // ESP32-S3 boards: BOOT button on GPIO 0
#endif
}

// Act on a finished group of claps
void handleClaps(int count) {
  Serial.print("heard ");
  Serial.print(count);
  Serial.println(count == 1 ? " clap" : " claps");

  if (count == 2) {
    if (lightsOn) lightsOff();
    else lightsOnInMode(soundMode);      // Back on in the same mode
  } else {
    Serial.println("-> ignored (double clap = lights off/on)");
  }
}

// ============================================================================
// ===== SETUP AND LOOP =====
// ============================================================================
void setup() {
  pinMode(nosePin, OUTPUT);
  pinMode(portPin, OUTPUT);
  pinMode(starPin, OUTPUT);
  pinMode(micPin, INPUT);
  analogReadResolution(12);  // 0-4095 on both RP2040 and ESP32-S3
#if !defined(ARDUINO_ARCH_RP2040)
  pinMode(0, INPUT_PULLUP);  // ESP32-S3 BOOT button
#endif

  strip.setBrightness(brightness);
  strip.begin();
  strip.clear();
  strip.show();
  normColor = strip.Color(brightness, brightness, brightness);

  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) { }  // Don't hold up battery power for long

  Serial.println("========================================");
  Serial.println("  alia_clapper - clap-controlled Alia LEDs");
  Serial.println("  BOOT button = NORMAL / SOUND mode | double clap = lights off/on");
  Serial.println("  (type 2 = fake double clap, b = fake button, + / - = sensitivity,");
  Serial.println("   v = live mic levels)");
  Serial.println("========================================");
  Serial.println("-> NORMAL mode");
  startPattern(NORMAL_FIRST);
}

void loop() {
  // 1. Listen (~20 ms) - this also sets the frame rate
  micUpdate();
  updateSoundFeatures();

  // 2. Count claps; act once the group is finished
  static int clapCount = 0;
  static unsigned long lastClapTime = 0;
  if (micClap) {
    clapCount++;
    lastClapTime = millis();
    tailFlashUntil = millis() + 120;
  }
  if (clapCount > 0 && millis() - lastClapTime > CLAP_GROUP_MS) {
    handleClaps(clapCount);
    clapCount = 0;
  }

  // BOOT button: act on the press (checked every frame, ~20 ms = debounce)
  static bool buttonWasDown = false;
  static unsigned long lastButton = 0;
  bool buttonDown = bootButtonPressed();
  if (buttonDown && !buttonWasDown && millis() - lastButton > 300) {
    lastButton = millis();
    handleButton();
  }
  buttonWasDown = buttonDown;

  // Serial: 2 = fake double clap, b = fake button press
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line == "1" || line == "2" || line == "3") handleClaps(line.toInt());
    else if (line == "b") handleButton();
    else if (line == "v") {
      showLevels = !showLevels;
      Serial.println(showLevels ? "-> live levels ON (v again to stop)" : "-> live levels OFF");
    }
    else if (line == "+" || line == "-") {
      // Sensitivity: shrink / grow the range above the floor to reach "full"
      float range = (loudFullRatio - 1.0) * (line == "+" ? 0.8 : 1.25);
      if (range < 0.05) range = 0.05;
      if (range > 5.0) range = 5.0;
      loudFullRatio = 1.0 + range;
      Serial.print("-> sound effects ");
      Serial.print(line == "+" ? "MORE" : "LESS");
      Serial.print(" sensitive: loudFullRatio = ");
      Serial.print(loudFullRatio, 2);
      Serial.print("  (quiet floor ");
      Serial.print(quietFloor, 1);
      Serial.print(", now ");
      Serial.print(micSta, 1);
      Serial.println(")");
    }
  }

  // Live readout: quiet floor, current level, and effect loudness (0-1)
  if (showLevels) {
    static unsigned long lastShow = 0;
    if (millis() - lastShow >= 250) {
      lastShow = millis();
      Serial.print("floor ");
      Serial.print(quietFloor, 2);
      Serial.print("  level ");
      Serial.print(micSta, 2);
      Serial.print("  x");
      Serial.print(quietFloor > 0 ? micSta / quietFloor : 0, 2);
      Serial.print("  loudness ");
      Serial.println(micLoudness, 2);
    }
  }

  // 3. Move on when the pattern's time is up
  if (lightsOn && millis() - patternStart >= patternMs[currentPattern]) {
    nextPattern();
  }

  // 4. Draw (SOUND mode: overall brightness follows the sound)
  if (lightsOn) {
    strip.setBrightness(soundMode ? soundBrightness() : brightness);
    runPattern(currentPattern, millis() - patternStart, patternStarting);
    patternStarting = false;

    if ((long)(modeFlashUntil - millis()) > 0) {  // Mode change: flash the props
      for (int i = 0; i < TAIL_START; i++) strip.setPixelColor(i, modeFlashColor);
    }
  } else {
    strip.clear();
    navMode = NAV_OFF;
  }
  if ((long)(tailFlashUntil - millis()) > 0) {    // Clap heard: flash the tail
    for (int i = TAIL_START; i <= TAIL_END; i++) strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  updateNavLights();
}
