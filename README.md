# U2PBridge
> High-Performance USB-to-PS/2 Mouse Adapter Firmware for STM32

**U2PBridge** is a custom, bare-metal USB-to-PS/2 mouse bridge designed specifically for retro systems and custom hardware platforms like the **SIDbox**.

Unlike standard legacy adapters capped at 60–100 Hz, U2PBridge bypasses OS interrupt bottlenecks to deliver high-speed, ultra-low-latency tracking over PS/2 while maintaining 100% standard PS/2 protocol compatibility.

---

## Features

- **Ultra-Low Latency:** High-speed PS/2 clocking allowing up to **1000 Hz polling rates**.
- **Universal PS/2 Compatibility:** Supports standard PS/2 hosts using synchronous device-driven clock interrupts.
- **Bare-Metal Performance:** Direct hardware control with minimal interrupt overhead for crisp, jitter-free cursor tracking.
- **Plug-and-Play USB HID:** Accepts standard USB optical/laser mice via USB Host mode.

---

## Target System & Hardware

- **MCU:** STM32 (e.g., STM32F4 series / 84 MHz core clock)
- **Target Systems:** SIDbox, vintage PCs, retro microcomputers, or custom FPGA/MCU hardware with PS/2 host interfaces.

---

## Pinout Configuration

Connect your target system's PS/2 port to the STM32 GPIO pins as follows:

| Signal | STM32 Pin | Description |
| :--- | :--- | :--- |
| **PS/2 CLK** | `PB0` *(example)* | Clock line (Driven by STM32) |
| **PS/2 DATA**| `PB1` *(example)* | Serial Data line |
| **USB D-** | `PA11` | USB Host Data Minus |
| **USB D+** | `PA12` | USB Host Data Plus |
| **VCC** | `5V` | 5V Power Supply |
| **GND** | `GND` | Common Ground |
| **LED STATUS** | `C11` *blue LED* | flashes when moved or button clicked

> **Note:** This doesn't support wheel mouse (inteli-mouse, yet)

> **Note:** Ensure 4.7kΩ pull-up resistors are connected to 5V on both `PS/2 CLK` and `PS/2 DATA` lines if your host system or target board does not provide them internally. *Adjust STM32 pins in table above if your configuration uses different GPIOs.*
---

## Pinout Configuration

![U2PBridge Pinout Diagram](images/pinout.jpg)

Connect your target system's PS/2 port to the STM32 GPIO pins as follows:



## Flashing Firmware via DFU (USB Bootloader)

Ready-to-flash binary releases are available in the [Releases Tab](../../releases). You can flash the pre-compiled `.bin` or `.dfu` file over USB without requiring an ST-Link programmer.

### Prerequisites
- Download and install **[STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)** or `dfu-util`.
- Get the latest firmware release from the **Releases** section.

### Step-by-Step DFU Flashing Instructions

1. **Enter Bootloader Mode:**
   - Connect the `BOOT0` pin on the STM32 board to **3.3V** (or hold down the onboard `BOOT` button).
   - Plug the board into your computer via USB.
   - Power up or reset the board.

2. **Flash via STM32CubeProgrammer:**
   - Open STM32CubeProgrammer.
   - Select **USB** from the connection drop-down list on the top right.
   - Click **Connect**.
   - Navigate to the **Erasing & Programming** tab.
   - Browse and select the downloaded `U2PBridge.bin` (or `.dfu`) file.
   - Set the start address to `0x08000000`.
   - Check **Verify programming** and click **Start Programming**.

3. **Complete Installation:**
   - Once successfully flashed, disconnect the board from USB.
   - Disconnect `BOOT0` from **3.3V** (set it back to GND or release the button).
   - Plug in your USB mouse, connect to your host system, and enjoy smooth high-speed tracking!

---

### Flashing on Linux via CLI

If you're on Linux, you can flash directly using `dfu-util` without needing a GUI tool:

```bash
sudo dfu-util -a 0 -d 0483:df11 -s 0x08000000:leave -D USB2PS2Bridge1.bin
```


## License

Distributed under the MIT License.
