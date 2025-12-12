/*
 * Alia_4 - Beta Alia eVTOL LED Animation Controller
 *
 * VERSION: 1.2.0 (Modular Pattern Architecture)
 * BUILD: 16
 * DATE: 2024-12-12
 *
 * This is a well-documented reference implementation for building LED animation
 * controllers with modular patterns. Easy to customize and extend!
 *
 * ========== HARDWARE CONFIGURATION ==========
 * Board: Seeed XIAO RP2040
 * LEDs: WS2812B addressable LEDs (41 total)
 *   - 4 Lift Props (9 LEDs each): LEDs 0-35
 *   - Tail Prop (5 LEDs): LEDs 36-40
 *
 * Pin Assignments:
 *   - GPIO 0: Starboard navigation light (green)
 *   - GPIO 1: Port navigation light (red)
 *   - GPIO 2: Nose navigation light (white)
 *   - GPIO 4: WS2812B data line
 *
 * ========== AUTO-CYCLE PATTERNS ==========
 * The system automatically cycles through these patterns:
 *   1. FLIGHT      - eVTOL flight simulation (~36s) - Uses completion flag
 *   2. SLOW RAINBOW - Rainbow cycle (10s) - Uses timer
 *   3. FAST WHITE   - Running white lights (10s) - Uses timer
 *   4. RAINBOW PROPS - Theater chase rainbow (10s) - Uses timer
 *
 * ========== HOW TO ADD YOUR OWN PATTERN ==========
 * Follow these steps to add a new pattern to the auto-cycle:
 *
 * STEP 1: Write your pattern function
 *   - For simple patterns: void myPattern() { ... }
 *   - For complex patterns: bool myPattern() { return true; } // true when done
 *   - Use strip.setPixelColor() to set LED colors
 *   - Call strip.show() to update the LEDs
 *   - Call lights() or lights(false) for navigation lights
 *   - Check gotBreak flag to allow early exit
 *
 * STEP 2: Add your pattern to the CUSTOM PATTERNS section (line ~600)
 *   - See the pattern template and examples
 *
 * STEP 3: Update NUM_PATTERNS constant (line ~76)
 *   - Increment by 1 for each pattern you add
 *
 * STEP 4: Add your pattern to the switch statement in loop() (line ~1020)
 *   - Add a new case with your pattern number
 *   - Call your pattern function
 *
 * STEP 5: Add pattern name to subModeNames array (line ~1032)
 *   - For debug output
 *
 * See README.md for detailed examples and best practices.
 *
 * ========== KEY DESIGN PATTERNS ==========
 * - Finite State Machine: Uses phase variable to track animation stages
 * - Non-linear Motion: Exponential curves (progress²) for realistic movement
 * - Completion Flags: Complex patterns return bool, simple patterns use timer
 * - Power Management: 100ms delay between patterns prevents brownouts
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// ===== VERSION INFO =====
#define VERSION "1.2.0"
#define BUILD_NUMBER 16

// ===== PATTERN CONFIGURATION =====
// ***** CHANGE THIS when you add/remove patterns from auto-cycle *****
#define NUM_PATTERNS 4  // Total number of patterns in auto-cycle (0-3)

// ===== LED CONFIGURATION =====
// These brightness values work well with USB-C power (2A+)
// For USB-A power, reduce brightness to 25 to prevent brownouts
#define brightness 50  // LED brightness (0-255)
#define fastProp 2     // Fast prop speed constant (legacy, not actively used)
#define slowProp 100   // Slow prop speed constant (legacy, not actively used)
#define normProp 50    // Normal prop speed constant (legacy, not actively used)
#define propSkip 2     // Prop skip constant for legacy prop() function

// Seeed XIAO RP2040 Pin Definitions
#define portPin 1      // GPIO 1 - Port navigation light (red)
#define nosePin 2      // GPIO 2 - Nose navigation light (white)
#define ledPin 4       // GPIO 4 - WS2812B data pin (Physical pin 10)
#define starPin  0     // GPIO 0 - Starboard navigation light (green)
#define waitTime 50    // Default wait time for pattern animations (ms)

// ===== STATE VARIABLES =====
// Navigation light blinking
int blinkTime = 1500;      // Blink interval for colored patterns (ms)
bool blinkState = true;    // Current blink state

// Mode control (simplified to just auto-cycle)
int mode = 0;              // Current mode (always 0 for auto-cycle)
int maxMode = 1;           // Only one mode available
bool gotBreak = false;     // Flag to signal pattern interruption

// Auto-cycle pattern management
unsigned long lastModeChange = 0;      // Timestamp of last pattern change
int autoCycleSubMode = 0;              // Current pattern in auto-cycle (0-3)
bool patternComplete = false;          // Completion flag for FLIGHT pattern
const int AUTO_CYCLE_DURATION = 10000; // Duration for simple patterns (10 seconds)

// ===== COLOR DEFINITIONS =====
// Pre-calculated RGB colors for different animation states
uint32_t normColor, fastColor, slowColor;

// Legacy prop color variables (used by old prop() function)
uint32_t fl_color, fr_color, bl_color, br_color, t_color;

// ===== TIMING VARIABLES =====
unsigned long int now = 0;   // Current time for navigation light blinking
unsigned long int p_now = 0; // Current time for legacy prop() function

// ===== LEGACY PROP FUNCTION VARIABLES =====
// These are used by the prop() function which is not part of the auto-cycle
// but kept for Mode 6 compatibility
static uint8_t fl_pos = 0;           // Front left prop position
static uint8_t fr_pos = 0;           // Front right prop position
static uint8_t bl_pos = 0;           // Back left prop position
static uint8_t br_pos = 0;           // Back right prop position
static uint8_t t_pos = 0;            // Tail prop position
int fl_speed = normProp;             // Front left speed
int fr_speed = normProp;             // Front right speed
int bl_speed = normProp;             // Back left speed
int br_speed = normProp;             // Back right speed
int t_speed = normProp;              // Tail speed

// Last update timestamps for legacy prop() function
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

// ===== LED STRIP CONFIGURATION =====
#define LED_COUNT 41

// LED segment definitions - Maps physical LED positions to logical props
// Props are arranged in a quad configuration:
//   Prop 1 (Front Left)   Prop 2 (Front Right)
//   Prop 3 (Rear Left)    Prop 4 (Rear Right)
//              Tail (Rear Center)
#define PROP1_START 0      // Prop 1: LEDs 0-8 (9 LEDs) - Rotates CCW
#define PROP1_END 8
#define PROP2_START 9      // Prop 2: LEDs 9-17 (9 LEDs) - Rotates CW
#define PROP2_END 17
#define PROP3_START 18     // Prop 3: LEDs 18-26 (9 LEDs) - Rotates CW
#define PROP3_END 26
#define PROP4_START 27     // Prop 4: LEDs 27-35 (9 LEDs) - Rotates CCW
#define PROP4_END 35
#define TAIL_START 36      // Tail: LEDs 36-40 (5 LEDs) - Single LED sequencing
#define TAIL_END 40

// Initialize NeoPixel strip object
Adafruit_NeoPixel strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800);

// ===== SETUP FUNCTION =====
// Runs once at startup to initialize hardware and serial communication

void setup() {
  // Initialize navigation light pins as outputs
  pinMode(nosePin, OUTPUT);
  pinMode(portPin, OUTPUT);
  pinMode(starPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Initialize I2C (not actively used, but kept for compatibility)
  Wire.begin();
  Wire.setClock(400000);  // 400kHz I2C clock

  // Initialize serial communication for debug output
  Serial.begin(115200);

  // Initialize LED strip
  strip.setBrightness(brightness);
  strip.begin();

  // Print startup banner with version info
  Serial.println("========================================");
  Serial.println("  Alia_4 - Beta Alia eVTOL LED Controller");
  Serial.print("  Version: ");
  Serial.println(VERSION);
  Serial.print("  Build: ");
  Serial.println(BUILD_NUMBER);
  Serial.println("  Hardware: Seeed XIAO RP2040");
  Serial.println("========================================");

  // Pre-calculate RGB colors for different animation states
  // normColor: White color for standard prop/tail display
  normColor = strip.Color(brightness, brightness, brightness);
  // fastColor: Bluish-white for fast animations (legacy, not actively used)
  fastColor = strip.Color(brightness * 2, brightness * 2, (brightness * 2) + 40);
  // slowColor: Reddish color for slow animations (legacy, not actively used)
  slowColor = strip.Color((brightness / 4) + 20, brightness / 4, brightness / 4);

  // Initialize timing variables
  now = millis();
  p_now = now;
  lastModeChange = millis();  // Start auto-cycle timer

  // Turn on nose navigation light
  digitalWrite(nosePin, HIGH);

  Serial.println("Initialization complete");
  Serial.println("Starting in Auto-Cycle Mode");
  delay(1000);
}

// ===== NAVIGATION LIGHT CONTROL =====
// Controls the three navigation lights (nose, port, starBoard)
// White patterns: constant on
// Colored patterns: blinking

// Overload: Default to blinking mode
void lights() {
  lights(true);
}

// Controls navigation lights based on pattern type
// Parameters:
//   shouldBlink: true for blinking (colored patterns), false for constant (white patterns)
void lights(bool shouldBlink) {
  if (shouldBlink) {
    // Blink navigation lights every blinkTime milliseconds
    if ((millis() - now) > blinkTime) {
      now = millis();
      blinkState = !blinkState;
      digitalWrite(nosePin, blinkState);
      digitalWrite(portPin, blinkState);
      digitalWrite(starPin, blinkState);
    }
  } else {
    // Keep navigation lights constantly on for white patterns
    digitalWrite(nosePin, HIGH);
    digitalWrite(portPin, HIGH);
    digitalWrite(starPin, HIGH);
  }
}

// ===== HELPER FUNCTIONS FOR LED PATTERNS =====
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

// ===== FLIGHT PATTERN FUNCTION =====
//
// FLIGHT - Simulates Beta Alia eVTOL complete flight sequence
//
// This function implements a realistic flight pattern with 5 distinct phases:
//   Phase 0: LIFT           - Vertical takeoff (5s) - Props accelerate, tail very slow
//   Phase 1: TRANSITION_IN  - Transition to forward flight (8s) - Hold max, then decel
//   Phase 2: CONVENTIONAL   - Cruise flight (5s) - Props parked, tail fast
//   Phase 3: TRANSITION_OUT - Transition back (13s) - Props accel, hold, decel for landing
//   Phase 4: GROUND_PAUSE   - Resting on ground (5s) - Props parked, tail very slow
//
// Total cycle time: ~36 seconds
//
// Key Design Patterns:
//   1. Finite State Machine - Uses phase variable to track flight stage
//   2. Non-linear Motion - Exponential curves (progress²) for realistic acceleration
//   3. Completion Flag - Returns true when done instead of using timer (prevents interruption)
//   4. Static Variables - Maintains state between calls (runs once per loop() iteration)
//
// Prop Rotation:
//   - Props 1 & 4: Counter-clockwise (CCW)
//   - Props 2 & 3: Clockwise (CW)
//   - 2-LED "blade" pattern when spinning
//   - Static "parked" position when stopped
//
// Returns:
//   true  - Sequence complete, ready to advance to next pattern
//   false - Still running or interrupted
//
bool flightPattern() {
  // Static variables maintain state between function calls
  static uint8_t prop1_angle = 0, prop2_angle = 0, prop3_angle = 0, prop4_angle = 0;
  static uint8_t tail_pos = 0;
  static unsigned long lastPropUpdate = 0;
  static unsigned long lastTailUpdate = 0;
  static int propDelay = 500;      // Start very slow for visible acceleration
  static int tailDelay = 200;      // Tail slow constant speed initially
  static unsigned long phaseStart = 0;
  static int phase = 0;  // Current phase: 0=LIFT, 1=TRANSITION_IN, 2=CONVENTIONAL, 3=TRANSITION_OUT/LANDING, 4=GROUND_PAUSE

  // ===== TIMING CONSTANTS =====
  // Speed parameters (in milliseconds between updates - lower = faster)
  const int minPropDelay = 10;      // Fastest prop speed - VERY FAST (10ms between updates)
  const int maxPropDelay = 500;     // Slowest prop speed (starting/parked speed)
  const int minTailDelay = 50;      // Fastest tail speed
  const int maxTailDelay = 200;     // Slowest tail speed
  const int verySlowTailDelay = 400; // Very slow tail for LIFT and GROUND_PAUSE phases

  // Phase durations (in milliseconds)
  const int liftTime = 5000;        // LIFT: 5 seconds to accelerate props
  const int transitionInHoldTime = 3000;  // Hold at max speed before deceleration
  const int transitionInTime = 8000;   // TRANSITION_IN: 3s hold + 5s decelerate = 8s total
  const int conventionalTime = 5000;   // CONVENTIONAL: 5 seconds cruise flight
  const int transitionOutSpinUpTime = 5000;  // 5 seconds to spin up
  const int transitionOutHoldTime = 3000;    // Hold at max speed
  const int transitionOutSpinDownTime = 5000; // 5 seconds to spin down
  const int transitionOutLandingTime = 13000; // 5s + 3s + 5s = 13s total
  const int groundPauseTime = 5000;    // Pause on ground with props parked before completing

  // ===== NON-LINEAR MOTION CALCULATOR =====
  // Lambda function to calculate prop delay based on progress (0.0 to 1.0)
  // Uses exponential curve (progress²) so props:
  //   - Accelerate quickly through low speeds (visible movement immediately)
  //   - Spend more time at high speeds (smooth, gradual approach to max)
  // This creates a more natural, organic motion pattern
  auto calculatePropDelay = [](float progress) -> int {
    float curved = progress * progress;  // Exponential curve (square)
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

// ============================================================================
// ===== CUSTOM PATTERNS SECTION =====
// ============================================================================
//
// Add your own custom patterns here! Follow the template below.
//
// PATTERN TEMPLATE - Copy this to create your own pattern:
//
// // Pattern description - what does it do?
// void myCustomPattern() {
//   // Your animation code here
//   // Example: Light up all LEDs in a specific color
//   for (int i = 0; i < LED_COUNT; i++) {
//     strip.setPixelColor(i, strip.Color(255, 0, 0));  // Red
//   }
//   strip.show();
//   lights();  // Update navigation lights (blinking for colored patterns)
//   delay(50); // Small delay to control animation speed
// }
//
// ADVANCED PATTERN TEMPLATE - For patterns that need completion tracking:
//
// // Complex pattern with multiple phases - returns true when complete
// bool myComplexPattern() {
//   static int phase = 0;
//   static unsigned long phaseStart = 0;
//
//   // Initialize on first call
//   if (phaseStart == 0) {
//     phaseStart = millis();
//   }
//
//   // Your phase-based animation logic here
//   if (phase == 0) {
//     // Do something for 5 seconds
//     if (millis() - phaseStart > 5000) {
//       phase = 1;
//       phaseStart = millis();
//     }
//   } else if (phase == 1) {
//     // Do something else for 3 seconds
//     if (millis() - phaseStart > 3000) {
//       phase = 0;
//       phaseStart = 0;
//       return true;  // Signal completion
//     }
//   }
//
//   // Update LEDs
//   strip.show();
//   lights();
//
//   // Check for early exit
//   if (gotBreak) {
//     phase = 0;
//     phaseStart = 0;
//     gotBreak = false;
//     return false;
//   }
//
//   return false;  // Still running
// }
//
// EXAMPLE CUSTOM PATTERN 1: Simple color wipe
// Uncomment to use:
//
// void redWipePattern() {
//   static int currentLED = 0;
//   static unsigned long lastUpdate = 0;
//
//   if (millis() - lastUpdate > 50) {
//     strip.setPixelColor(currentLED, strip.Color(brightness, 0, 0));
//     strip.show();
//     lights();
//
//     currentLED++;
//     if (currentLED >= LED_COUNT) {
//       currentLED = 0;
//       strip.clear();
//     }
//
//     lastUpdate = millis();
//   }
// }
//
// EXAMPLE CUSTOM PATTERN 2: Breathing effect
// Uncomment to use:
//
// void breathingPattern() {
//   static int brightness_val = 0;
//   static int direction = 1;
//   static unsigned long lastUpdate = 0;
//
//   if (millis() - lastUpdate > 20) {
//     brightness_val += direction * 5;
//     if (brightness_val >= 255) {
//       brightness_val = 255;
//       direction = -1;
//     } else if (brightness_val <= 0) {
//       brightness_val = 0;
//       direction = 1;
//     }
//
//     for (int i = 0; i < LED_COUNT; i++) {
//       strip.setPixelColor(i, strip.Color(0, 0, brightness_val));
//     }
//     strip.show();
//     lights();
//
//     lastUpdate = millis();
//   }
// }
//
// After adding your pattern:
// 1. Increment NUM_PATTERNS at the top of the file
// 2. Add a case to the switch statement in loop()
// 3. Add the pattern name to subModeNames array
//
// ============================================================================

// ===== MAIN LOOP =====
// Runs continuously - manages auto-cycle pattern rotation
//
// Auto-Cycle Operation:
//   1. FLIGHT pattern runs until completion (returns true)
//   2. Other patterns run for AUTO_CYCLE_DURATION (10 seconds)
//   3. 100ms delay between patterns prevents power brownouts
//   4. Cycles through 4 patterns: FLIGHT, SLOW RAINBOW, FAST WHITE, RAINBOW PROPS
//
void loop() {
  // ===== AUTO-CYCLE MODE =====
  // Automatically cycles through 4 different patterns
  // Uses completion flag for FLIGHT, timer for others

  bool shouldAdvance = false;  // Flag to signal pattern transition

  // Debug output every 5 seconds (helpful for monitoring)
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

  // Check if it's time to advance to next pattern
  if (autoCycleSubMode == 0) {
    // FLIGHT pattern - wait for completion signal ONLY (ignores timer)
    // This prevents interruption mid-sequence
    if (patternComplete) {
      shouldAdvance = true;
      patternComplete = false;
      Serial.println("FLIGHT pattern signaled completion!");
    }
  } else {
    // Other patterns - use timer (10 seconds each)
    if (millis() - lastModeChange >= AUTO_CYCLE_DURATION) {
      shouldAdvance = true;
      Serial.println("Timer expired for pattern");
    }
  }

  // Advance to next pattern if needed
  if (shouldAdvance) {
    lastModeChange = millis();
    autoCycleSubMode++;
    if (autoCycleSubMode >= NUM_PATTERNS) {  // Uses NUM_PATTERNS constant
      autoCycleSubMode = 0;
    }

    // Clear LEDs and wait for power to stabilize (prevents brownouts)
    strip.clear();
    strip.show();
    delay(100);  // 100ms stabilization delay

    // ***** ADD YOUR PATTERN NAME HERE (for debug output) *****
    const char* subModeNames[] = {
      "FLIGHT",         // Pattern 0
      "SLOW RAINBOW",   // Pattern 1
      "FAST WHITE",     // Pattern 2
      "RAINBOW PROPS"   // Pattern 3
      // Add more pattern names here as you add patterns
    };
    Serial.print("===== Auto-cycle switching to: ");
    Serial.print(subModeNames[autoCycleSubMode]);
    Serial.println(" =====");
  }

  // ===== PATTERN EXECUTION =====
  // ***** ADD YOUR PATTERN CASE HERE *****
  switch (autoCycleSubMode) {
    case 0:
      // FLIGHT - Complete eVTOL flight simulation (~36 seconds)
      // Uses completion flag (returns true when done)
      patternComplete = flightPattern();
      break;

    case 1:
      // SLOW RAINBOW - Full rainbow cycle across all 41 LEDs (10 seconds)
      // Uses timer (AUTO_CYCLE_DURATION)
      rainbow(waitTime / 5);
      break;

    case 2:
      // FAST WHITE - Running white lights with sine wave (10 seconds)
      // Uses timer (AUTO_CYCLE_DURATION)
      RunningLights(0xff, 0xff, 0xff, 50);
      break;

    case 3:
      // RAINBOW PROPS - Theater chase rainbow effect (10 seconds)
      // Uses timer (AUTO_CYCLE_DURATION)
      theaterChaseRainbow(waitTime);
      break;

    // ***** ADD YOUR CUSTOM PATTERN CASES HERE *****
    // Example:
    // case 4:
    //   myCustomPattern();
    //   break;
    //
    // case 5:
    //   patternComplete = myComplexPattern();  // If using completion flag
    //   break;
  }
}
