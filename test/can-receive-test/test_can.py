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
        print("\n  [Filter test — Independent mode required on Pico]")
        # Pico should be configured with:
        #   mcp2515->setMask(0, 0x7FF);        // exact match on RXB0
        #   mcp2515->setFilter(0, 0x400);       // only accept 0x400 in RXB0
        #   mcp2515->setMask(1, 0x7FF);
        #   mcp2515->setFilter(2, 0x500);       // only accept 0x500 in RXB1

        allowed  = can.Message(arbitration_id=0x400, data=[0xAA], is_extended_id=False)
        rejected = can.Message(arbitration_id=0x123, data=[0xBB], is_extended_id=False)

        bus.send(rejected)
        print(f"  SENT  [Filter-rejected] ID:0x123")
        time.sleep(0.1)

        results.append(send_and_wait_echo(bus, allowed, "Filter-allowed"))

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
