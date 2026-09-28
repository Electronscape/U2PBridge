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
| **Amiga V / Up** | `PA0` | DE-9 pin 1 vertical phase A |
| **Amiga H / Down** | `PA1` | DE-9 pin 2 horizontal phase A |
| **Amiga VQ / Left** | `PA2` | DE-9 pin 3 vertical phase B |
| **Amiga HQ / Right** | `PA3` | DE-9 pin 4 horizontal phase B |
| **Amiga Button 1** | `PA4` | DE-9 pin 6 left button, active-low |
| **Amiga Button 2** | `PA5` | DE-9 pin 9 right button, active-low |
| **Amiga Button 3** | `PA6` | Optional DE-9 pin 5 middle button, active-low |
| **USB D-** | `PA11` | USB Host Data Minus |
| **USB D+** | `PA12` | USB Host Data Plus |
| **VCC** | `5V` | 5V Power Supply |
| **GND** | `GND` | Common Ground |
| **LED STATUS** | `C11` *blue LED* | flashes when moved or button clicked

> **Note:** This doesn't support wheel mouse (inteli-mouse, yet)

> **Note:** Ensure 4.7kΩ pull-up resistors are connected to 5V on both `PS/2 CLK` and `PS/2 DATA` lines if your host system or target board does not provide them internally. *Adjust STM32 pins in table above if your configuration uses different GPIOs.*
---

## Pinout Configuration

<img width="293" height="129" alt="conn_dsub9m" src="https://github.com/user-attachments/assets/6e25f474-10bf-41e7-801c-f721269f300c" />


<img width="1561" height="1178" alt="pinouts" src="https://github.com/user-attachments/assets/d8f707be-e465-4876-aa5b-666317ecbefc" />

<svg xmlns="http://www.w3.org/2000/svg" width="293.29" height="128.95" viewBox="0 0 293.29 128.95" role="img" aria-label="Pinout diagram of the 9-pin D-sub male (DE-9) connector, mating-face view."><metadata><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:cc="http://creativecommons.org/ns#" xmlns:dc="http://purl.org/dc/elements/1.1/"><cc:Work rdf:about=""><dc:title>9-pin D-sub male (DE-9)</dc:title><dc:description>Pinout diagram of the 9-pin D-sub male (DE-9) connector, mating-face view.</dc:description><dc:identifier>https://allpinouts.org/img/conn_dsub9m.svg</dc:identifier><dc:subject><rdf:Bag><rdf:li>connector</rdf:li><rdf:li>pinout</rdf:li><rdf:li>pinout diagram</rdf:li><rdf:li>D-sub</rdf:li><rdf:li>D-subminiature</rdf:li><rdf:li>9-pin</rdf:li><rdf:li>DE-9</rdf:li><rdf:li>male</rdf:li></rdf:Bag></dc:subject><dc:source>https://allpinouts.org/</dc:source><dc:creator><cc:Agent><dc:title>Nicola Asuni</dc:title></cc:Agent></dc:creator><dc:publisher><cc:Agent><dc:title>allpinouts.org</dc:title></cc:Agent></dc:publisher><dc:rights><cc:Agent><dc:title>&#169; 2026 Nicola Asuni - Tecnick.com</dc:title></cc:Agent></dc:rights></cc:Work></rdf:RDF></metadata><style>.sh{fill:none;stroke:#333;stroke-width:1.5;stroke-linejoin:round;stroke-linecap:round}.in{fill:#8c8c8c;fill-opacity:.14;stroke:#333;stroke-width:1;stroke-linejoin:round}.pn{fill:none;stroke:#333;stroke-width:1.2}.p1{fill:#8c8c8c;fill-opacity:.35;stroke:#333;stroke-width:1.2}.tx{font-family:Arial,Helvetica,sans-serif;fill:#333;text-anchor:middle}@media (prefers-color-scheme:dark){.sh,.pn,.p1,.in{stroke:#ccc}.tx{fill:#ccc}.p1{fill:#9a9a9a}.in{fill:#9a9a9a}}</style><rect class="sh" x="8" y="8" width="277.29" height="112.95" rx="14.4" ry="14.4"/><circle class="sh" cx="34.19" cy="64.47" r="13.68"/><circle class="sh" cx="259.1" cy="64.47" r="13.68"/><path class="in" d="M 73.13 41.75 L 73.13 41.75 Q 70.5 26.86 85.62 26.86 L 207.67 26.86 Q 222.79 26.86 220.16 41.75 L 212.14 87.2 Q 209.52 102.09 194.4 102.09 L 98.89 102.09 Q 83.77 102.09 81.15 87.2 Z"/><path class="sh" d="M 63.64 37.51 L 63.64 37.51 Q 59.88 16.23 81.48 16.23 L 211.81 16.23 Q 233.41 16.23 229.65 37.51 L 220.15 91.44 Q 216.4 112.72 194.8 112.72 L 98.49 112.72 Q 76.89 112.72 73.14 91.44 Z"/><circle class="p1" cx="96.78" cy="51.7" r="8.48"/><text class="tx" x="96.78" y="51.7" dy="0.35em" font-size="8">1</text><circle class="pn" cx="121.72" cy="51.7" r="8.48"/><text class="tx" x="121.72" y="51.7" dy="0.35em" font-size="8">2</text><circle class="pn" cx="146.64" cy="51.7" r="8.48"/><text class="tx" x="146.64" y="51.7" dy="0.35em" font-size="8">3</text><circle class="pn" cx="171.58" cy="51.7" r="8.48"/><text class="tx" x="171.58" y="51.7" dy="0.35em" font-size="8">4</text><circle class="pn" cx="196.5" cy="51.7" r="8.48"/><text class="tx" x="196.5" y="51.7" dy="0.35em" font-size="8">5</text><circle class="pn" cx="109.25" cy="77.25" r="8.48"/><text class="tx" x="109.25" y="77.25" dy="0.35em" font-size="8">6</text><circle class="pn" cx="134.18" cy="77.25" r="8.48"/><text class="tx" x="134.18" y="77.25" dy="0.35em" font-size="8">7</text><circle class="pn" cx="159.11" cy="77.25" r="8.48"/><text class="tx" x="159.11" y="77.25" dy="0.35em" font-size="8">8</text><circle class="pn" cx="184.04" cy="77.25" r="8.48"/><text class="tx" x="184.04" y="77.25" dy="0.35em" font-size="8">9</text></svg>

<svg xmlns="http://www.w3.org/2000/svg" width="293.29" height="128.95" viewBox="0 0 293.29 128.95" role="img" aria-label="Pinout diagram of the 9-pin D-sub female (DE-9) connector, mating-face view."><metadata><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:cc="http://creativecommons.org/ns#" xmlns:dc="http://purl.org/dc/elements/1.1/"><cc:Work rdf:about=""><dc:title>9-pin D-sub female (DE-9)</dc:title><dc:description>Pinout diagram of the 9-pin D-sub female (DE-9) connector, mating-face view.</dc:description><dc:identifier>https://allpinouts.org/img/conn_dsub9f.svg</dc:identifier><dc:subject><rdf:Bag><rdf:li>connector</rdf:li><rdf:li>pinout</rdf:li><rdf:li>pinout diagram</rdf:li><rdf:li>D-sub</rdf:li><rdf:li>D-subminiature</rdf:li><rdf:li>9-pin</rdf:li><rdf:li>DE-9</rdf:li><rdf:li>female</rdf:li></rdf:Bag></dc:subject><dc:source>https://allpinouts.org/</dc:source><dc:creator><cc:Agent><dc:title>Nicola Asuni</dc:title></cc:Agent></dc:creator><dc:publisher><cc:Agent><dc:title>allpinouts.org</dc:title></cc:Agent></dc:publisher><dc:rights><cc:Agent><dc:title>&#169; 2026 Nicola Asuni - Tecnick.com</dc:title></cc:Agent></dc:rights></cc:Work></rdf:RDF></metadata><style>.sh{fill:none;stroke:#333;stroke-width:1.5;stroke-linejoin:round;stroke-linecap:round}.in{fill:#8c8c8c;fill-opacity:.14;stroke:#333;stroke-width:1;stroke-linejoin:round}.pn{fill:none;stroke:#333;stroke-width:1.2}.p1{fill:#8c8c8c;fill-opacity:.35;stroke:#333;stroke-width:1.2}.tx{font-family:Arial,Helvetica,sans-serif;fill:#333;text-anchor:middle}@media (prefers-color-scheme:dark){.sh,.pn,.p1,.in{stroke:#ccc}.tx{fill:#ccc}.p1{fill:#9a9a9a}.in{fill:#9a9a9a}}</style><rect class="sh" x="8" y="8" width="277.29" height="112.95" rx="14.4" ry="14.4"/><circle class="sh" cx="34.19" cy="64.47" r="13.68"/><circle class="sh" cx="259.1" cy="64.47" r="13.68"/><path class="in" d="M 73.13 41.75 L 73.13 41.75 Q 70.5 26.86 85.62 26.86 L 207.67 26.86 Q 222.79 26.86 220.16 41.75 L 212.14 87.2 Q 209.52 102.09 194.4 102.09 L 98.89 102.09 Q 83.77 102.09 81.15 87.2 Z"/><path class="sh" d="M 63.64 37.51 L 63.64 37.51 Q 59.88 16.23 81.48 16.23 L 211.81 16.23 Q 233.41 16.23 229.65 37.51 L 220.15 91.44 Q 216.4 112.72 194.8 112.72 L 98.49 112.72 Q 76.89 112.72 73.14 91.44 Z"/><circle class="p1" cx="196.5" cy="51.7" r="8.48"/><text class="tx" x="196.5" y="51.7" dy="0.35em" font-size="8">1</text><circle class="pn" cx="171.57" cy="51.7" r="8.48"/><text class="tx" x="171.57" y="51.7" dy="0.35em" font-size="8">2</text><circle class="pn" cx="146.64" cy="51.7" r="8.48"/><text class="tx" x="146.64" y="51.7" dy="0.35em" font-size="8">3</text><circle class="pn" cx="121.71" cy="51.7" r="8.48"/><text class="tx" x="121.71" y="51.7" dy="0.35em" font-size="8">4</text><circle class="pn" cx="96.78" cy="51.7" r="8.48"/><text class="tx" x="96.78" y="51.7" dy="0.35em" font-size="8">5</text><circle class="pn" cx="184.04" cy="77.25" r="8.48"/><text class="tx" x="184.04" y="77.25" dy="0.35em" font-size="8">6</text><circle class="pn" cx="159.11" cy="77.25" r="8.48"/><text class="tx" x="159.11" y="77.25" dy="0.35em" font-size="8">7</text><circle class="pn" cx="134.18" cy="77.25" r="8.48"/><text class="tx" x="134.18" y="77.25" dy="0.35em" font-size="8">8</text><circle class="pn" cx="109.25" cy="77.25" r="8.48"/><text class="tx" x="109.25" y="77.25" dy="0.35em" font-size="8">9</text></svg>

<img width="293" height="129" alt="conn_dsub9f" src="https://github.com/user-attachments/assets/f403c304-c587-4bcd-a93f-bded65bb5bf6" />


![Uploading conn_dsub9m.svg…]()


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
