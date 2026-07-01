#!/usr/bin/env python3
"""
CAN receive/echo test for MCP2515 on Pico.

Hardware setup:
  Ubuntu CAN adapter (e.g. USB-CAN) ─── CANH/CANL ─── Pico MCP2515
  120Ω terminator at each end of the bus.

Ubuntu setup (run once per boot):
  sudo ip link set can0 type can bitrate 1000000
  sudo ip link set can0 up

Install dependency:
  pip install python-can

Run:
  python3 test_can.py
  python3 test_can.py --channel can1 --filter  # independent mode test
"""

import argparse
import time
import can

CHANNEL = "can0"
BITRATE = 1000000   # must match MCP2515_BITRATE (KBPS1000)
ECHO_TIMEOUT = 2.0  # seconds to wait for Pico echo


def send_and_wait_echo(bus: can.BusABC, msg: can.Message, label: str) -> bool:
    expected_id = msg.arbitration_id | 0x400
    bus.send(msg)
    print(f"  SENT  [{label}] ID:0x{msg.arbitration_id:03X} "
          f"LEN:{len(msg.data)} DATA:{msg.data.hex(' ').upper()}")

    deadline = time.monotonic() + ECHO_TIMEOUT
    while time.monotonic() < deadline:
        rx = bus.recv(timeout=deadline - time.monotonic())
        if rx is None:
            break
        if rx.arbitration_id == expected_id:
            print(f"  ECHO  [{label}] ID:0x{rx.arbitration_id:03X} "
                  f"LEN:{len(rx.data)} DATA:{rx.data.hex(' ').upper()} ✓")
            return True

    print(f"  FAIL  [{label}] no echo for ID:0x{expected_id:03X} within {ECHO_TIMEOUT}s")
    return False


def verify_no_echo(bus: can.BusABC, msg: can.Message, label: str, timeout: float = 0.3) -> bool:
    """Send a frame and verify the Pico does NOT echo it (filter rejected it)."""
    rejected_echo_id = msg.arbitration_id | 0x400
    bus.send(msg)
    print(f"  SENT  [{label}] ID:0x{msg.arbitration_id:03X} (expect: filtered)")

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        rx = bus.recv(timeout=deadline - time.monotonic())
        if rx is None:
            break
        if rx.arbitration_id == rejected_echo_id:
            print(f"  FAIL  [{label}] got unexpected echo ID:0x{rx.arbitration_id:03X} ✗")
            return False

    print(f"  OK    [{label}] correctly filtered — no echo ✓")
    return True


def run_tests(channel: str, filter_test: bool):
    bus = can.interface.Bus(channel=channel, bustype="socketcan")
    print(f"Connected to {channel} at {BITRATE // 1000} kbps\n")

    results = []

    # ── Test 1: Standard frame, short payload ──────────────────────────────
    msg = can.Message(arbitration_id=0x100, data=[0x01, 0x02, 0x03], is_extended_id=False)
    results.append(send_and_wait_echo(bus, msg, "STD short"))
    time.sleep(0.05)

    # ── Test 2: Standard frame, 8-byte payload ────────────────────────────
    msg = can.Message(arbitration_id=0x200, data=list(range(8)), is_extended_id=False)
    results.append(send_and_wait_echo(bus, msg, "STD 8-byte"))
    time.sleep(0.05)

    # ── Test 3: Extended frame ─────────────────────────────────────────────
    msg = can.Message(arbitration_id=0x1ABCDEF, data=[0xDE, 0xAD, 0xBE, 0xEF],
                      is_extended_id=True)
    results.append(send_and_wait_echo(bus, msg, "EXT frame"))
    time.sleep(0.05)

    # ── Test 4: Burst of 3 frames (rollover stress) ───────────────────────
    print("  Sending burst of 3 frames...")
    burst_ids = [0x300, 0x301, 0x302]
    for bid in burst_ids:
        bus.send(can.Message(arbitration_id=bid, data=[bid & 0xFF], is_extended_id=False))
    burst_ok = 0
    deadline = time.monotonic() + ECHO_TIMEOUT * 2
    while time.monotonic() < deadline and burst_ok < len(burst_ids):
        rx = bus.recv(timeout=deadline - time.monotonic())
        if rx and (rx.arbitration_id & ~0x400) in burst_ids:
            burst_ok += 1
    ok = burst_ok == len(burst_ids)
    results.append(ok)
    print(f"  BURST received {burst_ok}/{len(burst_ids)} echoes {'✓' if ok else '✗'}")

    # ── Test 5: Filter (only when --filter flag passed) ───────────────────
    if filter_test:
        print("\n  [Filter test]")

        # Ask Pico to configure filters (ID=0x000 data=[0x01]).
        # ID 0x000 always passes because MCP2515 filter slot 1 defaults to 0x000
        # and is never overwritten, so it acts as a permanent control channel.
        cmd_on  = can.Message(arbitration_id=0x000, data=[0x01], is_extended_id=False)
        cmd_off = can.Message(arbitration_id=0x000, data=[0x00], is_extended_id=False)

        if not send_and_wait_echo(bus, cmd_on, "Filter-enter"):
            print("  SKIP  filter test (Pico did not ACK filter setup)")
            results.append(False)
        else:
            time.sleep(0.05)

            # 5a: RXB0 filter (0x100) → expect echo
            msg = can.Message(arbitration_id=0x100, data=[0xAA], is_extended_id=False)
            results.append(send_and_wait_echo(bus, msg, "Filter RXB0-allowed"))
            time.sleep(0.05)

            # 5b: RXB1 filter (0x200) → expect echo
            msg = can.Message(arbitration_id=0x200, data=[0xCC], is_extended_id=False)
            results.append(send_and_wait_echo(bus, msg, "Filter RXB1-allowed"))
            time.sleep(0.05)

            # 5c: rejected frame → must NOT be echoed
            msg = can.Message(arbitration_id=0x300, data=[0xBB], is_extended_id=False)
            results.append(verify_no_echo(bus, msg, "Filter-rejected"))
            time.sleep(0.05)

            # Reset filters so subsequent tests (if any) are unaffected
            results.append(send_and_wait_echo(bus, cmd_off, "Filter-exit"))

    # ── Test 6: Extended ID Filter (only when --filter flag passed) ───────
    if filter_test:
        print("\n  [Extended ID filter test]")

        cmd_on  = can.Message(arbitration_id=0x000, data=[0x02], is_extended_id=False)
        cmd_off = can.Message(arbitration_id=0x000, data=[0x00], is_extended_id=False)

        if not send_and_wait_echo(bus, cmd_on, "ExtFilter-enter"):
            print("  SKIP  ext filter test (Pico did not ACK)")
            results.append(False)
        else:
            time.sleep(0.05)

            # 6a: RXB0 extended filter (0x1ABCDEF) → expect echo
            msg = can.Message(arbitration_id=0x1ABCDEF, data=[0xAA], is_extended_id=True)
            results.append(send_and_wait_echo(bus, msg, "ExtFilter RXB0-allowed"))
            time.sleep(0.05)

            # 6b: RXB1 extended filter (0x1234567) → expect echo
            msg = can.Message(arbitration_id=0x1234567, data=[0xCC], is_extended_id=True)
            results.append(send_and_wait_echo(bus, msg, "ExtFilter RXB1-allowed"))
            time.sleep(0.05)

            # 6c: non-matching extended ID → must NOT be echoed
            msg = can.Message(arbitration_id=0x1111111, data=[0xBB], is_extended_id=True)
            results.append(verify_no_echo(bus, msg, "ExtFilter-rejected"))
            time.sleep(0.05)

            results.append(send_and_wait_echo(bus, cmd_off, "ExtFilter-exit"))

    # ── Summary ───────────────────────────────────────────────────────────
    passed = sum(results)
    total  = len(results)
    print(f"\n{'='*40}")
    print(f"Result: {passed}/{total} tests passed")
    if passed == total:
        print("All tests passed ✓")
    else:
        print("Some tests FAILED ✗")

    bus.shutdown()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--channel", default=CHANNEL, help="SocketCAN interface (default: can0)")
    parser.add_argument("--filter", action="store_true", help="Run filter test (Independent mode)")
    args = parser.parse_args()
    run_tests(args.channel, args.filter)
