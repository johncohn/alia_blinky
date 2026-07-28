# Alia 4 - Beta Alia eVTOL LED Animation Controller

LED animation controller for a scale model of the Beta Alia eVTOL aircraft. Features realistic flight simulation with dynamic prop and tail animations.

## Key Documents

| Document | Description | QR Code |
|----------|-------------|---------|
| [GitHub Repo](https://github.com/johncohn/alia_blinky) (https://github.com/johncohn/alia_blinky) | Top-level repository — browse, download, or fork | <img src="images/qr_repo.png" width="120"> |
| [TUTORIAL.md](TUTORIAL.md) (https://github.com/johncohn/alia_blinky/blob/main/TUTORIAL.md) | Complete setup and customization guide | <img src="images/qr_tutorial.png" width="120"> |
| [SOLDER.md](SOLDER.md) (https://github.com/johncohn/alia_blinky/blob/main/SOLDER.md) | Photo guide: soldering & gluing the PCB | <img src="images/qr_solder.png" width="120"> |

**Version:** 1.2.0
**Build:** 16
**Hardware:** Supports multiple boards (auto-detected) + 41 WS2812B LEDs
- Seeed XIAO RP2040
- Adafruit QT Py RP2040
- Seeed XIAO ESP32-S3
- Adafruit QT Py ESP32-S3 (no PSRAM)

## Features

Auto-cycles through 5 animation patterns:

1. **FLIGHT** (~36s) - Realistic eVTOL flight simulation with 5 phases
   - Lift, transition in, conventional flight, transition out, landing
   - Non-linear prop acceleration/deceleration
   - Counter-rotating props, variable tail speed
2. **SLOW RAINBOW** (10s) - Full rainbow color cycle
3. **FAST WHITE** (10s) - Running white lights with sine wave
4. **RAINBOW PROPS** (10s) - Theater chase rainbow effect
5. **CUSTOM** (10s) - Your editable pattern slot (see Customization below)

## Quick Start

### Requirements
- Arduino IDE 2.x
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
   - Open `alia_blinky.ino`
   - Select your board (step 1) and port
   - Click Upload (→)

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

The easiest option: the auto-cycle already includes an editable **CUSTOM** slot, `customPattern()` (~line 1073) — just replace its body with one of TUTORIAL.md's sample patterns and re-upload. No other changes needed.

See [TUTORIAL.md](TUTORIAL.md) for that walkthrough, several ready-to-paste samples, and how to add a whole *additional* pattern (which does require 4 edits):

1. Write pattern function in CUSTOM PATTERNS section (~line 1062)
2. Update `NUM_PATTERNS` constant (line 78)
3. Add case to switch statement (line 1194)
4. Add name to `subModeNames` array (line 1179)

### Adjust Settings

**Brightness** (line 83):
```cpp
#define brightness 50  // 0-255 (lower for USB-A power)
```

**Pattern Duration** (line 134):
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
