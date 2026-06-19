#!/usr/bin/env python3
"""
Motor RPM/Torque UDP Test Sender for Alia eVTOL Flight Sim

Sends UDP JSON packets to the ESP32 on port 5000 to test motor combinations.

Matches Alia_blinky_flightsim.ino expected JSON format:
{
  "model": "a250",
  "crashed": false,
  "state": "running",
  "lift": [
    {"rpm": X, "torque": Y, "state": "normal/degraded/failed"},
    ... (4 motors: FR, FL, RL, RR)
  ],
  "pusher": {"rpm": X, "torque": Y, "state": "normal/degraded/failed"}
}

Run: python3 motor_test_simulator.py
"""

import socket
import json
import time
import os

# UDP Configuration
UDP_IP = "255.255.255.255"  # Broadcast - will reach ESP32 on any IP
UDP_PORT = 5000

# Match the Arduino constants
MAX_RPM = 2500.0
MAX_TORQUE = 1600.0
MIN_TORQUE = -300.0
MAX_LED_BRIGHTNESS = 127

# Terminal colors
class Colors:
    RESET = '\033[0m'
    WHITE = '\033[97m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    GREEN = '\033[92m'
    CYAN = '\033[96m'
    DIM = '\033[2m'
    BRIGHT = '\033[1m'

def clear_screen():
    os.system('clear' if os.name != 'nt' else 'cls')

def create_udp_socket():
    """Create a UDP socket for broadcasting."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    return sock

def send_motor_data(sock, fl_rpm, fl_torque, fl_state,
                    fr_rpm, fr_torque, fr_state,
                    rl_rpm, rl_torque, rl_state,
                    rr_rpm, rr_torque, rr_state,
                    tail_rpm, tail_torque, tail_state,
                    crashed=False, sim_state="running"):
    """
    Send motor data as JSON UDP packet.

    Note: The JSON lift array order is [FR, FL, RL, RR] per the Arduino code mapping.
    """
    data = {
        "model": "a250",
        "crashed": crashed,
        "state": sim_state,
        "lift": [
            {"rpm": fr_rpm, "torque": fr_torque, "state": fr_state},  # Index 0 -> FR (Prop 2)
            {"rpm": fl_rpm, "torque": fl_torque, "state": fl_state},  # Index 1 -> FL (Prop 1)
            {"rpm": rl_rpm, "torque": rl_torque, "state": rl_state},  # Index 2 -> RL (Prop 3)
            {"rpm": rr_rpm, "torque": rr_torque, "state": rr_state},  # Index 3 -> RR (Prop 4)
        ],
        "pusher": {"rpm": tail_rpm, "torque": tail_torque, "state": tail_state}
    }

    json_str = json.dumps(data)
    sock.sendto(json_str.encode(), (UDP_IP, UDP_PORT))
    return json_str

def display_status(name, fl, fr, rl, rr, tail, crashed=False):
    """Display current motor status in terminal."""
    def motor_str(rpm, torque, state):
        if state == "failed":
            return f"{Colors.RED}FAILED{Colors.RESET}"
        elif state == "degraded":
            return f"{Colors.RED}DEGRADED{Colors.RESET}"
        elif torque < 0:
            pct = int(abs(torque) / abs(MIN_TORQUE) * 100)
            return f"{Colors.YELLOW}DRAG {pct}%{Colors.RESET}"
        else:
            return f"{Colors.GREEN}{rpm:.0f}rpm {torque:.0f}Nm{Colors.RESET}"

    print(f"\n{Colors.BRIGHT}=== {name} ==={Colors.RESET}")
    if crashed:
        print(f"{Colors.RED}*** CRASHED ***{Colors.RESET}")
    print(f"  FL: {motor_str(fl[0], fl[1], fl[2]):30s}  FR: {motor_str(fr[0], fr[1], fr[2])}")
    print(f"  RL: {motor_str(rl[0], rl[1], rl[2]):30s}  RR: {motor_str(rr[0], rr[1], rr[2])}")
    print(f"  TAIL: {motor_str(tail[0], tail[1], tail[2])}")

def test_case(sock, name, fl, fr, rl, rr, tail, crashed=False, duration=2.0):
    """Run a single test case - send UDP and display."""
    send_motor_data(sock,
                    fl[0], fl[1], fl[2],
                    fr[0], fr[1], fr[2],
                    rl[0], rl[1], rl[2],
                    rr[0], rr[1], rr[2],
                    tail[0], tail[1], tail[2],
                    crashed=crashed)

    display_status(name, fl, fr, rl, rr, tail, crashed)
    print(f"\n  {Colors.DIM}Sent to {UDP_IP}:{UDP_PORT}{Colors.RESET}")
    time.sleep(duration)

def run_all_tests(sock):
    """Run through all test scenarios."""
    clear_screen()
    print(f"\n{Colors.BRIGHT}MOTOR TEST SEQUENCE - Sending UDP to port {UDP_PORT}{Colors.RESET}")
    print("=" * 60)

    # Normal operation tests
    test_case(sock, "All STOPPED",
              (0, 0, "normal"), (0, 0, "normal"),
              (0, 0, "normal"), (0, 0, "normal"),
              (0, 0, "normal"))

    test_case(sock, "All LOW (500 RPM, 200 Nm)",
              (500, 200, "normal"), (500, 200, "normal"),
              (500, 200, "normal"), (500, 200, "normal"),
              (500, 200, "normal"))

    test_case(sock, "All MEDIUM (1500 RPM, 800 Nm)",
              (1500, 800, "normal"), (1500, 800, "normal"),
              (1500, 800, "normal"), (1500, 800, "normal"),
              (1500, 800, "normal"))

    test_case(sock, "All HIGH (2500 RPM, 1600 Nm)",
              (2500, 1600, "normal"), (2500, 1600, "normal"),
              (2500, 1600, "normal"), (2500, 1600, "normal"),
              (2500, 1600, "normal"))

    # Degraded/Failed tests
    test_case(sock, "FL DEGRADED",
              (0, 0, "degraded"), (2000, 1000, "normal"),
              (2000, 1000, "normal"), (2000, 1000, "normal"),
              (1500, 600, "normal"))

    test_case(sock, "FR FAILED",
              (2000, 1000, "normal"), (0, 0, "failed"),
              (2000, 1000, "normal"), (2000, 1000, "normal"),
              (1500, 600, "normal"))

    # DRAG tests - proportional yellow brightness
    test_case(sock, "FL SLIGHT DRAG (torque=-50, ~17%)",
              (1000, -50, "normal"), (1500, 800, "normal"),
              (1500, 800, "normal"), (1500, 800, "normal"),
              (800, 400, "normal"))

    test_case(sock, "FL MEDIUM DRAG (torque=-150, 50%)",
              (800, -150, "normal"), (1500, 800, "normal"),
              (1500, 800, "normal"), (1500, 800, "normal"),
              (800, 400, "normal"))

    test_case(sock, "FL HEAVY DRAG (torque=-300, 100%)",
              (500, -300, "normal"), (1500, 800, "normal"),
              (1500, 800, "normal"), (1500, 800, "normal"),
              (800, 400, "normal"))

    # Varying drag across motors
    test_case(sock, "Varying DRAG: FL=25%, FR=50%, RL=75%, RR=100%",
              (600, -75, "normal"),   # FL: 25%
              (500, -150, "normal"),  # FR: 50%
              (400, -225, "normal"),  # RL: 75%
              (300, -300, "normal"),  # RR: 100%
              (800, 400, "normal"))

    # Combined scenarios
    test_case(sock, "FL DEGRADED + FR DRAG",
              (0, 0, "degraded"), (500, -200, "normal"),
              (2000, 1000, "normal"), (2000, 1000, "normal"),
              (1000, 500, "normal"))

    # Flight phases
    test_case(sock, "LIFT PHASE",
              (2400, 1400, "normal"), (2400, 1400, "normal"),
              (2400, 1400, "normal"), (2400, 1400, "normal"),
              (200, 50, "normal"))

    test_case(sock, "CONVENTIONAL (props parked)",
              (0, 0, "normal"), (0, 0, "normal"),
              (0, 0, "normal"), (0, 0, "normal"),
              (2500, 1200, "normal"))

    test_case(sock, "DESCENT with DRAG",
              (1200, -150, "normal"), (1200, -150, "normal"),
              (1200, -150, "normal"), (1200, -150, "normal"),
              (500, -50, "normal"))

    # Crash test
    test_case(sock, "CRASHED!",
              (0, 0, "normal"), (0, 0, "normal"),
              (0, 0, "normal"), (0, 0, "normal"),
              (0, 0, "normal"), crashed=True)

    # Return to normal
    test_case(sock, "Recovery - All NORMAL",
              (1500, 800, "normal"), (1500, 800, "normal"),
              (1500, 800, "normal"), (1500, 800, "normal"),
              (1000, 500, "normal"))

def drag_brightness_sweep(sock):
    """Sweep through drag brightness levels on FL motor."""
    clear_screen()
    print(f"\n{Colors.BRIGHT}DRAG BRIGHTNESS SWEEP - FL Motor{Colors.RESET}")
    print("=" * 60)
    print("Sweeping torque from 0 to -300 Nm on Front Left motor")
    print("Yellow brightness should increase proportionally\n")

    for torque in range(0, -301, -25):
        brightness_pct = int(abs(torque) / 300 * 100)
        send_motor_data(sock,
                        500, torque, "normal",  # FL with varying drag
                        1500, 800, "normal",
                        1500, 800, "normal",
                        1500, 800, "normal",
                        800, 400, "normal")
        print(f"  FL Torque: {torque:+4d} Nm -> Yellow brightness: {brightness_pct:3d}%")
        time.sleep(0.5)

    print("\nSweep complete!")
    time.sleep(1)

def interactive_mode(sock):
    """Interactive mode for manual testing."""
    while True:
        clear_screen()
        print(f"\n{Colors.BRIGHT}INTERACTIVE UDP MOTOR TEST{Colors.RESET}")
        print(f"Sending to {UDP_IP}:{UDP_PORT}")
        print("=" * 60)
        print("\nCommands:")
        print("  test  - Enter motor values manually")
        print("  auto  - Run automatic test sequence")
        print("  sweep - Drag brightness sweep")
        print("  q     - Quit")

        cmd = input("\nCommand: ").strip().lower()

        if cmd == 'q':
            break
        elif cmd == 'auto':
            run_all_tests(sock)
            input("\nPress Enter to continue...")
            continue
        elif cmd == 'sweep':
            drag_brightness_sweep(sock)
            input("\nPress Enter to continue...")
            continue
        elif cmd != 'test':
            continue

        print("\nEnter: RPM TORQUE STATE (state: normal/degraded/failed)")
        print("Example: 1500 800 normal  or  500 -200 normal  or  0 0 degraded\n")

        motors = {}
        for name in ['FL', 'FR', 'RL', 'RR', 'TAIL']:
            while True:
                try:
                    val = input(f"  {name}: ").strip().split()
                    rpm = float(val[0])
                    torque = float(val[1])
                    state = val[2] if len(val) > 2 else "normal"
                    if state not in ["normal", "degraded", "failed"]:
                        state = "normal"
                    motors[name] = (rpm, torque, state)
                    break
                except (ValueError, IndexError):
                    print("    Invalid. Use: RPM TORQUE [STATE]")

        crashed = input("  Crashed? (y/n): ").strip().lower() == 'y'

        test_case(sock, "CUSTOM TEST",
                  motors['FL'], motors['FR'],
                  motors['RL'], motors['RR'],
                  motors['TAIL'], crashed=crashed, duration=5)

def continuous_send(sock):
    """Continuously send packets at a fixed rate."""
    clear_screen()
    print(f"\n{Colors.BRIGHT}CONTINUOUS SEND MODE{Colors.RESET}")
    print("=" * 60)
    print("Enter motor values (will send repeatedly at 10Hz)")
    print("Press Ctrl+C to stop\n")

    print("Enter: RPM TORQUE STATE for each motor")
    motors = {}
    for name in ['FL', 'FR', 'RL', 'RR', 'TAIL']:
        while True:
            try:
                val = input(f"  {name}: ").strip().split()
                rpm = float(val[0])
                torque = float(val[1])
                state = val[2] if len(val) > 2 else "normal"
                motors[name] = (rpm, torque, state)
                break
            except (ValueError, IndexError):
                print("    Invalid. Use: RPM TORQUE [STATE]")

    print("\nSending continuously... Press Ctrl+C to stop")
    count = 0
    try:
        while True:
            send_motor_data(sock,
                           motors['FL'][0], motors['FL'][1], motors['FL'][2],
                           motors['FR'][0], motors['FR'][1], motors['FR'][2],
                           motors['RL'][0], motors['RL'][1], motors['RL'][2],
                           motors['RR'][0], motors['RR'][1], motors['RR'][2],
                           motors['TAIL'][0], motors['TAIL'][1], motors['TAIL'][2])
            count += 1
            print(f"\r  Packets sent: {count}", end="", flush=True)
            time.sleep(0.1)  # 10Hz
    except KeyboardInterrupt:
        print(f"\n\nStopped after {count} packets")

def main():
    print(f"\n{Colors.BRIGHT}{'=' * 60}")
    print("  ALIA eVTOL MOTOR UDP TEST SENDER")
    print("  Sends JSON packets to ESP32 on port 5000")
    print(f"{'=' * 60}{Colors.RESET}")
    print(f"\n  Target: {UDP_IP}:{UDP_PORT} (broadcast)")
    print(f"  Constants: MAX_RPM={MAX_RPM}, MAX_TORQUE={MAX_TORQUE}, MIN_TORQUE={MIN_TORQUE}")
    print("\n  Options:")
    print("    1. Run automatic test sequence")
    print("    2. Interactive mode")
    print("    3. Drag brightness sweep")
    print("    4. Continuous send mode")

    sock = create_udp_socket()

    try:
        choice = input("\nSelect (1/2/3/4): ").strip()

        if choice == '1':
            run_all_tests(sock)
            print(f"\n{Colors.GREEN}All tests complete!{Colors.RESET}")
        elif choice == '2':
            interactive_mode(sock)
        elif choice == '3':
            drag_brightness_sweep(sock)
        elif choice == '4':
            continuous_send(sock)
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()

    print("\nExiting.")

if __name__ == "__main__":
    main()
