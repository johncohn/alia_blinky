# Alia 4 LED Controller - Complete Tutorial

This comprehensive tutorial will guide you from setup to creating custom LED patterns.

## Part 1: Setting Up Your Development Environment

### Step 1: Install Arduino IDE

1. **Download Arduino IDE 2.x** from https://www.arduino.cc/en/software
   - Choose your operating system (Windows, macOS, or Linux)
   - Install the downloaded application

2. **Launch Arduino IDE**
   - Open the application after installation

**CLI Alternative:**
```bash
# Install Arduino CLI
brew install arduino-cli  # macOS
# Or download from: https://arduino.github.io/arduino-cli/
```

### Step 2: Install Seeed XIAO RP2040 Board Support

**Arduino IDE:**
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

**CLI Alternative:**
```bash
arduino-cli core install rp2040:rp2040
```

### Step 3: Install Required Libraries

**Arduino IDE:**
1. **Open Library Manager:**
   - Click the Library Manager icon (📚) in the left sidebar
   - Or go to: Tools → Manage Libraries

2. **Install Adafruit NeoPixel:**
   - In the search box, type: `Adafruit NeoPixel`
   - Find **"Adafruit NeoPixel"** by Adafruit
   - Click **Install** (version 1.15.2 or later)
   - Click "Install All" if prompted for dependencies

**CLI Alternative:**
```bash
arduino-cli lib install "Adafruit NeoPixel"
```

### Step 4: Download the Alia 4 Code

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
   - **Arduino IDE:** File → Open → Navigate to `Alia_4_v9.ino`
   - **CLI:** Navigate to the directory in terminal

### Step 5: Upload to Your Board

1. **Put Board in Bootloader Mode:**
   - **Method 1 (Recommended):** Double-tap the RESET button quickly
   - **Method 2:** Hold BOOT button, press and release RESET, release BOOT
   - A drive named **RPI-RP2** should appear on your computer

2. **Upload the Code:**

   **Arduino IDE:**
   - Click the Upload button (→)
   - Or go to: Sketch → Upload
   - The code will compile and upload automatically
   - Board will reboot and start running the animations

   **CLI Alternative:**
   ```bash
   arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 Alia_4_v9.ino
   arduino-cli upload -p /dev/cu.usbmodem* --fqbn rp2040:rp2040:seeed_xiao_rp2040 Alia_4_v9.ino
   ```

3. **Verify It Works:**
   - **Arduino IDE:** Tools → Serial Monitor (set baud rate to 115200)
   - **CLI:** `screen /dev/cu.usbmodem* 115200` (macOS/Linux)
   - You should see version info and pattern transitions

---

## Part 2: Adding Your First Custom Pattern

Let's create a simple pattern that makes all LEDs pulse red!

### Step 1: Write Your Pattern Function

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

### Step 2: Update Pattern Count

1. **Find line 72** (near the top of the file)
2. **Change:**
   ```cpp
   #define NUM_PATTERNS 4  // Total number of patterns
   ```
   **To:**
   ```cpp
   #define NUM_PATTERNS 5  // Total number of patterns (added RED PULSE)
   ```

### Step 3: Add to Auto-Cycle

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

### Step 4: Add Pattern Name for Debug Output

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

### Step 5: Upload and Test!

1. **Save the file:** Ctrl+S (Windows/Linux) or Cmd+S (macOS)
2. **Put board in bootloader mode** (double-tap RESET)
3. **Click Upload** (→) in Arduino IDE
4. **Watch your new pattern!**
   - Wait through FLIGHT (~36s), RAINBOW (10s), WHITE (10s), RAINBOW PROPS (10s)
   - Your RED PULSE pattern will appear after ~66 seconds!

---

## Part 3: Removing Patterns from Auto-Cycle

Want to remove patterns you don't like? Here's how:

### Option A: Remove a Single Pattern (e.g., Remove SLOW RAINBOW)

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

### Option B: Remove ALL Patterns (Only Keep FLIGHT)

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

### Option C: Replace All Patterns with Your Own

1. **Set NUM_PATTERNS** to the number of your custom patterns
2. **Replace all cases** with your pattern functions
3. **Update pattern names** array
4. **Done!** You now have a completely custom LED controller

---

## Part 4: Pattern Tips and Tricks

### Making Patterns Run Longer or Shorter

**For timer-based patterns**, change `AUTO_CYCLE_DURATION` (line 74):
```cpp
const int AUTO_CYCLE_DURATION = 15000;  // 15 seconds instead of 10
```

**For FLIGHT pattern**, modify phase durations (around line 641):
```cpp
const int liftTime = 10000;  // 10 seconds instead of 5
```

### Creating Prop-Specific Patterns

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

### Using Different Colors

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

### Debugging Your Pattern

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

### Common Mistakes to Avoid

1. **Forgetting to update NUM_PATTERNS** - Pattern won't appear in cycle
2. **Not renumbering cases** - Patterns will play in wrong order
3. **Missing strip.show()** - LEDs won't update
4. **Long delay() calls** - Makes animations choppy, use millis() instead
5. **Not checking gotBreak** - Pattern can't be interrupted

---

## Part 5: Going Further

### More Pattern Ideas

- **Chase patterns:** LEDs moving in sequence
- **Fire effect:** Random orange/red flickering
- **Sparkle:** Random LEDs twinkling
- **Color wheel:** Cycle through rainbow
- **Strobe:** Rapid flashing (be careful with brightness!)
- **Comet:** Trailing LED with fade
- **Fill patterns:** Progressively fill/empty
- **Wave patterns:** Sine wave brightness across LEDs

### Study the FLIGHT Pattern

The FLIGHT pattern (line 593) is a great example of:
- Multi-phase animations
- Non-linear motion (exponential curves)
- Completion-based timing
- State management with static variables

Read through it to learn advanced techniques!

### Share Your Patterns

Create cool patterns? Share them!
- Fork the GitHub repo
- Add your patterns
- Submit a pull request
- Help others learn!
