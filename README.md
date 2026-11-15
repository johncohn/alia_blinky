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

1. **Get the code**
   - Open [alia_blinky.ino](https://raw.githubusercontent.com/johncohn/alia_blinky/main/alia_blinky.ino) → right-click → **Save As** → save it as `alia_blinky.ino`
   - (Want the PCB design files or soldering photos too? Grab the [full repo](https://github.com/johncohn/alia_blinky) instead — see Documentation below.)

2. **Install Board Support**

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

3. **Install Library**
   - Tools → Manage Libraries
   - Search "Adafruit NeoPixel"
   - Install (v1.15.2+)

4. **Upload**
   - Open the `alia_blinky.ino` you saved in step 1
   - Select your board (step 2) and port
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

## Microphone Test (Sound Board)

The newer "sound" version of the PCB adds an analog MEMS microphone (MSM381ACP003) wired to the **A3** pad. [`alia_mic_test/alia_mic_test.ino`](alia_mic_test/alia_mic_test.ino) is a separate sketch for checking whether the board can hear claps, talking, or music. It isn't hi-fi, just "is there a signal above the background noise".

**Use it:**
1. Open `alia_mic_test/alia_mic_test.ino`, select your board and port (same as above), and upload.
2. Open **Tools → Serial Plotter** and set the baud rate to **115200**. All other debug text is turned off so the plotter only gets numbers.
3. Stay quiet for about 5 seconds while it learns the room's noise floor, then clap or talk.

**How it detects sound (STA/LTA):** every 20 ms it measures loudness as the RMS of the mic signal. It keeps two moving averages of that loudness: a short-term one that follows syllables and claps, and a long-term one that tracks the background noise. Sound counts as detected when the short-term average rises above the threshold, which always sits just above the noise floor. While sound is detected, the noise floor updates much more slowly, so talking doesn't push the threshold up.

| Plot trace | Meaning |
|------------|---------|
| `sta` | Smoothed loudness (about 0.1 s average) |
| `lta` | Learned noise floor (about 5 s average) |
| `thresh` | Current trigger level: `lta × TRIGGER_RATIO + MIN_MARGIN` |
| `detect` | Steps up while sound is detected |
| `level` | Raw 20 ms loudness (off by default; set `PLOT_LEVEL true` to show it) |

**On the board:** the nose and wingtip lights stay on while sound is detected, the prop LEDs act as a level meter (green → yellow → red), and one dim tail LED shows the sketch is running.

**Tuning** (settings at the top of the sketch):
- `TRIGGER_RATIO` (default 1.2) is the main sensitivity setting. Lower it to make detection easier, and raise it if the lights come on in a quiet room. Keep `RELEASE_RATIO` (1.05) below it.
- `STA_TIME_MS` (100) controls how quickly the smoothed trace reacts. Around 50 shows separate syllables; around 200 shows whole words.
- `LED_METER false` turns the LEDs off. The mic is powered from the same supply as the LEDs, so compare with this off if the noise floor looks high.

Turning up the gain in software won't make it trigger more easily, because the noise floor scales by the same amount. A bigger real signal needs an amplifier in hardware.

> **Hardware note:** on the current sound PCB, the mic's VDD is on the LED supply (`VBUS`, about 4.6 V), not 3.3 V. Check this against the MSM381ACP003 datasheet's rated supply voltage, and make sure the mic output on A3 stays below 3.3 V before relying on the readings.

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
- **[alia_mic_test/](alia_mic_test/alia_mic_test.ino)** - Microphone test sketch for the sound PCB (see [Microphone Test](#microphone-test-sound-board))
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
