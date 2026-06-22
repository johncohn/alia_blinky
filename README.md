# Alia 4 - Beta Alia eVTOL LED Animation Controller

LED animation controller for a scale model of the Beta Alia eVTOL aircraft. Features realistic flight simulation with dynamic prop and tail animations.

## Key Documents

| Document | Description | QR Code |
|----------|-------------|---------|
| [TUTORIAL.md](TUTORIAL.md) | Complete setup and customization guide | <img src="images/qr_tutorial.png" width="80"> |
| [SOLDER.md](SOLDER.md) | Photo guide: soldering & gluing the PCB | <img src="images/qr_solder.png" width="80"> |

**Version:** 1.2.0
**Build:** 16
**Hardware:** Supports multiple boards (auto-detected) + 41 WS2812B LEDs
- Seeed XIAO RP2040
- Adafruit QT Py RP2040
- Seeed XIAO ESP32-S3
- Adafruit QT Py ESP32-S3 (no PSRAM)

## Features

Auto-cycles through 4 animation patterns:

1. **FLIGHT** (~36s) - Realistic eVTOL flight simulation with 5 phases
   - Lift, transition in, conventional flight, transition out, landing
   - Non-linear prop acceleration/deceleration
   - Counter-rotating props, variable tail speed
2. **SLOW RAINBOW** (10s) - Full rainbow color cycle
3. **FAST WHITE** (10s) - Running white lights with sine wave
4. **RAINBOW PROPS** (10s) - Theater chase rainbow effect

## Quick Start

### Requirements
- Arduino IDE 2.x (or Arduino CLI)
- One of the supported boards (see Hardware section below)
- Adafruit NeoPixel library

### Setup (Arduino IDE)

1. **Install Board Support**

   **For RP2040 boards:**
   - Tools → Board → Boards Manager
   - Search "rp2040"
   - Install "Raspberry Pi Pico/RP2040" (v5.4.3+)
   - Select your board:
     - Tools → Board → Seeed XIAO RP2040, or
     - Tools → Board → Adafruit QT Py RP2040

   **For ESP32-S3 boards:**
   - Tools → Board → Boards Manager
   - Search "esp32"
   - Install "esp32 by Espressif Systems" (v3.3.3+)
   - Select your board:
     - Tools → Board → esp32 → XIAO_ESP32S3, or
     - Tools → Board → esp32 → Adafruit QT Py ESP32-S3 no psram

2. **Install Library**
   - Tools → Manage Libraries
   - Search "Adafruit NeoPixel"
   - Install (v1.15.2+)

3. **Upload**
   - Download/clone this repo
   - Open `Alia_blinky_esp32.ino`
   - Select your board (step 1) and port
   - Click Upload (→)

**CLI Alternative:**

For RP2040:
```bash
arduino-cli core install rp2040:rp2040
arduino-cli lib install "Adafruit NeoPixel"
# Seeed XIAO RP2040:
arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 Alia_blinky_esp32.ino
arduino-cli upload -p /dev/cu.usbmodem* --fqbn rp2040:rp2040:seeed_xiao_rp2040 Alia_blinky_esp32.ino
# Adafruit QT Py RP2040:
arduino-cli compile --fqbn rp2040:rp2040:adafruit_qtpy Alia_blinky_esp32.ino
arduino-cli upload -p /dev/cu.usbmodem* --fqbn rp2040:rp2040:adafruit_qtpy Alia_blinky_esp32.ino
```

For ESP32-S3:
```bash
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit NeoPixel"
# Seeed XIAO ESP32-S3:
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 Alia_blinky_esp32.ino
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:XIAO_ESP32S3 Alia_blinky_esp32.ino
# Adafruit QT Py ESP32-S3:
arduino-cli compile --fqbn esp32:esp32:adafruit_qtpy_esp32s3_nopsram Alia_blinky_esp32.ino
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:adafruit_qtpy_esp32s3_nopsram Alia_blinky_esp32.ino
```

## Hardware

### LED Layout (41 WS2812B LEDs)
- **Prop 1-4:** 9 LEDs each (0-8, 9-17, 18-26, 27-35)
- **Tail:** 5 LEDs (36-40)

### Pin Assignments

The code automatically detects your board and uses the correct GPIO pins. All boards use the same **physical pin positions** (TX, RX, SCK, MISO):

| Board | Starboard (TX) | Port (RX) | Nose (SCK) | WS2812B (MISO) |
|-------|----------------|-----------|------------|----------------|
| **Seeed XIAO RP2040** | GPIO 0 | GPIO 1 | GPIO 2 | GPIO 4 |
| **Adafruit QT Py RP2040** | GPIO 20 | GPIO 5 | GPIO 6 | GPIO 4 |
| **Seeed XIAO ESP32-S3** | GPIO 43 | GPIO 44 | GPIO 8 | GPIO 7 |
| **Adafruit QT Py ESP32-S3** | GPIO 5 | GPIO 16 | GPIO 36 | GPIO 37 |

**Physical Connections:**
- TX pin → Starboard navigation light (green)
- RX pin → Port navigation light (red)
- SCK pin → Nose navigation light (white)
- MISO pin → WS2812B data line

### PCB Design Files
`alia_blinky_kicad.zip` contains complete KiCad project files for a custom PCB. The same PCB works with all supported boards - just swap the microcontroller!

## Customization

### Add Your Own Pattern

See [TUTORIAL.md](TUTORIAL.md) for complete instructions. Quick summary:

1. Write pattern function in CUSTOM PATTERNS section (~line 1002)
2. Update `NUM_PATTERNS` constant (line 72)
3. Add case to switch statement (line 1196)
4. Add name to `subModeNames` array (line 1182)

Example:
```cpp
void myPattern() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));  // Red
  }
  strip.show();
  lights();
}
```

### Adjust Settings

**Brightness** (line 47):
```cpp
#define brightness 50  // 0-255 (lower for USB-A power)
```

**Pattern Duration** (line 74):
```cpp
const int AUTO_CYCLE_DURATION = 10000;  // milliseconds
```

## Power Considerations

- **USB-C:** 2-3A (recommended)
- **USB-A:** 500mA (reduce brightness to 25)
- **External 5V:** 2A+ (best)

Pattern power draw:
- FLIGHT: 0.5-1.0A (sparse LEDs)
- RAINBOW: 2.0-2.5A (all LEDs)

## Troubleshooting

**LEDs brownout on USB-A power:**
- Reduce brightness to 25
- Use USB-C or external 5V supply
- ESP32-S3 draws more power than RP2040 - use high-power USB port

**Compilation errors:**
- For RP2040: Ensure arduino-pico core v5.4.3+
- For ESP32: Ensure esp32 core v3.3.3+
- Verify Adafruit NeoPixel library installed
- Check you selected the correct board in Tools → Board

**LEDs not working:**
- Check serial monitor (115200 baud) - should show detected board type
- Verify correct board selected in Arduino IDE
- Check WS2812B connection (MISO pin - see Pin Assignments table)
- Verify 41 LEDs total
- ESP32-S3: Ensure "no psram" variant selected for QT Py

**Wrong board detected:**
- Serial monitor shows detected board on startup
- Ensure correct board selected in Tools → Board menu
- Code auto-detects based on compiler flags

## Documentation

- **[TUTORIAL.md](TUTORIAL.md)** - Complete step-by-step guide
- **[SOLDER.md](SOLDER.md)** - Photo guide for soldering the microcontroller and gluing the diffuser covers
- **[LICENSE](LICENSE)** - MIT License
- **In-code comments** - Extensive documentation throughout

## Credits

**Created by:** John Cohn, PhD
**Original Date:** December 2024
**Updated:** January 2025 (Multi-board support added)
**Contact:** jcohn@beta.team | johncohnvt@gmail.com

Developed with major assistance from Anthropic Claude.

Please reach out if you have comments, questions, or suggestions!

## License

MIT License - Copyright (c) 2024 John Cohn

See [LICENSE](LICENSE) file for full text.
