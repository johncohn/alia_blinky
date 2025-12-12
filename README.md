# Alia 4 - Beta Alia eVTOL LED Animation Controller

LED animation controller for a scale model of the Beta Alia electric vertical takeoff and landing (eVTOL) aircraft. Features realistic flight simulation with dynamic prop and tail animations, plus additional color patterns.

**✨ Easy to customize!** This codebase is designed as a learning resource with comprehensive tutorials for creating your own LED patterns.

**Version:** 1.2.0 (Modular Pattern Architecture)
**Build:** 16
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

---

## 🚀 Getting Started Tutorial

This tutorial will guide you from zero to creating your own custom LED patterns!

### Part 1: Setting Up Your Development Environment

#### Step 1: Install Arduino IDE

1. **Download Arduino IDE 2.x** from https://www.arduino.cc/en/software
   - Choose your operating system (Windows, macOS, or Linux)
   - Install the downloaded application

2. **Launch Arduino IDE**
   - Open the application after installation

#### Step 2: Install Seeed XIAO RP2040 Board Support

1. **Open Boards Manager:**
   - Click the Boards Manager icon (📋) in the left sidebar
   - Or go to: Tools → Board → Boards Manager

2. **Add RP2040 Board Package:**
   - In the search box, type: `rp2040`
   - Find **"Raspberry Pi Pico/RP2040"** by Earle F. Philhower, III
   - Click **Install** (version 5.4.3 or later recommended)
   - Wait for installation to complete (may take a few minutes)

3. **Select Your Board:**
   - Go to: Tools → Board → Raspberry Pi RP2040 Boards
   - Select: **Seeed XIAO RP2040**

#### Step 3: Install Required Libraries

1. **Open Library Manager:**
   - Click the Library Manager icon (📚) in the left sidebar
   - Or go to: Tools → Manage Libraries

2. **Install Adafruit NeoPixel:**
   - In the search box, type: `Adafruit NeoPixel`
   - Find **"Adafruit NeoPixel"** by Adafruit
   - Click **Install** (version 1.15.2 or later)
   - Click "Install All" if prompted for dependencies

#### Step 4: Download the Alia 4 Code

1. **Clone or Download from GitHub:**
   ```bash
   git clone https://github.com/johncohn/alia_blinky.git
   ```

   Or download ZIP:
   - Visit: https://github.com/johncohn/alia_blinky
   - Click the green "Code" button
   - Select "Download ZIP"
   - Extract to a folder on your computer

2. **Open the Project:**
   - In Arduino IDE: File → Open
   - Navigate to the folder
   - Open `Alia_4_v9.ino`

#### Step 5: Upload to Your Board

1. **Put Board in Bootloader Mode:**
   - **Method 1 (Recommended):** Double-tap the RESET button quickly
   - **Method 2:** Hold BOOT button, press and release RESET, release BOOT
   - A drive named **RPI-RP2** should appear on your computer

2. **Upload the Code:**
   - In Arduino IDE, click the Upload button (→)
   - Or go to: Sketch → Upload
   - The code will compile and upload automatically
   - Board will reboot and start running the animations

3. **Verify It Works:**
   - Open Serial Monitor: Tools → Serial Monitor
   - Set baud rate to **115200**
   - You should see version info and pattern transitions

---

### Part 2: Adding Your First Custom Pattern

Let's create a simple pattern that makes all LEDs pulse red!

#### Step 1: Write Your Pattern Function

1. **Find the CUSTOM PATTERNS section** (around line 1002)
2. **Add this code:**

```cpp
// RED PULSE - All LEDs pulse red
void redPulsePattern() {
  static int brightness_val = 0;
  static int direction = 5;
  static unsigned long lastUpdate = 0;

  // Update every 20ms for smooth animation
  if (millis() - lastUpdate > 20) {
    // Increase or decrease brightness
    brightness_val += direction;

    // Reverse direction at limits
    if (brightness_val >= 255) {
      brightness_val = 255;
      direction = -5;
    } else if (brightness_val <= 0) {
      brightness_val = 0;
      direction = 5;
    }

    // Set all LEDs to red with current brightness
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(brightness_val, 0, 0));
    }

    strip.show();
    lights();  // Blink navigation lights

    lastUpdate = millis();
  }
}
```

#### Step 2: Update Pattern Count

1. **Find line 72** (near the top of the file)
2. **Change:**
   ```cpp
   #define NUM_PATTERNS 4  // Total number of patterns
   ```
   **To:**
   ```cpp
   #define NUM_PATTERNS 5  // Total number of patterns (added RED PULSE)
   ```

#### Step 3: Add to Auto-Cycle

1. **Find the pattern execution switch** (around line 1196)
2. **Add a new case after case 3:**

```cpp
    case 3:
      // RAINBOW PROPS - Theater chase rainbow effect (10 seconds)
      // Uses timer (AUTO_CYCLE_DURATION)
      theaterChaseRainbow(waitTime);
      break;

    case 4:
      // RED PULSE - All LEDs pulse red (10 seconds)
      // Uses timer (AUTO_CYCLE_DURATION)
      redPulsePattern();
      break;
```

#### Step 4: Add Pattern Name for Debug Output

1. **Find the subModeNames array** (around line 1182)
2. **Add your pattern name:**

```cpp
const char* subModeNames[] = {
  "FLIGHT",         // Pattern 0
  "SLOW RAINBOW",   // Pattern 1
  "FAST WHITE",     // Pattern 2
  "RAINBOW PROPS",  // Pattern 3
  "RED PULSE"       // Pattern 4 - YOUR NEW PATTERN!
};
```

#### Step 5: Upload and Test!

1. **Save the file:** Ctrl+S (Windows/Linux) or Cmd+S (macOS)
2. **Put board in bootloader mode** (double-tap RESET)
3. **Click Upload** (→)
4. **Watch your new pattern!**
   - Wait through FLIGHT (~36s), RAINBOW (10s), WHITE (10s), RAINBOW PROPS (10s)
   - Your RED PULSE pattern will appear after ~66 seconds!

---

### Part 3: Removing Patterns from Auto-Cycle

Want to remove patterns you don't like? Here's how:

#### Option A: Remove a Single Pattern (e.g., Remove SLOW RAINBOW)

1. **Update NUM_PATTERNS** (line 72):
   ```cpp
   #define NUM_PATTERNS 4  // Was 5, now 4
   ```

2. **Remove the pattern case** (around line 1203):
   - Delete or comment out the entire case for the pattern:
   ```cpp
   // case 1:
   //   // SLOW RAINBOW - Full rainbow cycle across all 41 LEDs (10 seconds)
   //   // Uses timer (AUTO_CYCLE_DURATION)
   //   rainbow(waitTime / 5);
   //   break;
   ```

3. **Renumber remaining cases** (important!):
   ```cpp
   case 0:  // FLIGHT (unchanged)
     patternComplete = flightPattern();
     break;

   case 1:  // Was case 2, now case 1
     RunningLights(0xff, 0xff, 0xff, 50);
     break;

   case 2:  // Was case 3, now case 2
     theaterChaseRainbow(waitTime);
     break;

   case 3:  // Was case 4, now case 3
     redPulsePattern();
     break;
   ```

4. **Update pattern names array** (line 1182):
   ```cpp
   const char* subModeNames[] = {
     "FLIGHT",
     "FAST WHITE",      // Removed "SLOW RAINBOW"
     "RAINBOW PROPS",
     "RED PULSE"
   };
   ```

#### Option B: Remove ALL Patterns (Only Keep FLIGHT)

1. **Update NUM_PATTERNS** (line 72):
   ```cpp
   #define NUM_PATTERNS 1  // Only FLIGHT pattern
   ```

2. **Remove all other cases** (keep only case 0):
   ```cpp
   switch (autoCycleSubMode) {
     case 0:
       // FLIGHT - Complete eVTOL flight simulation (~36 seconds)
       // Uses completion flag (returns true when done)
       patternComplete = flightPattern();
       break;
   }
   ```

3. **Update pattern names array** (line 1182):
   ```cpp
   const char* subModeNames[] = {
     "FLIGHT"  // Only pattern
   };
   ```

4. **Result:** FLIGHT pattern will run continuously, restarting after completion!

#### Option C: Replace All Patterns with Your Own

1. **Set NUM_PATTERNS** to the number of your custom patterns
2. **Replace all cases** with your pattern functions
3. **Update pattern names** array
4. **Done!** You now have a completely custom LED controller

---

### Part 4: Pattern Tips and Tricks

#### Making Patterns Run Longer or Shorter

**For timer-based patterns**, change `AUTO_CYCLE_DURATION` (line 74):
```cpp
const int AUTO_CYCLE_DURATION = 15000;  // 15 seconds instead of 10
```

**For FLIGHT pattern**, modify phase durations (around line 641):
```cpp
const int liftTime = 10000;  // 10 seconds instead of 5
```

#### Creating Prop-Specific Patterns

Control individual props using these constants:
```cpp
void spinPropsClockwise() {
  static int angle = 0;

  strip.clear();

  // All props spin the same direction
  for (int prop = 0; prop < 4; prop++) {
    int ledStart = prop * 9;  // Props are 9 LEDs each
    strip.setPixelColor(ledStart + (angle % 9), normColor);
  }

  strip.show();
  lights(false);

  angle++;
  delay(50);
}
```

#### Using Different Colors

```cpp
// RGB color values (0-255 for each)
strip.Color(255, 0, 0)      // Pure red
strip.Color(0, 255, 0)      // Pure green
strip.Color(0, 0, 255)      // Pure blue
strip.Color(255, 255, 0)    // Yellow
strip.Color(255, 0, 255)    // Magenta
strip.Color(0, 255, 255)    // Cyan
strip.Color(255, 255, 255)  // White
strip.Color(128, 0, 128)    // Purple (lower brightness)
```

#### Debugging Your Pattern

Add serial debug output:
```cpp
void myPattern() {
  static unsigned long lastDebug = 0;

  if (millis() - lastDebug > 1000) {
    Serial.println("My pattern is running!");
    Serial.print("Current animation value: ");
    Serial.println(someValue);
    lastDebug = millis();
  }

  // Pattern code...
}
```

#### Common Mistakes to Avoid

1. **Forgetting to update NUM_PATTERNS** - Pattern won't appear in cycle
2. **Not renumbering cases** - Patterns will play in wrong order
3. **Missing strip.show()** - LEDs won't update
4. **Long delay() calls** - Makes animations choppy, use millis() instead
5. **Not checking gotBreak** - Pattern can't be interrupted

---

### Part 5: Going Further

#### More Pattern Ideas

- **Chase patterns:** LEDs moving in sequence
- **Fire effect:** Random orange/red flickering
- **Sparkle:** Random LEDs twinkling
- **Color wheel:** Cycle through rainbow
- **Strobe:** Rapid flashing (be careful with brightness!)
- **Comet:** Trailing LED with fade
- **Fill patterns:** Progressively fill/empty
- **Wave patterns:** Sine wave brightness across LEDs

#### Study the FLIGHT Pattern

The FLIGHT pattern (line 593) is a great example of:
- Multi-phase animations
- Non-linear motion (exponential curves)
- Completion-based timing
- State management with static variables

Read through it to learn advanced techniques!

#### Share Your Patterns

Create cool patterns? Share them!
- Fork the GitHub repo
- Add your patterns
- Submit a pull request
- Help others learn!

---

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
