# LED-PCB-Controller

A custom ATmega328P controller board for WS2812B addressable LED strips, designed in KiCad and built for a light box. Three buttons and a brightness knob drive three lighting modes, from adjustable warm/cool white to six animated "party" effects, all running on a standalone Arduino-compatible board with no dev board required.


![Assembled board](Assembled_PCB.jpg)


## Features

- **Standalone ATmega328P** (DIP-28) running at 16 MHz, fully Arduino-compatible
- **Three lighting modes** selectable with a dedicated MODE button
- **Analog brightness control** via an on-board potentiometer, with smoothing to avoid flicker
- **Six animated effects** in Party mode: rainbow wave, color chase, twinkle, fire, bouncing ball, and meteor rain
- **EMI-hardened button input**: falling-edge detection, a confirm re-read to reject noise spikes from the LED data line, and release tracking to block repeat triggers
- **Two programming paths**: 6-pin ICSP header and FTDI serial header with DTR auto-reset
- **Through-hole only**, so it's easy to hand-solder
- **Compact 2-layer board**, roughly 44.5 × 100 mm

## Repository Contents

| File | Description |
|------|-------------|
| `Lights_For_lightbox.ino` | Arduino firmware (FastLED) |
| `LightBoxPCB.kicad_sch` | KiCad schematic |
| `LightBoxPCB.kicad_pcb` | KiCad PCB layout (2-layer) |
| `Gerbers.zip` | Fabrication-ready Gerber and drill files |

The KiCad files were saved with KiCad 10, so open them with KiCad 10 or newer.

## Controls

The **MODE** button cycles through White → Party → Color. Buttons **A** and **B** change behavior depending on the active mode. The potentiometer sets brightness in every mode.

| Mode  | Button A           | Button B      |
|-------|--------------------|---------------|
| WHITE | Warmer             | Cooler        |
| PARTY | Next effect preset | Shift color   |
| COLOR | Toggle auto-cycle  | Step hue      |

## Hardware

### Pin Map

| ATmega328P Pin | Arduino Pin | Function |
|----------------|-------------|----------|
| PD6 (pin 12)   | D6          | LED data out (through 470 Ω series resistor) |
| PC0 (pin 23)   | A0          | Brightness potentiometer wiper |
| PD2 (pin 4)    | D2          | MODE button (to GND, internal pull-up) |
| PD3 (pin 5)    | D3          | Button A (to GND, internal pull-up) |
| PD4 (pin 6)    | D4          | Button B (to GND, internal pull-up) |
| PD0 / PD1      | D0 / D1     | FTDI serial RX / TX |
| PB3 / PB4 / PB5 | D11 / D12 / D13 | ICSP MOSI / MISO / SCK |

### Connectors

| Ref | Type | Pinout |
|-----|------|--------|
| J3  | Barrel jack | 5 V power input |
| J4  | 1×3 header  | LED strip: 5V, DATA, GND |
| J1  | 2×3 header  | ICSP (standard AVR pinout) |
| J2  | 1×6 header  | FTDI: GND, CTS (n/c), VCC, TX, RX, DTR |

### Bill of Materials

| Ref | Value | Footprint |
|-----|-------|-----------|
| U1 | ATmega328P-P | DIP-28 |
| CLK1 | 16 MHz crystal | HC49-4H |
| C4, C5 | 22 pF | Ceramic disc, 5 mm pitch |
| C1, C2, C3, C6, C7 | 100 nF | Ceramic disc, 5 mm pitch |
| C8 | 10 µF | Radial electrolytic, 5 mm dia. |
| C9 | 1000 µF | Radial electrolytic, 10 mm dia. |
| R1 | 10 kΩ | Axial (reset pull-up) |
| R2 | 470 Ω | Axial (LED data series resistor) |
| RV1 | Potentiometer | Alps RK09L, vertical |
| BTN_MODE1, BTN_A1, BTN_B1 | Tactile switch | 12 mm THT |
| J1 | 2×3 pin header | 2.54 mm |
| J2 | 1×6 pin header | 2.54 mm |
| J3 | Barrel jack | Würth 6941xx301002 |
| J4 | 1×3 pin header | 2.54 mm |

### Design Notes

- **Decoupling:** 100 nF caps on VCC and AVCC, a 100 nF cap on AREF, a 10 µF bulk cap near the MCU, and a 1000 µF reservoir cap on the 5 V rail to absorb inrush current when the strip powers up or changes brightness quickly.
- **LED data line:** the 470 Ω series resistor damps ringing and protects the first LED's data input.
- **Auto-reset:** DTR on the FTDI header couples to RESET through a 100 nF cap, so uploads over serial work like a normal Arduino.

## Power

The barrel jack feeds the 5 V rail directly. There's no on-board regulator, so use a **regulated 5 V supply**. Feeding it 9 V or 12 V will damage the MCU and the LEDs.

Each WS2812B can draw up to about 60 mA at full white. The default 11-LED configuration needs up to about 0.66 A, so a 5 V / 1 A supply is plenty. Scale the supply up if you increase `NUM_LEDS`.

## Fabrication and Assembly

1. Upload `Gerbers.zip` directly to a PCB fab (JLCPCB, PCBWay, OSH Park, etc.). The defaults are fine: 2 layers, 1.6 mm, HASL.
2. Solder components from shortest to tallest: resistors, ceramic caps, crystal, IC socket (recommended for the DIP-28), headers, electrolytics, buttons, potentiometer, barrel jack.
3. Watch the polarity on C8 and C9.
4. Before inserting the ATmega328P, power the board and check for 5 V between VCC and GND on the socket.

## Firmware

### Dependencies

- [Arduino IDE](https://www.arduino.cc/en/software) or Arduino CLI
- [FastLED](https://github.com/FastLED/FastLED) library (install via Library Manager)

### Configuration

Edit these defines at the top of `Lights_For_lightbox.ino` to match your setup:

```cpp
#define NUM_LEDS       11     // number of LEDs on your strip
#define COLOR_ORDER    GRB    // change if colors look swapped
#define DEBOUNCE_MS    600    // raise if buttons still double-trigger
#define CONFIRM_MS     12     // noise-rejection re-check delay
```

### Flashing

Set the board to **Arduino Uno** (ATmega328P, 16 MHz). Then use one of these methods.

**Option A: ICSP (blank chip)**
1. Connect an ISP programmer (USBasp, or another Arduino running the *ArduinoISP* sketch) to J1.
2. Run **Tools → Burn Bootloader** once to set the fuses for the 16 MHz external crystal.
3. Upload with **Sketch → Upload Using Programmer**, or switch to Option B now that the bootloader is installed.

**Option B: FTDI (bootloader installed)**
1. Connect a 5 V FTDI adapter to J2, matching GND to GND.
2. Select the adapter's port and click **Upload**. DTR resets the chip automatically.

You can also flash the chip in an Arduino Uno and move it to the board.

## Troubleshooting

| Symptom | Likely cause and fix |
|---------|----------------------|
| Buttons trigger on their own | EMI from the LED data line. Raise `DEBOUNCE_MS` (e.g. 800) and keep the LED cable away from the button wiring. |
| LEDs flicker or reset at high brightness | Supply can't keep up. Use a beefier 5 V supply and confirm C9 is installed. |
| Wrong colors (e.g. red shows as green) | Change `COLOR_ORDER` to `RGB` or `BRG`. |
| Upload fails over FTDI | Bootloader not installed (use Option A first), TX/RX swapped, or the adapter is set to 3.3 V. |
| Board doesn't run after burning bootloader | Check the crystal and its 22 pF load caps. |

## Author

**Audrey** — Computer Engineering, UC Riverside
