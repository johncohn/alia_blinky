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

### v1.0.3 Build 14 (Current)
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
