# Quickstart: Write Your Own LED Pattern

Get the software running, then add a custom pattern in a few minutes. For full details (all boards, wiring, troubleshooting) see [README.md](README.md) or [TUTORIAL.md](TUTORIAL.md).

## 1. Load the Software

1. Get the code: go to [github.com/johncohn/alia_blinky](https://github.com/johncohn/alia_blinky) → green **Code** button → **Download ZIP**, then unzip it (or `git clone https://github.com/johncohn/alia_blinky.git` if you use git)
2. Install **Arduino IDE 2.x** ([arduino.cc](https://www.arduino.cc/en/software))
3. **Tools → Board → Boards Manager** → install the core for your board:
   - RP2040 boards: search `rp2040`, install "Raspberry Pi Pico/RP2040"
   - ESP32-S3 boards: search `esp32`, install "esp32 by Espressif Systems"
4. **Tools → Manage Libraries** → install **Adafruit NeoPixel**
5. In Arduino IDE, **File → Open** → select `Alia_blinky_esp32.ino` from the folder you unzipped in step 1, select your board under **Tools → Board**, then click **Upload**

That's it — the board should start cycling through the built-in patterns.

## 2. The Recipe for Adding a Pattern

Every custom pattern needs 4 edits to `Alia_blinky_esp32.ino`:

| # | What | Where |
|---|------|-------|
| 1 | Write your pattern function | `CUSTOM PATTERNS SECTION` (~line 1075) |
| 2 | Add one to `NUM_PATTERNS` | ~line 91 |
| 3 | Add a `case` for it in the `switch` | `loop()` (~line 1268) |
| 4 | Add its name to `subModeNames[]` | ~line 1254 |

Then **Upload**, open **Tools → Serial Monitor** (115200 baud), and watch — it prints the pattern name each time the auto-cycle switches, so you can confirm yours is running.

Each example below is copy-paste ready. Just drop the function in, then add the matching `case` line and name.

---

## Examples

### Red, White & Blue

Marching stripes down the whole string.

```cpp
void redWhiteBluePattern() {
  static int offset = 0;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 100) {
    for (int i = 0; i < LED_COUNT; i++) {
      int stripe = (i + offset) % 3;
      if (stripe == 0) strip.setPixelColor(i, strip.Color(brightness, 0, 0));         // Red
      else if (stripe == 1) strip.setPixelColor(i, strip.Color(brightness, brightness, brightness)); // White
      else strip.setPixelColor(i, strip.Color(0, 0, brightness));                     // Blue
    }
    strip.show();
    lights();
    offset++;
    lastUpdate = millis();
  }
}
```

Add to `switch`: `case 4: redWhiteBluePattern(); break;`
Add to names: `"RED WHITE BLUE"`

### Dot Chase

One white LED travels down the entire string (all 4 props, then the tail) and loops.

```cpp
void dotChasePattern() {
  static int currentLED = 0;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 30) {
    strip.clear();
    strip.setPixelColor(currentLED, strip.Color(brightness, brightness, brightness));
    strip.show();
    lights();

    currentLED++;
    if (currentLED >= LED_COUNT) currentLED = 0;
    lastUpdate = millis();
  }
}
```

Add to `switch`: `case 5: dotChasePattern(); break;`
Add to names: `"DOT CHASE"`

### Rainbow Props

Each prop is a solid color; all 4 slowly cycle through the rainbow, offset so no two props ever match.

```cpp
void rainbowPropsSolidPattern() {
  static uint16_t baseHue = 0;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 20) {
    uint16_t hueStep = 65536 / 4;  // split the color wheel 4 ways

    for (int prop = 0; prop < 4; prop++) {
      uint32_t color = strip.gamma32(strip.ColorHSV(baseHue + (prop * hueStep)));
      for (int led = 0; led < 9; led++) {
        strip.setPixelColor((prop * 9) + led, color);
      }
    }
    strip.show();
    lights();
    baseHue += 100;
    lastUpdate = millis();
  }
}
```

Add to `switch`: `case 6: rainbowPropsSolidPattern(); break;`
Add to names: `"RAINBOW PROPS SOLID"`

### Bonus: Sparkle

Random LEDs twinkle white against a dark background.

```cpp
void sparklePattern() {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 50) {
    strip.clear();
    for (int i = 0; i < 6; i++) {  // 6 sparkles at a time
      strip.setPixelColor(random(LED_COUNT), strip.Color(brightness, brightness, brightness));
    }
    strip.show();
    lights();
    lastUpdate = millis();
  }
}
```

Add to `switch`: `case 7: sparklePattern(); break;`
Add to names: `"SPARKLE"`

### Bonus: Fire

Random orange/red flicker across all LEDs.

```cpp
void firePattern() {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 40) {
    for (int i = 0; i < LED_COUNT; i++) {
      int flicker = random(brightness / 2, brightness);
      strip.setPixelColor(i, strip.Color(flicker, flicker / 4, 0));
    }
    strip.show();
    lights();
    lastUpdate = millis();
  }
}
```

Add to `switch`: `case 8: firePattern(); break;`
Add to names: `"FIRE"`

---

**Tip:** only add the patterns you actually want — each one needs its own `case` number (0, 1, 2, 3... in order, no gaps) and `NUM_PATTERNS` must equal the total count. If you add just one from above, `NUM_PATTERNS` becomes `5` and its case number is `4`.

## See Also

| | |
|---|---|
| [README.md](README.md) | Hardware, wiring, full feature list |
| [TUTORIAL.md](TUTORIAL.md) | Full walkthrough — removing patterns, advanced tips, debugging |
| [SOLDER.md](SOLDER.md) | Photo guide for soldering and gluing the PCB |
