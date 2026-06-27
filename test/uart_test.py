"""
UART echo test for Pico W UART DMA driver.

Usage:
    python uart_test.py --port COM3          (Windows)
    python uart_test.py --port /dev/ttyUSB0  (Linux)

Requires: pip install pyserial
"""

import argparse
import time
import serial


BAUD      = 57600
RX_HALF   = 64          # must match UartTestTask::RX_HALF_SIZE
IDLE_MS   = 5           # must match UART idleTimeoutMs
READ_GAP  = 0.15        # seconds to wait after sending before reading (> idle timeout)


def open_port(port: str) -> serial.Serial:
    return serial.Serial(port, BAUD, timeout=READ_GAP)


def flush(ser: serial.Serial) -> None:
    ser.reset_input_buffer()
    ser.reset_output_buffer()


def send_recv(ser: serial.Serial, data: bytes, label: str) -> bytes:
    """Send data, collect all echo bytes until the port goes quiet."""
    flush(ser)
    ser.write(data)
    received = bytearray()
    # Keep reading until no bytes arrive for READ_GAP seconds
    while True:
        chunk = ser.read(len(data) - len(received) + 1)
        if not chunk:
            break
        received.extend(chunk)
        if len(received) >= len(data):
            break
    return bytes(received)


def check(label: str, sent: bytes, received: bytes) -> bool:
    if sent == received:
        print(f"  PASS  {label}  ({len(sent)} bytes)")
        return True
    print(f"  FAIL  {label}")
    print(f"        sent     ({len(sent):4d}): {sent[:32].hex()}{'...' if len(sent) > 32 else ''}")
    print(f"        received ({len(received):4d}): {received[:32].hex()}{'...' if len(received) > 32 else ''}")
    return False


def run_tests(port: str) -> None:
    results = []

    with open_port(port) as ser:
        time.sleep(0.5)   # let Pico finish boot
        flush(ser)

        print(f"\nConnected to {port} at {BAUD} baud\n")

        # ── Case 1: Small packet — idle timer path ──────────────────────────
        # Size < RX_HALF_SIZE. DMA stalls, idle timer aborts, reports actual bytes.
        data = b"hello_pico"
        rx = send_recv(ser, data, "small")
        results.append(check("Case 1 — small packet (idle timer path)", data, rx))

        time.sleep(0.1)

        # ── Case 2: Exact half size — natural DMA completion ────────────────
        data = b"A" * RX_HALF
        rx = send_recv(ser, data, "exact half")
        results.append(check("Case 2 — exact rxHalfSize (DMA completion)", data, rx))

        time.sleep(0.1)

        # ── Case 3: Larger than half — multiple ping-pong completions ───────
        data = b"B" * (RX_HALF * 3 + 17)
        rx = send_recv(ser, data, "3.5 halves")
        results.append(check("Case 3 — 3.5 × rxHalfSize (ping-pong continuity)", data, rx))

        time.sleep(0.1)

        # ── Case 4: Two bursts with gap — timer re-arm ──────────────────────
        # Pico echoes first burst after idle, then second burst after idle.
        # Python collects both echoes and checks total content.
        part1 = b"C" * 30
        part2 = b"D" * 30
        flush(ser)
        ser.write(part1)
        time.sleep(READ_GAP)      # let idle timer fire and Pico echo part1
        ser.write(part2)
        time.sleep(READ_GAP)      # let idle timer fire and Pico echo part2
        rx = ser.read(len(part1) + len(part2) + 1)
        results.append(check("Case 4 — two bursts with gap (timer re-arm)", part1 + part2, rx))

        time.sleep(0.1)

        # ── Case 5: Rapid back-to-back — stress ping-pong ───────────────────
        data = b"E" * RX_HALF * 50
        rx = send_recv(ser, data, "stress")
        results.append(check("Case 5 — 50 × rxHalfSize rapid burst (ping-pong stress)", data, rx))

    # ── Summary ──────────────────────────────────────────────────────────────
    passed = sum(results)
    total  = len(results)
    print(f"\n{'='*52}")
    print(f"  {passed}/{total} passed")
    if passed == total:
        print("  All tests passed — UART DMA stack is correct.")
    else:
        print("  Some tests failed — check DMA config / idle timeout.")
    print(f"{'='*52}\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="Serial port (e.g. COM3 or /dev/ttyUSB0)")
    args = parser.parse_args()
    run_tests(args.port)
