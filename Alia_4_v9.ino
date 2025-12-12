/*
 * Alia_4 - Beta Alia eVTOL LED Animation Controller
 *
 * VERSION: 1.0.1
 * BUILD: 002
 * DATE: 2024-12-06
 *
 * Hardware: Seeed XIAO RP2040
 * LED Controller: WS2812B (41 LEDs)
 *   - 4 Lift Props (9 LEDs each): LEDs 0-35
 *   - Tail Prop (5 LEDs): LEDs 36-40
 *
 * Features:
 *   - FLIGHT: Simulates complete eVTOL flight sequence (lift, transition, cruise, landing)
 *   - SLOW RAINBOW: Smooth rainbow color cycle
 *   - FAST WHITE: Fast white running lights
 *   - RAINBOW PROPS: Rainbow-colored theater chase
 *   - XMAS: Christmas-themed patterns (auto-cycle)
 *
 * Pin Assignments (Seeed XIAO RP2040):
 *   - GPIO 1: Port navigation light
 *   - GPIO 2: Nose navigation light
 *   - GPIO 4: WS2812B data line
 *   - GPIO 0: Starboard navigation light
 *   - GPIO 21: Boot button (mode switching)
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// RP2040 BOOTSEL button support
#ifdef ARDUINO_ARCH_RP2040
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#endif

// ===== VERSION INFO =====
#define VERSION "1.0.3"
#define BUILD_NUMBER 14

// ===== LED CONFIGURATION =====
#define brightness 50  // Restored to original brightness
#define fastProp 2
#define slowProp 100
#define normProp 50
#define propSkip 2

// Seeed XIAO RP2040 Pin Definitions (Physical pin -> GPIO mapping)
// Testing different pin combinations
#define portPin 1    // Try GPIO 28 (D8/A2)
#define nosePin 2    // Try GPIO 27 (D9/A3)
#define ledPin 4      // Physical pin 10 - NeoPixel data pin (WORKING)
#define starPin  0  // Try GPIO 29 (D7/A1)
// Button configuration - BOOTSEL causes crashes, disabled for now
// #define USE_BOOTSEL_BUTTON  // Disabled - causes system crash
// Auto-cycle mode works great without button
#define buttonPin 3  // Reserved for future external button
#define waitTime 50

int blinkTime = 1500;
bool blinkState = true;
int mode = 0;
int maxMode = 7;  // Increased to accommodate new Xmas mode
bool gotBreak = false;

// Auto-cycle mode variables
unsigned long lastModeChange = 0;
int autoCycleSubMode = 0;
bool patternComplete = false;  // Flag to signal pattern completion
const int AUTO_CYCLE_DURATION = 10000; // Default duration for simple patterns

// Xmas pattern mode variables
unsigned long lastXmasModeChange = 0;
int xmasSubMode = 0;
const int XMAS_CYCLE_DURATION = 10000; // 10 seconds per Xmas pattern
uint8_t gHue = 0; // rotating "base color" used by patterns

// Button debounce variables
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 250;  // Increased for better debouncing

uint32_t normColor, fastColor, slowColor;

uint32_t fl_color, fr_color, bl_color, br_color, t_color;

unsigned long int now = 0;
unsigned long int p_now = 0;

static uint8_t fl_pos = 0;
static uint8_t fr_pos = 0;
static uint8_t bl_pos = 0;
static uint8_t br_pos = 0;
static uint8_t t_pos = 0;
int fl_speed = normProp;
int fr_speed = normProp;
int bl_speed = normProp;
int br_speed = normProp;
int t_speed = normProp;

static unsigned long fl_lastUpdate = 0;
static unsigned long fr_lastUpdate = 0;
static unsigned long bl_lastUpdate = 0;
static unsigned long br_lastUpdate = 0;
static unsigned long t_lastUpdate = 0;
static uint16_t fl_currentLed = 0;
static uint16_t fr_currentLed = 0;
static uint16_t bl_currentLed = 0;
static uint16_t br_currentLed = 0;
static uint16_t t_currentLed = 0;


#define LED_COUNT 41

// LED segment definitions
#define PROP1_START 0
#define PROP1_END 8
#define PROP2_START 9
#define PROP2_END 17
#define PROP3_START 18
#define PROP3_END 26
#define PROP4_START 27
#define PROP4_END 35
#define TAIL_START 36
#define TAIL_END 40

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800);

// Helper function to read BOOTSEL button
bool readButton() {
#ifdef USE_BOOTSEL_BUTTON
  #ifdef ARDUINO_ARCH_RP2040
    // Read BOOTSEL button using Pico SDK hardware registers
    const uint CS_PIN_INDEX = 1;

    // Disable interrupts while we check
    uint32_t flags = save_and_disable_interrupts();

    // Set CS pin to be a GPIO input (temporarily)
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // Read the button state (active low)
    bool button_pressed = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

    // Restore CS pin to normal operation
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return button_pressed ? LOW : HIGH;  // Return LOW when pressed
  #else
    return HIGH;  // Not on RP2040
  #endif
#else
  return HIGH;  // No button configured
#endif
}

void checkButton() {
  bool buttonReading = readButton();

  // Debug button state
  static unsigned long lastButtonDebug = 0;
  if (millis() - lastButtonDebug > 5000) {
    lastButtonDebug = millis();
    Serial.print("Button: reading=");
    Serial.print(buttonReading);
    Serial.print(" lastState=");
    Serial.println(lastButtonState);
  }

  // Button pressed (LOW) and enough time has passed since last press
  if (buttonReading == LOW && lastButtonState == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();

    mode++;
    if (mode >= maxMode) {
      mode = 0;
    }
    Serial.print("===== MODE CHANGED TO: ");
    Serial.print(mode);
    Serial.println(" =====");

    // Reset auto-cycle variables when entering mode 0
    if (mode == 0) {
      autoCycleSubMode = 0;
      lastModeChange = millis();
    }

    // Reset Xmas mode variables when entering mode 1
    if (mode == 1) {
      xmasSubMode = 0;
      lastXmasModeChange = millis();
    }

    gotBreak = true;
    strip.clear();
  }

  lastButtonState = buttonReading;
}

void setup() {
  pinMode(nosePin, OUTPUT);
  pinMode(portPin, OUTPUT);
  pinMode(starPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  // BOOTSEL button doesn't need pinMode setup
  Wire.begin();
  Wire.setClock(400000);  //400khz clock

  Serial.begin(115200);
  strip.setBrightness(brightness);
  strip.begin();

  // Print version info
  Serial.println("========================================");
  Serial.println("  Alia_4 - Beta Alia eVTOL LED Controller");
  Serial.print("  Version: ");
  Serial.println(VERSION);
  Serial.print("  Build: ");
  Serial.println(BUILD_NUMBER);
  Serial.println("  Hardware: Seeed XIAO RP2040");
  Serial.println("========================================");

  // Initialize colors
  normColor = strip.Color(brightness, brightness, brightness);
  fastColor = strip.Color(brightness * 2, brightness * 2, (brightness * 2) + 40);
  slowColor = strip.Color((brightness / 4) + 20, brightness / 4, brightness / 4);

  now = millis();
  p_now = now;
  lastModeChange = millis();  // Initialize auto-cycle timer
  lastDebounceTime = millis(); // Initialize debounce timer
  digitalWrite(nosePin, HIGH);

  Serial.println("Initialization complete");
  Serial.println("Starting in Auto-Cycle Mode");
  delay(1000);
}
void lights() {
  lights(true); // Default to blinking
}

void lights(bool shouldBlink) {
  if (shouldBlink) {
    if ((millis() - now) > blinkTime) {
      now = millis();
      blinkState = !blinkState;
      digitalWrite(nosePin, blinkState);
      digitalWrite(portPin, blinkState);
      digitalWrite(starPin, blinkState);
    }
  } else {
    // Constant on for white patterns
    digitalWrite(nosePin, HIGH);
    digitalWrite(portPin, HIGH);
    digitalWrite(starPin, HIGH);
  }
}
// Some functions of our own for creating animated effects -----------------

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {  // For each pixel in strip...
    strip.setPixelColor(i, color);               //  Set pixel's color (in RAM)
    strip.show();                                //  Update strip to match
    lights();
    if (gotBreak) {
      gotBreak = false;
      break;
    }
    delay(wait);  //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  // Check if it's a white pattern
  bool isWhite = (color == strip.Color(brightness, brightness, brightness));

  for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
    strip.clear();               //   Set all pixels in RAM to 0 (off)
    // 'c' counts up from 'b' to end of strip in steps of 3...
    for (int c = b; c < strip.numPixels(); c += 3) {
      strip.setPixelColor(c, color);  // Set pixel 'c' to value 'color'
    }
    strip.show();  // Update strip with new contents
    lights(!isWhite);  // Constant lights for white, blinking for colored

    if (gotBreak) {
      gotBreak = false;
      break;
    }
    delay(wait);  // Pause for a moment
  }
}
/*
void colorWipe_smart(uint32_t color) {
  static uint8_t wait = 50;
  static unsigned long lastUpdate = 0;
  static uint16_t currentLed = 0;
  
  unsigned long now = millis();
  if (now > lastUpdate+delay) {
    strip.setPixelColor(currentLed, color);
    strip.show();    
    currentLed = currentLed>strip.numPixels() ? 0 : currentLed+1;
    lastUpdate = now;
  }
}
*/

void setPixel(int Pixel, byte red, byte green, byte blue) {
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
#endif
}

void showStrip() {
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  strip.show();
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  FastLED.show();
#endif
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position = 0;

  for (int j = 0; j < LED_COUNT * 2; j++) {
    Position++;  // = 0; //Position + Rate;
    for (int i = 0; i < LED_COUNT; i++) {
      // sine wave, 3 offset waves make a rainbow!
      //float level = sin(i+Position) * 127 + 128;
      //setPixel(i,level,0,0);
      //float level = sin(i+Position) * 127 + 128;
      setPixel(i, ((sin(i + Position) * 127 + 128) / 255) * red,
               ((sin(i + Position) * 127 + 128) / 255) * green,
               ((sin(i + Position) * 127 + 128) / 255) * blue);
    }

    showStrip();
    lights();
    if (gotBreak) {
      break;
    }
    delay(WaveDelay);
  }
}

void prop(uint32_t fl_color, uint32_t fr_color, uint32_t bl_color, uint32_t br_color, uint32_t t_color, uint fl_wait, int fr_wait, int bl_wait, int br_wait, int t_wait) {

  // for (int c = 48; c < 56; c += 1) {
  //   strip.setPixelColor(c, strip.Color(0, 0, 0));
  // }

  if (p_now > t_lastUpdate + t_wait) {


    for (int c = 48; c < 56; c += 1) {

      if ((c % propSkip) == t_pos) {

        strip.setPixelColor(c, t_color);  // Set pixel 'c' to value 'color'

      } else {

        strip.setPixelColor(c, strip.Color(0, 0, 0));
      }
    }
    strip.show();  // Update strip with new contents
    t_lastUpdate = millis();
    t_pos++;
    if (t_pos >= propSkip) t_pos = 0;
  }

  if (p_now > fl_lastUpdate + fl_wait) {


    for (int c = 0; c < 12; c += 1) {

      if ((c % propSkip) == fl_pos) {

        strip.setPixelColor(c, fl_color);  // Set pixel 'c' to value 'color'

      } else {

        strip.setPixelColor(c, strip.Color(0, 0, 0));
      }
    }
    strip.show();  // Update strip with new contents
    fl_lastUpdate = millis();
    fl_pos++;
    if (fl_pos >= propSkip) fl_pos = 0;
  }
  if (p_now > bl_lastUpdate + bl_wait) {

    for (int c = 12; c < 24; c += 1) {
      if ((c % propSkip) == bl_pos) {
        strip.setPixelColor(c, bl_color);  // Set pixel 'c' to value 'color'

      } else {
        strip.setPixelColor(c, strip.Color(0, 0, 0));
      }
    }
    strip.show();  // Update strip with new contents
    bl_lastUpdate = millis();
    bl_pos++;
    if (bl_pos >= propSkip) bl_pos = 0;
  }
  if (p_now > fr_lastUpdate + fr_wait) {

    for (int c = 24; c < 36; c += 1) {
      if ((c % propSkip) == fr_pos) {
        strip.setPixelColor(c, fr_color);  // Set pixel 'c' to value 'color'

      } else {
        strip.setPixelColor(c, strip.Color(0, 0, 0));
      }
    }
    strip.show();  // Update strip with new contents
    fr_lastUpdate = millis();
    fr_pos++;
    if (fr_pos >= propSkip) fr_pos = 0;
  }
  if (p_now > br_lastUpdate + br_wait) {

    for (int c = 36; c < 48; c += 1) {
      if ((c % propSkip) == br_pos) {
        strip.setPixelColor(c, br_color);  // Set pixel 'c' to value 'color'

      } else {
        strip.setPixelColor(c, strip.Color(0, 0, 0));
      }
    }
    strip.show();  // Update strip with new contents
    br_lastUpdate = millis();
    br_pos++;
    if (br_pos >= propSkip) br_pos = 0;
  }
  lights();
  strip.show();  // Update strip with new contents

  p_now = millis();
  if (gotBreak) {
    gotBreak = false;
    return;
  }
}


// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    strip.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    strip.show();  // Update strip with new contents
    lights();
    if (gotBreak) {
      gotBreak = false;
      break;
    }
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;           // First pixel starts at red (hue 0)
  for (int a = 0; a < 30; a++) {   // Repeat 30 times...
    for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
      strip.clear();               //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for (int c = b; c < strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int hue = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue));  // hue -> RGB
        strip.setPixelColor(c, color);                        // Set pixel 'c' to value 'color'
      }
      strip.show();  // Update strip with new contents
      lights();
      if (gotBreak) {
        break;
      }
      delay(wait);                  // Pause for a moment
      firstPixelHue += 65536 / 90;  // One cycle of color wheel over 90 frames
    }
    if (gotBreak) {
      gotBreak = false;
      break;
    }
  }
}

void BouncingColoredBalls(int BallCount, byte colors[][3]) {
  float Gravity = -9.81;
  int StartHeight = 1;

  float Height[BallCount];
  float ImpactVelocityStart = sqrt(-2 * Gravity * StartHeight);
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int Position[BallCount];
  long ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];

  for (int i = 0; i < BallCount; i++) {
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0;
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i) / pow(BallCount, 2);
  }

  while (true) {
    for (int i = 0; i < BallCount; i++) {
      TimeSinceLastBounce[i] = millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow(TimeSinceLastBounce[i] / 1000, 2.0) + ImpactVelocity[i] * TimeSinceLastBounce[i] / 1000;

      if (Height[i] < 0) {
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();

        if (ImpactVelocity[i] < 0.01) {
          ImpactVelocity[i] = ImpactVelocityStart;
        }
      }
      Position[i] = round(Height[i] * (LED_COUNT - 1) / StartHeight);
    }

    for (int i = 0; i < BallCount; i++) {
      setPixel(Position[i], colors[i][0], colors[i][1], colors[i][2]);
    }
    lights();
    if (gotBreak) {
      break;
    }
    showStrip();
    setAll(0, 0, 0);
  }
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {
  setAll(0, 0, 0);

  for (int i = 0; i < LED_COUNT + LED_COUNT; i++) {


    // fade brightness all LEDs one step
    for (int j = 0; j < LED_COUNT; j++) {
      if ((!meteorRandomDecay) || (random(10) > 5)) {
        fadeToBlack(j, meteorTrailDecay);
      }
    }

    // draw meteor
    for (int j = 0; j < meteorSize; j++) {
      if ((i - j < LED_COUNT) && (i - j >= 0)) {
        setPixel(i - j, red, green, blue);
      }
    }

    showStrip();
    lights();
    if (gotBreak) {
      break;
    }
    delay(SpeedDelay);
  }
}

void fadeToBlack(int ledNo, byte fadeValue) {
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  uint32_t oldColor;
  uint8_t r, g, b;
  int value;

  oldColor = strip.getPixelColor(ledNo);
  r = (oldColor & 0x00ff0000UL) >> 16;
  g = (oldColor & 0x0000ff00UL) >> 8;
  b = (oldColor & 0x000000ffUL);

  r = (r <= 10) ? 0 : (int)r - (r * fadeValue / 256);
  g = (g <= 10) ? 0 : (int)g - (g * fadeValue / 256);
  b = (b <= 10) ? 0 : (int)b - (b * fadeValue / 256);

  strip.setPixelColor(ledNo, r, g, b);
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  leds[ledNo].fadeToBlackBy(fadeValue);
#endif
}

// ===== XMAS PATTERN FUNCTIONS =====
// Convert Wheel position to RGB color (0-255 input)
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Confetti pattern - random colored speckles
void confetti() {
  // Fade all pixels
  for (int i = 0; i < LED_COUNT; i++) {
    fadeToBlack(i, 10);
  }
  // Add a random pixel
  int pos = random(LED_COUNT);
  uint32_t color = Wheel(gHue + random(64));
  strip.setPixelColor(pos, color);
  strip.show();
  lights();
  if (gotBreak) return;
  delay(20);
}

// Juggle pattern - colored dots weaving
void juggle() {
  // Fade all pixels
  for (int i = 0; i < LED_COUNT; i++) {
    fadeToBlack(i, 20);
  }

  // Add 8 dots with different colors
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    // Simplified beatsin - oscillate position
    int pos = ((millis() / (100 - i * 10)) % LED_COUNT);
    uint32_t color = Wheel(dothue);
    strip.setPixelColor(pos, color);
    dothue += 32;
  }
  strip.show();
  lights();
  if (gotBreak) return;
  delay(20);
}

// BPM pattern - pulsing stripes
void bpm() {
  uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);

  for (int i = 0; i < LED_COUNT; i++) {
    uint8_t hue = gHue + (i * 2);
    uint8_t beatBrightness = beat - gHue + (i * 10);
    strip.setPixelColor(i, Wheel(hue));
  }
  strip.show();
  lights();
  if (gotBreak) return;
  delay(20);
}

// Rainbow with glitter
void rainbowWithGlitter() {
  // Fill with rainbow
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, Wheel((i * 256 / LED_COUNT + gHue) & 255));
  }
  // Add glitter
  if (random(100) < 80) {
    strip.setPixelColor(random(LED_COUNT), strip.Color(255, 255, 255));
  }
  strip.show();
  lights();
  if (gotBreak) return;
  delay(20);
}

// Helper function for BPM pattern
uint8_t beatsin8(uint8_t bpm, uint8_t lowest, uint8_t highest) {
  uint16_t beat = (millis() * bpm) / 60000;
  uint8_t beatsin = (sin16(beat * 65536 / 256) + 32768) / 256;
  return map(beatsin, 0, 255, lowest, highest);
}

// Helper function for sine calculation
int16_t sin16(uint16_t theta) {
  static const int16_t base[] = {0, 6393, 12539, 18204, 23170, 27245, 30273, 32137};
  uint16_t offset = (theta & 0x3FFF) >> 3;
  if (theta & 0x4000) offset = 2047 - offset;
  uint8_t section = offset / 256;
  uint8_t b = offset & 0xFF;
  int16_t result = base[section];
  result += ((base[section + 1] - base[section]) * b) >> 8;
  if (theta & 0x8000) result = -result;
  return result;
}

// FLIGHT - Simulates Beta Alia eVTOL flight sequence
// Returns true when the complete sequence is finished
bool flightPattern() {
  static uint8_t prop1_angle = 0, prop2_angle = 0, prop3_angle = 0, prop4_angle = 0;
  static uint8_t tail_pos = 0;
  static unsigned long lastPropUpdate = 0;
  static unsigned long lastTailUpdate = 0;
  static int propDelay = 500;      // Start very slow for visible acceleration
  static int tailDelay = 200;      // Tail slow constant speed initially
  static unsigned long phaseStart = 0;
  static int phase = 0;  // 0=LIFT, 1=TRANSITION_IN, 2=CONVENTIONAL, 3=TRANSITION_OUT/LANDING, 4=GROUND_PAUSE

  const int minPropDelay = 10;      // Fastest prop speed - VERY FAST
  const int maxPropDelay = 500;     // Slowest prop speed (starting speed)
  const int minTailDelay = 50;      // Fastest tail speed
  const int maxTailDelay = 200;     // Slowest tail speed
  const int verySlowTailDelay = 400; // Very slow tail for LIFT and GROUND_PAUSE
  const int liftTime = 5000;        // LIFT: 5 seconds to accelerate props
  const int transitionInHoldTime = 3000;  // Hold at max speed before deceleration
  const int transitionInTime = 8000;   // TRANSITION_IN: 3s hold + 5s decelerate = 8s
  const int conventionalTime = 5000;   // CONVENTIONAL: 5 seconds cruise flight
  const int transitionOutSpinUpTime = 5000;  // 5 seconds to spin up
  const int transitionOutHoldTime = 3000;    // Hold at max speed
  const int transitionOutSpinDownTime = 5000; // 5 seconds to spin down
  const int transitionOutLandingTime = 13000; // 5s + 3s + 5s = 13s total
  const int groundPauseTime = 5000;    // Pause on ground with props parked before completing

  // Helper function: Non-linear prop speed based on progress (0.0 to 1.0)
  // Uses exponential curve: fast through low speeds, more time at high speeds
  auto calculatePropDelay = [](float progress) -> int {
    // Square the progress for exponential curve
    float curved = progress * progress;
    // Interpolate between max (slow) and min (fast)
    return maxPropDelay - (curved * (maxPropDelay - minPropDelay));
  };

  unsigned long now = millis();
  static unsigned long phaseElapsed = 0;

  // PHASE 0: LIFT - Vertical takeoff with non-linear acceleration
  if (phase == 0) {
    phaseElapsed = now - phaseStart;
    static unsigned long lastDebug = 0;
    float liftProgress = (float)phaseElapsed / liftTime;
    if (liftProgress > 1.0) liftProgress = 1.0;

    // Non-linear prop acceleration - fast at start, slower at high speeds
    propDelay = calculatePropDelay(liftProgress);

    // Update props
    if (now - lastPropUpdate >= propDelay) {
      lastPropUpdate = now;

      // Update prop angles (counter-rotation)
      prop1_angle = (prop1_angle > 0) ? prop1_angle - 1 : 8;  // CCW
      prop2_angle = (prop2_angle + 1) % 9;  // CW
      prop3_angle = (prop3_angle + 1) % 9;  // CW
      prop4_angle = (prop4_angle > 0) ? prop4_angle - 1 : 8;  // CCW

      // Debug every 2 seconds
      if (now - lastDebug > 2000) {
        lastDebug = now;
        Serial.print("LIFT: propDelay=");
        Serial.print(propDelay);
        Serial.print(" progress=");
        Serial.print(liftProgress * 100);
        Serial.print("% elapsed=");
        Serial.print(phaseElapsed / 1000);
        Serial.println("s");
      }
    }

    // Update tail - VERY SLOW during lift
    if (now - lastTailUpdate >= verySlowTailDelay) {
      lastTailUpdate = now;
      tail_pos = (tail_pos + 1) % (TAIL_END - TAIL_START + 1);
    }

    // Transition to TRANSITION_IN when time elapsed
    if (phaseElapsed >= liftTime) {
      phase = 1;
      phaseStart = now;
      propDelay = minPropDelay;  // Ensure at max speed
      Serial.println("===== TRANSITION_IN START =====");
    }
  }

  // PHASE 1: TRANSITION_IN - Hold at max speed for 3s, then non-linear decelerate for 5s
  else if (phase == 1) {
    phaseElapsed = now - phaseStart;
    static unsigned long lastDebug = 0;

    // After 3 second hold, start non-linear decelerating
    if (phaseElapsed > transitionInHoldTime) {
      float decelProgress = (float)(phaseElapsed - transitionInHoldTime) / (transitionInTime - transitionInHoldTime);
      if (decelProgress > 1.0) decelProgress = 1.0;
      // Reverse the curve for deceleration (fast at high speeds, slow at low speeds)
      propDelay = calculatePropDelay(1.0 - decelProgress);
    } else {
      // Hold at max speed (minPropDelay)
      propDelay = minPropDelay;
    }

    // Keep props spinning
    if (now - lastPropUpdate >= propDelay) {
      lastPropUpdate = now;

      // Update prop angles (continue counter-rotation)
      prop1_angle = (prop1_angle > 0) ? prop1_angle - 1 : 8;  // CCW
      prop2_angle = (prop2_angle + 1) % 9;  // CW
      prop3_angle = (prop3_angle + 1) % 9;  // CW
      prop4_angle = (prop4_angle > 0) ? prop4_angle - 1 : 8;  // CCW

      // Debug every 2 seconds
      if (now - lastDebug > 2000) {
        lastDebug = now;
        Serial.print("TRANSITION_IN: ");
        if (phaseElapsed <= transitionInHoldTime) {
          Serial.print("HOLDING at max speed, ");
        } else {
          Serial.print("DECELERATING, ");
        }
        Serial.print("propDelay=");
        Serial.print(propDelay);
        Serial.print(" elapsed=");
        Serial.print(phaseElapsed / 1000);
        Serial.println("s");
      }
    }

    // Keep tail at max speed
    if (now - lastTailUpdate >= minTailDelay) {
      lastTailUpdate = now;
      tail_pos = (tail_pos + 1) % (TAIL_END - TAIL_START + 1);
    }

    // Move to CONVENTIONAL when time elapsed
    if (phaseElapsed >= transitionInTime) {
      phase = 2;
      phaseStart = now;
      propDelay = maxPropDelay;  // Ensure props are stopped
      Serial.println("===== CONVENTIONAL START =====");
    }
  }

  // PHASE 2: CONVENTIONAL - Props off/parked, tail at faster speed
  else if (phase == 2) {
    phaseElapsed = now - phaseStart;
    static unsigned long lastDebug = 0;

    // Only tail spinning - faster than other phases
    const int conventionalTailDelay = 30;  // Faster than minTailDelay (50)
    if (now - lastTailUpdate >= conventionalTailDelay) {
      lastTailUpdate = now;
      tail_pos = (tail_pos + 1) % (TAIL_END - TAIL_START + 1);
    }

    // Debug every 2 seconds
    if (now - lastDebug > 2000) {
      lastDebug = now;
      Serial.print("CONVENTIONAL: props OFF, tail at max speed, elapsed=");
      Serial.print(phaseElapsed / 1000);
      Serial.println("s");
    }

    if (phaseElapsed >= conventionalTime) {
      phase = 3;
      phaseStart = now;
      tailDelay = minTailDelay;  // Start from max speed
      Serial.println("===== TRANSITION_OUT/LANDING START =====");
    }
  }

  // PHASE 3: TRANSITION_OUT + LANDING - Non-linear 5s spin up, 3s hold, 5s spin down
  else if (phase == 3) {
    phaseElapsed = now - phaseStart;
    static unsigned long lastDebug = 0;

    // Decelerate tail throughout
    if (now - lastTailUpdate >= tailDelay) {
      lastTailUpdate = now;
      tailDelay += 1;
      if (tailDelay > maxTailDelay) tailDelay = maxTailDelay;
      tail_pos = (tail_pos + 1) % (TAIL_END - TAIL_START + 1);
    }

    // Props: Non-linear acceleration and deceleration
    if (phaseElapsed < transitionOutSpinUpTime) {
      // 0-5s: Non-linear accelerate props
      float accelProgress = (float)phaseElapsed / transitionOutSpinUpTime;
      propDelay = calculatePropDelay(accelProgress);
    } else if (phaseElapsed < transitionOutSpinUpTime + transitionOutHoldTime) {
      // 5-8s: Hold at max speed (minPropDelay)
      propDelay = minPropDelay;
    } else {
      // 8-13s: Non-linear decelerate props for landing
      float decelProgress = (float)(phaseElapsed - transitionOutSpinUpTime - transitionOutHoldTime) / transitionOutSpinDownTime;
      if (decelProgress > 1.0) decelProgress = 1.0;
      propDelay = calculatePropDelay(1.0 - decelProgress);
    }

    // Update props
    if (now - lastPropUpdate >= propDelay) {
      lastPropUpdate = now;

      // Update prop angles (counter-rotation)
      prop1_angle = (prop1_angle > 0) ? prop1_angle - 1 : 8;  // CCW
      prop2_angle = (prop2_angle + 1) % 9;  // CW
      prop3_angle = (prop3_angle + 1) % 9;  // CW
      prop4_angle = (prop4_angle > 0) ? prop4_angle - 1 : 8;  // CCW
    }

    // Debug every 2 seconds
    if (now - lastDebug > 2000) {
      lastDebug = now;
      Serial.print("TRANS_OUT/LANDING: ");
      if (phaseElapsed < transitionOutSpinUpTime) {
        Serial.print("SPIN UP, ");
      } else if (phaseElapsed < transitionOutSpinUpTime + transitionOutHoldTime) {
        Serial.print("HOLDING at max, ");
      } else {
        Serial.print("SPIN DOWN, ");
      }
      Serial.print("propDelay=");
      Serial.print(propDelay);
      Serial.print(" tailDelay=");
      Serial.print(tailDelay);
      Serial.print(" elapsed=");
      Serial.print(phaseElapsed / 1000);
      Serial.println("s");
    }

    // Transition to ground pause when landing complete
    if (phaseElapsed >= transitionOutLandingTime) {
      phase = 4;  // Move to GROUND_PAUSE
      phaseStart = now;
      propDelay = maxPropDelay;  // Props fully stopped
      Serial.println("===== LANDING COMPLETE - GROUND PAUSE =====");
    }
  }

  // PHASE 4: GROUND_PAUSE - Props parked, tail VERY slow sequence for 5 seconds
  else if (phase == 4) {
    phaseElapsed = now - phaseStart;
    static unsigned long lastDebug = 0;

    // Keep tail sequencing VERY slowly
    if (now - lastTailUpdate >= verySlowTailDelay) {
      lastTailUpdate = now;
      tail_pos = (tail_pos + 1) % (TAIL_END - TAIL_START + 1);
    }

    // Debug every 2 seconds
    if (now - lastDebug > 2000) {
      lastDebug = now;
      Serial.print("GROUND_PAUSE: elapsed=");
      Serial.print(phaseElapsed / 1000);
      Serial.println("s");
    }

    // Complete sequence after ground pause
    if (phaseElapsed >= groundPauseTime) {
      phase = 0;
      phaseStart = now;
      propDelay = maxPropDelay;
      tailDelay = maxTailDelay;
      Serial.println("===== FLIGHT SEQUENCE COMPLETE =====");
      return true;  // Signal completion
    }
  }

  // RENDER LEDS
  strip.clear();

  // Render props (2-LED blade pattern) - Show parked position when stopped
  bool propsSpinning = (phase != 2) && (propDelay < maxPropDelay);

  if (propsSpinning) {
    // Props are spinning - show animated 2-LED blade pattern
    // Prop 1 (CCW): LEDs 0-8 (9 LEDs)
    uint8_t led1a = prop1_angle % 9;
    uint8_t led1b = (prop1_angle + 4) % 9;  // Opposite side
    strip.setPixelColor(PROP1_START + led1a, normColor);
    strip.setPixelColor(PROP1_START + led1b, normColor);

    // Prop 2 (CW): LEDs 9-17
    uint8_t led2a = prop2_angle % 9;
    uint8_t led2b = (prop2_angle + 4) % 9;
    strip.setPixelColor(PROP2_START + led2a, normColor);
    strip.setPixelColor(PROP2_START + led2b, normColor);

    // Prop 3 (CW): LEDs 18-26
    uint8_t led3a = prop3_angle % 9;
    uint8_t led3b = (prop3_angle + 4) % 9;
    strip.setPixelColor(PROP3_START + led3a, normColor);
    strip.setPixelColor(PROP3_START + led3b, normColor);

    // Prop 4 (CCW): LEDs 27-35
    uint8_t led4a = prop4_angle % 9;
    uint8_t led4b = (prop4_angle + 4) % 9;
    strip.setPixelColor(PROP4_START + led4a, normColor);
    strip.setPixelColor(PROP4_START + led4b, normColor);
  } else {
    // Props are parked - show stationary blade positions
    // Prop 1: LEDs 0 and 4
    strip.setPixelColor(PROP1_START + 0, normColor);
    strip.setPixelColor(PROP1_START + 4, normColor);

    // Prop 2: LEDs 8 and 4 (user requested 8 and 4)
    strip.setPixelColor(PROP2_START + 8, normColor);
    strip.setPixelColor(PROP2_START + 4, normColor);

    // Prop 3: LEDs 0 and 4
    strip.setPixelColor(PROP3_START + 0, normColor);
    strip.setPixelColor(PROP3_START + 4, normColor);

    // Prop 4: LEDs 8 and 4 (user requested 8 and 4)
    strip.setPixelColor(PROP4_START + 8, normColor);
    strip.setPixelColor(PROP4_START + 4, normColor);
  }

  // Render tail (single LED sequencing)
  uint8_t tailLed = TAIL_START + tail_pos;
  strip.setPixelColor(tailLed, normColor);

  strip.show();
  lights(false);  // Constant nav lights for white pattern

  if (gotBreak) {
    // Reset for next run
    phase = 0;
    phaseStart = millis();
    propDelay = maxPropDelay;
    tailDelay = maxTailDelay;
    prop1_angle = 0;
    prop2_angle = 0;
    prop3_angle = 0;
    prop4_angle = 0;
    tail_pos = 0;
    gotBreak = false;
    return false;  // Interrupted, not complete
  }

  return false;  // Still running
}

void loop() {
  // Check button state
  checkButton();

  // Update gHue for color cycling effects
  static unsigned long lastHueUpdate = 0;
  if (millis() - lastHueUpdate > 20) {
    gHue++;
    lastHueUpdate = millis();
  }

  byte colors[3][3] = { { 0xff, 0, 0 },
                        { 0xff, 0xff, 0xff },
                        { 0, 0, 0xff } };

  switch (mode) {
    case 0: {
      // Auto-cycle mode - cycles through all patterns
      // Use pattern completion flag for FLIGHT, timer for others
      bool shouldAdvance = false;

      // Debug current state
      static unsigned long lastStateDebug = 0;
      if (millis() - lastStateDebug > 5000) {
        lastStateDebug = millis();
        Serial.print("AutoCycle: subMode=");
        Serial.print(autoCycleSubMode);
        Serial.print(" patternComplete=");
        Serial.print(patternComplete);
        Serial.print(" timeSinceChange=");
        Serial.print((millis() - lastModeChange) / 1000);
        Serial.println("s");
      }

      if (autoCycleSubMode == 0) {
        // FLIGHT pattern - wait for completion signal ONLY
        if (patternComplete) {
          shouldAdvance = true;
          patternComplete = false;
          Serial.println("FLIGHT pattern signaled completion!");
        }
      } else {
        // Other patterns - use timer
        if (millis() - lastModeChange >= AUTO_CYCLE_DURATION) {
          shouldAdvance = true;
          Serial.println("Timer expired for other pattern");
        }
      }

      if (shouldAdvance) {
        lastModeChange = millis();
        autoCycleSubMode++;
        if (autoCycleSubMode >= 4) {  // 4 patterns: FLIGHT, SLOW RAINBOW, FAST WHITE, RAINBOW PROPS
          autoCycleSubMode = 0;
        }
        strip.clear();
        strip.show();  // Force all LEDs off
        delay(100);    // Wait 100ms for power to stabilize before starting new pattern
        const char* subModeNames[] = {"FLIGHT", "SLOW RAINBOW", "FAST WHITE", "RAINBOW PROPS"};
        Serial.print("===== Auto-cycle switching to: ");
        Serial.print(subModeNames[autoCycleSubMode]);
        Serial.println(" =====");
      }

      // Run the current pattern
      switch (autoCycleSubMode) {
        case 0:
          patternComplete = flightPattern();  // FLIGHT - returns true when complete
          break;
        case 1:
          rainbow(waitTime / 5);  // SLOW RAINBOW - restored
          break;
        case 2:
          RunningLights(0xff, 0xff, 0xff, 50);  // FAST WHITE
          break;
        case 3:
          theaterChaseRainbow(waitTime);  // RAINBOW PROPS
          break;
      }
      break;
    }

    case 1:
      // Xmas Auto-cycle mode - cycles through Christmas patterns
      // Check if it's time to advance to next pattern
      if (millis() - lastXmasModeChange >= XMAS_CYCLE_DURATION) {
        lastXmasModeChange = millis();
        xmasSubMode++;
        if (xmasSubMode >= 5) {
          xmasSubMode = 0;
        }
        strip.clear();
        Serial.print("Xmas auto-cycle switching to sub-mode: ");
        Serial.println(xmasSubMode);
      }

      // Run the current Xmas pattern
      switch (xmasSubMode) {
        case 0:
          rainbowWithGlitter();
          break;
        case 1:
          confetti();
          break;
        case 2:
          juggle();
          break;
        case 3:
          bpm();
          break;
        case 4:
          theaterChaseRainbow(waitTime);
          break;
      }
      break;

    case 2:
      theaterChase(strip.Color(brightness, brightness, brightness), waitTime);  // White, half brightness
      break;

    case 3:
      rainbow(waitTime / 5);  // Flowing rainbow cycle along the whole strip
      break;
    case 4:
      theaterChaseRainbow(waitTime);  // Rainbow-enhanced theaterChase variant
      break;
    case 5:
      RunningLights(0xff, 0xff, 0xff, 50);
      break;

    case 6:
      // Static prop mode - no IMU control
      fl_speed = normProp;
      fr_speed = normProp;
      bl_speed = normProp;
      br_speed = normProp;
      t_speed = normProp;
      fl_color = normColor;
      bl_color = normColor;
      fr_color = normColor;
      br_color = normColor;
      t_color = normColor;

      prop(fl_color, fr_color, bl_color, br_color, t_color, fl_speed, fr_speed, bl_speed, br_speed, t_speed);
      break;
    default:
      // statements
      break;
  }
}
