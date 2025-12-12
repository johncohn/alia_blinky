# Alia 4 - Beta Alia eVTOL LED Animation Controller

LED animation controller for a scale model of the Beta Alia eVTOL aircraft. Features realistic flight simulation with dynamic prop and tail animations.

**Version:** 1.2.0
**Build:** 16
**Hardware:** Seeed XIAO RP2040 + 41 WS2812B LEDs

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
- Seeed XIAO RP2040 board
- Adafruit NeoPixel library

### Setup (Arduino IDE)

1. **Install Board Support**
   - Tools → Board → Boards Manager
   - Search "rp2040"
   - Install "Raspberry Pi Pico/RP2040" (v5.4.3+)
   - Select: Tools → Board → Seeed XIAO RP2040

2. **Install Library**
   - Tools → Manage Libraries
   - Search "Adafruit NeoPixel"
   - Install (v1.15.2+)

3. **Upload**
   - Download/clone this repo
   - Open `Alia_4_v9.ino`
   - Double-tap RESET button on board (RPI-RP2 drive appears)
   - Click Upload (→)

**CLI Alternative:**
```bash
arduino-cli core install rp2040:rp2040
arduino-cli lib install "Adafruit NeoPixel"
arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 Alia_4_v9.ino
arduino-cli upload -p /dev/cu.usbmodem* --fqbn rp2040:rp2040:seeed_xiao_rp2040 Alia_4_v9.ino
```

## Hardware

### LED Layout (41 WS2812B LEDs)
- **Prop 1-4:** 9 LEDs each (0-8, 9-17, 18-26, 27-35)
- **Tail:** 5 LEDs (36-40)

### Pin Assignments
- **GPIO 0:** Starboard nav light
- **GPIO 1:** Port nav light
- **GPIO 2:** Nose nav light
- **GPIO 4:** WS2812B data line

### PCB Design Files
`alia_blinky_kicad.zip` contains complete KiCad project files for a custom PCB.

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

**Compilation errors:**
- Ensure arduino-pico core v5.4.3+
- Verify Adafruit NeoPixel library installed
- Check FQBN: `rp2040:rp2040:seeed_xiao_rp2040`

**Props not animating:**
- Check WS2812B connection on GPIO 4
- Verify 41 LEDs total
- Check serial monitor (115200 baud) for debug info

## Documentation

- **[TUTORIAL.md](TUTORIAL.md)** - Complete step-by-step guide
- **[LICENSE](LICENSE)** - MIT License
- **In-code comments** - Extensive documentation throughout

## Credits

**Created by:** John Cohn, PhD
**Date:** December 2024
**Contact:** jcohn@beta.team | johncohnvt@gmail.com

Developed with major assistance from Anthropic Claude.

Please reach out if you have comments, questions, or suggestions!

## License

MIT License - Copyright (c) 2024 John Cohn

See [LICENSE](LICENSE) file for full text.
