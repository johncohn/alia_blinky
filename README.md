# Alia 4 - Beta Alia eVTOL LED Animation Controller

LED animation controller for a scale model of the Beta Alia electric vertical takeoff and landing (eVTOL) aircraft. Features realistic flight simulation with dynamic prop and tail animations, plus additional color patterns.

**Version:** 1.0.3
**Build:** 14
**Date:** 2024-12-12
**Hardware:** Seeed XIAO RP2040

## Features

### Auto-Cycle Mode (Default)
Automatically cycles through 4 animation patterns:

1. **FLIGHT** (~36 seconds) - Realistic eVTOL flight simulation
   - LIFT: Props accelerate with non-linear curve, tail slow
   - TRANSITION_IN: Props hold max speed, then decelerate to parked
   - CONVENTIONAL: Props parked, tail spinning fast
   - TRANSITION_OUT/LANDING: Props spin up, hold, then land
   - GROUND_PAUSE: Props parked, tail slow idle

2. **SLOW RAINBOW** (10 seconds) - Full rainbow cycle across all LEDs

3. **FAST WHITE** (10 seconds) - Running white lights with sine wave

4. **RAINBOW PROPS** (10 seconds) - Theater chase rainbow effect

### Additional Modes
- **Mode 1:** Christmas patterns (rainbow glitter, confetti, juggle, BPM, theater rainbow)
- **Modes 2-6:** Individual pattern selections

## Hardware Configuration

### Microcontroller
- **Board:** Seeed XIAO RP2040
- **Core:** arduino-pico (rp2040:rp2040:seeed_xiao_rp2040)
- **Upload Method:** UF2 bootloader

### LED Layout (41 WS2812B LEDs)
- **Prop 1 (Front Left):** LEDs 0-8 (9 LEDs) - Counter-clockwise rotation
- **Prop 2 (Front Right):** LEDs 9-17 (9 LEDs) - Clockwise rotation
- **Prop 3 (Rear Left):** LEDs 18-26 (9 LEDs) - Clockwise rotation
- **Prop 4 (Rear Right):** LEDs 27-35 (9 LEDs) - Counter-clockwise rotation
- **Tail Prop:** LEDs 36-40 (5 LEDs)

### Pin Assignments

| Pin | GPIO | Function | Physical Pin |
|-----|------|----------|--------------|
| D1 | GPIO 1 | Port navigation light | 8 |
| D2 | GPIO 2 | Nose navigation light | 9 |
| D4 | GPIO 4 | WS2812B data line | 10 |
| D0 | GPIO 0 | Starboard navigation light | 7 |
| D3 | GPIO 3 | Reserved for external button | - |

### Navigation Lights
- **White patterns:** Constant on
- **Colored patterns:** Blinking (1.5s interval)

## FLIGHT Pattern Details

### Phase Timing
- **LIFT:** 5 seconds
- **TRANSITION_IN:** 8 seconds (3s hold + 5s deceleration)
- **CONVENTIONAL:** 5 seconds
- **TRANSITION_OUT/LANDING:** 13 seconds (5s spin up + 3s hold + 5s spin down)
- **GROUND_PAUSE:** 5 seconds
- **Total:** ~36 seconds

### Animation Features
- **Non-linear acceleration:** Props accelerate quickly from parked to mid-speed, then gradually to full speed
- **Non-linear deceleration:** Props spend more time at high speeds, then quickly drop to parked
- **Prop speeds:** 10ms (max) to 500ms (parked)
- **Tail speeds:**
  - LIFT/GROUND_PAUSE: 400ms (very slow)
  - CONVENTIONAL: 30ms (fast)
  - Other phases: 50-200ms
- **Parked positions:** Props 1&3 use LEDs 0&4, Props 2&4 use LEDs 8&4

### Prop Rotation
Props display a 2-LED blade pattern:
- **Props 1 & 4:** Counter-clockwise (CCW)
- **Props 2 & 3:** Clockwise (CW)

## Installation

### Requirements
- Arduino CLI or Arduino IDE
- Adafruit NeoPixel library (1.15.2 or later)
- arduino-pico core (5.4.3 or later)

### Install arduino-pico Core
```bash
arduino-cli core install rp2040:rp2040
```

### Install Libraries
```bash
arduino-cli lib install "Adafruit NeoPixel"
```

### Compile
```bash
arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 --export-binaries Alia_4_v9.ino
```

### Upload
1. Put XIAO RP2040 in bootloader mode:
   - Press and hold BOOT button
   - Press and release RESET button
   - Release BOOT button
   - OR: Double-tap RESET button quickly

2. RPI-RP2 drive should appear on your computer

3. Copy UF2 file to the drive:
```bash
cp build/rp2040.rp2040.seeed_xiao_rp2040/Alia_4_v9.ino.uf2 /Volumes/RPI-RP2/
```

4. Board will automatically reboot and start running

## Configuration

### Brightness
Adjust LED brightness by changing the `brightness` define:
```cpp
#define brightness 50  // Range: 0-255
```

### Pattern Timing
Modify pattern duration in `AUTO_CYCLE_DURATION`:
```cpp
const int AUTO_CYCLE_DURATION = 10000; // milliseconds
```

## Creating Custom Patterns

This codebase is designed to make it easy to add your own LED patterns. Follow these steps:

### Quick Start Guide

**Step 1: Write your pattern function**

Add your pattern function to the "CUSTOM PATTERNS SECTION" (~line 1002 in the .ino file):

```cpp
// Simple pattern example - lights all LEDs red
void myRedPattern() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(brightness, 0, 0));
  }
  strip.show();
  lights();  // Updates navigation lights (blinking)
}
```

**Step 2: Update NUM_PATTERNS**

Near the top of the file (~line 72), increment the pattern count:

```cpp
#define NUM_PATTERNS 5  // Changed from 4 to 5
```

**Step 3: Add to auto-cycle**

In the `loop()` function (~line 1196), add a new case:

```cpp
case 4:
  myRedPattern();
  break;
```

**Step 4: Add pattern name**

In the `subModeNames` array (~line 1182), add your pattern name:

```cpp
const char* subModeNames[] = {
  "FLIGHT",
  "SLOW RAINBOW",
  "FAST WHITE",
  "RAINBOW PROPS",
  "MY RED PATTERN"  // Add your name here
};
```

Done! Your pattern will now appear in the auto-cycle rotation.

### Pattern Function Conventions

#### Simple Patterns (Timer-based)

Use `void` return type for patterns that run for a fixed duration:

```cpp
void myPattern() {
  // Your animation code
  strip.show();
  lights();       // Blinking nav lights (colored patterns)
  // OR
  lights(false);  // Constant nav lights (white patterns)
}
```

The auto-cycle timer (`AUTO_CYCLE_DURATION`) will automatically advance after 10 seconds.

#### Complex Patterns (Completion-based)

Use `bool` return type for patterns that need to complete their own sequence:

```cpp
bool myComplexPattern() {
  static int phase = 0;
  static unsigned long phaseStart = 0;

  // Initialize on first call
  if (phaseStart == 0) {
    phaseStart = millis();
  }

  // Phase logic
  if (phase == 0) {
    // Do animation for 5 seconds
    if (millis() - phaseStart > 5000) {
      phase = 1;
      phaseStart = millis();
    }
  } else if (phase == 1) {
    // Do different animation for 3 seconds
    if (millis() - phaseStart > 3000) {
      phase = 0;
      phaseStart = 0;
      return true;  // Signal completion
    }
  }

  strip.show();
  lights();

  // Handle early exit
  if (gotBreak) {
    phase = 0;
    phaseStart = 0;
    gotBreak = false;
    return false;
  }

  return false;  // Still running
}
```

For completion-based patterns, call them like this in the switch statement:

```cpp
case 4:
  patternComplete = myComplexPattern();
  break;
```

### Example Patterns

The code includes two commented example patterns:

1. **Red Wipe Pattern** - Sequentially lights LEDs red
2. **Breathing Pattern** - Blue LEDs fade in and out

Uncomment these in the CUSTOM PATTERNS section to try them out!

### Best Practices

1. **Use static variables** for state that persists between calls
2. **Non-blocking code** - Avoid long `delay()` calls, use `millis()` instead
3. **Check gotBreak** - Allows early pattern exit if needed
4. **Power management** - Be mindful of total LED count and brightness
5. **Navigation lights** - Use `lights()` for colored patterns, `lights(false)` for white
6. **Clear LEDs** - Use `strip.clear()` at the start if needed

### Pattern Ideas

- **Chase patterns** - One or more LEDs moving around
- **Color wipes** - Fill from one end to the other
- **Breathing effects** - Fade brightness up and down
- **Twinkle/sparkle** - Random LEDs blinking
- **Color cycles** - Smooth transitions through color spectrum
- **Prop-specific** - Custom animations for individual props (see FLIGHT pattern)
- **Synchronized** - Props moving in coordinated patterns

### Hardware-Specific Patterns

This controller has 4 props + 1 tail. You can create patterns that treat each prop separately:

```cpp
void propSpinPattern() {
  static int angle = 0;

  // Clear all LEDs
  strip.clear();

  // Light one LED on each prop
  strip.setPixelColor(PROP1_START + (angle % 9), normColor);
  strip.setPixelColor(PROP2_START + (angle % 9), normColor);
  strip.setPixelColor(PROP3_START + (angle % 9), normColor);
  strip.setPixelColor(PROP4_START + (angle % 9), normColor);

  strip.show();
  lights(false);

  angle++;
  delay(50);
}
```

Prop/tail LED ranges:
- `PROP1_START` to `PROP1_END` (LEDs 0-8)
- `PROP2_START` to `PROP2_END` (LEDs 9-17)
- `PROP3_START` to `PROP3_END` (LEDs 18-26)
- `PROP4_START` to `PROP4_END` (LEDs 27-35)
- `TAIL_START` to `TAIL_END` (LEDs 36-40)

### Debugging Your Pattern

Monitor the serial output (115200 baud) to see:
- Pattern transitions
- Timing information
- Your own `Serial.print()` debug messages

Example:
```cpp
void myPattern() {
  static unsigned long lastDebug = 0;

  if (millis() - lastDebug > 1000) {
    Serial.println("My pattern is running!");
    lastDebug = millis();
  }

  // Pattern code...
}
```

## Power Considerations

### Current Draw
- **FLIGHT pattern:** ~0.5-1.0A (low power - sparse LEDs)
- **RAINBOW pattern:** ~2.0-2.5A (high power - all LEDs)
- **Other patterns:** ~1.0-1.5A

### Power Supply Recommendations
- **USB-C to USB-C:** Recommended (1.5-3A available)
- **USB-A to USB-C:** May brownout on high-power patterns (500-900mA limit)
- **External 5V supply to Vin:** Recommended for reliable operation (2A+)

### Brownout Protection
- 100ms delay between pattern transitions
- LEDs cleared and shown before pattern switch
- Allows power supply to stabilize

## Troubleshooting

### Board doesn't respond / No serial output
- BOOTSEL button code may be causing crashes
- Button functionality is currently disabled
- Use auto-cycle mode (works automatically on power-up)

### LEDs brownout on USB-A power
- Reduce brightness: `#define brightness 25`
- Use USB-C power or external 5V supply
- Consider removing high-power patterns from auto-cycle

### Compilation errors
- Ensure arduino-pico core version 5.4.3 or later
- Check Adafruit NeoPixel library is installed
- Verify FQBN: `rp2040:rp2040:seeed_xiao_rp2040`

### Props not animating
- Check WS2812B data connection on GPIO 4
- Verify LED count: 41 LEDs total
- Check serial debug output at 115200 baud

## Version History

### v1.2.0 Build 16 (Current) - Modular Pattern Architecture
- **Major refactoring for extensibility**
- Added comprehensive "Creating Custom Patterns" documentation
- Added CUSTOM PATTERNS section with templates and examples
- Introduced NUM_PATTERNS constant for easy pattern management
- Enhanced code comments with step-by-step pattern addition guide
- Added pattern templates: simple timer-based and complex completion-based
- Included example patterns: Red Wipe and Breathing Effect
- Updated README with detailed pattern creation tutorial
- Architecture optimized for easy customization and learning

### v1.1.0 Build 15 - Simplified and Documented
- Removed all Christmas pattern functions
- Removed non-functional button handling code
- Removed unused modes (kept only auto-cycle)
- Added comprehensive inline documentation
- Enhanced header comments with design patterns
- Documented FLIGHT pattern phases and timing
- Explained non-linear motion implementation
- Code reorganized for clarity

### v1.0.3 Build 14
- Restored original rainbow pattern (all 41 LEDs)
- Restored brightness to 50
- Auto-cycle: FLIGHT, SLOW RAINBOW, FAST WHITE, RAINBOW PROPS

### v1.0.3 Build 13
- Modified rainbow to chase pattern (8 LEDs)
- Reduced brightness to 25
- Power optimization attempts

### v1.0.3 Build 12
- Added 100ms delay between pattern transitions
- Power supply stabilization

### v1.0.3 Build 10
- Reverted Xmas integration from auto-cycle
- Disabled BOOTSEL button (stability)
- Faster tail in CONVENTIONAL phase (30ms)

### v1.0.3 Build 7
- Non-linear prop acceleration/deceleration
- Very slow tail during LIFT and GROUND_PAUSE
- Exponential speed curves

### v1.0.2 Build 6
- Much faster props (10ms minimum)
- 3-second holds at max speed
- Parked props 2&4 use LEDs 8&4

### v1.0.1 Build 5
- Faster timings (5s LIFT, 8s TRANSITION_IN, etc.)
- Faster prop speeds (20ms minimum)
- 5-second ground pause added

### v1.0.0 Build 1
- Initial release with FLIGHT pattern
- Auto-cycle mode
- Christmas patterns

## Technical Details

### Finite State Machine
FLIGHT pattern uses completion flag instead of timer:
- Returns `true` when complete
- Auto-cycle waits for signal before advancing
- Prevents interruption mid-sequence

### Color Management
- Navigation lights change based on pattern color
- White patterns: constant lights
- Colored patterns: blinking lights

### Debug Output
Serial monitor (115200 baud) shows:
- Version and build info
- Phase transitions
- Speed parameters
- Elapsed time
- Pattern changes

## Credits

Developed for Beta Alia eVTOL scale model LED animation.

## License

This project is provided as-is for personal and educational use.
