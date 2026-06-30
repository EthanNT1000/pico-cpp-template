# Build Yourself a Picoprobe

*picoprobe* turns a spare Raspberry Pi Pico into a pocket USB-to-SWD adapter. You end up with a two-Pico setup: one **programmer** Pico running *picoprobe*, and one **target** Pico running your code. The programmer bridges your host machine's debugger (OpenOCD + GDB) to the target's SWD pins — no more unplugging and re-plugging USB cables to flash new firmware.

---

## Prerequisites

- Two Raspberry Pi Picos
- The Pico C/C++ SDK and ARM toolchain already installed
- `git`, `cmake`, `make` available on your system

---

## 1. Build and Install Picoprobe

```bash
cd ~/git
git clone https://github.com/raspberrypi/picoprobe.git
cd picoprobe
mkdir build && cd build
cmake ..
make -j4
```

Then flash the firmware to the **programmer** Pico:

1. Hold down **BOOTSEL** and connect the programmer Pico to USB — it mounts as a USB storage device.
2. Copy `picoprobe.uf2` to the mounted storage:
   ```bash
   cp picoprobe.uf2 /media/$USER/RPI-RP2/
   ```
3. The Pico reboots automatically as a picoprobe.

```bash
cd ~/git
rm -rf picoprobe   # optional cleanup
```

---

## 2. Build and Install OpenOCD

OpenOCD manages the communication between your host debugger and the target MCU via *picoprobe*. You need the Raspberry Pi Foundation's fork, which has picoprobe support.

### Install dependencies (Ubuntu)

```bash
sudo apt update
sudo apt install libtool automake texinfo pkg-config libusb-1.0-0-dev gcc make libhidapi-hidraw0
```

### Build OpenOCD

```bash
git clone https://github.com/raspberrypi/openocd.git \
    --branch picoprobe --depth=1
cd openocd
./bootstrap
./configure --enable-picoprobe --disable-werror
make -j4
sudo make install
cd ../
rm -rf openocd   # optional cleanup
```

This installs `openocd` to `/usr/local/bin/`. Verify with:

```bash
openocd --version
```

---

## 3. Wire Up the Hardware

Connect the two Picos as follows. The programmer Pico is the one with a USB cable going to your machine.

| Programmer Pico | Target Pico      |
|-----------------|------------------|
| GP2             | SWDIO (DEBUG)    |
| GP3             | SWDCLK (DEBUG)   |
| GND             | GND (DEBUG)      |
| VSYS            | VSYS *(optional — powers target from probe)* |

> The target's DEBUG header (SWDIO, SWDCLK, GND) is a 3-pin connector on the edge of the board — you may need to solder header pins to it.

---

## 4. Troubleshooting

**`DAP Init Failed` errors** — almost always a wiring problem. The most common cause is an incorrectly grounded SWD GND pin. Try a different GND pin on the programmer Pico if this persists.

**Wrong kit / compiler selected** — if your IDE or CMake can't find the compiler, re-select `arm-none-eabi-gcc` as the active kit.

**OpenOCD can't find picoprobe** — you may need a udev rule on Linux so OpenOCD can open the USB device without root:

```bash
sudo tee /etc/udev/rules.d/60-openocd.rules << 'EOF'
# Raspberry Pi Picoprobe
ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0004", MODE="664", GROUP="plugdev"
EOF
sudo udevadm control --reload-rules
sudo usermod -aG plugdev $USER
# Log out and back in for the group change to take effect
```

---

## References

- [Original article by smittytone](https://blog.smittytone.net/2021/02/05/how-to-debug-a-raspberry-pi-pico-with-a-mac-swd/)
- [picoprobe source](https://github.com/raspberrypi/picoprobe)
- [Raspberry Pi OpenOCD fork](https://github.com/raspberrypi/openocd)
