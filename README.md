# Band Selection

Firmware for a Raspberry Pi Pico based low-pass filter (LPF) band selector.
The project uses a rotary encoder and push button to choose one of five LPF
bands, displays the current menu state on an SSD1306 OLED, and drives three
GPIO output pins that select the active filter band.

## Features

- Raspberry Pi Pico firmware written in C.
- SSD1306 128x64 OLED display over I2C.
- Rotary encoder menu navigation with push-button selection.
- Five selectable LPF ranges from 0 MHz to 30 MHz.
- Three GPIO control outputs for LPF select lines.
- On-board LED heartbeat to show the firmware is running.

## Hardware

### Raspberry Pi Pico pin usage

| Function | Pico GPIO |
| --- | --- |
| OLED SDA | GP18 |
| OLED SCL | GP19 |
| Encoder CLK | GP21 |
| Encoder DT | GP20 |
| Encoder SW | GP5 |
| LPF S2 | GP2 |
| LPF S1 | GP3 |
| LPF S0 | GP4 |
| On-board LED | GP25 |

The OLED is configured for the common SSD1306 I2C address `0x3C`.

### LPF band select table

| Band | Frequency range | S2 | S1 | S0 |
| --- | --- | --- | --- | --- |
| LPF 1 | 0 to 2 MHz | 0 | 0 | 1 |
| LPF 2 | 2 to 4 MHz | 0 | 1 | 1 |
| LPF 3 | 4 to 8 MHz | 0 | 1 | 0 |
| LPF 4 | 8 to 16 MHz | 1 | 0 | 0 |
| LPF 5 | 16 to 30 MHz | 0 | 0 | 0 |

## Configuration Points

Most hardware-specific values are defined near the top of `main.c`:

- `I2C_SDA`, `I2C_SCL`, and `OLED_ADDR` set the OLED connection.
- `ENC_CLK`, `ENC_DT`, and `ENC_SW` set the rotary encoder pins.
- `LPF_S2`, `LPF_S1`, and `LPF_S0` set the three filter select outputs.
- The `bands[]` table defines the display name and GPIO output pattern for
  each LPF range.

## Controls

1. On startup, the display shows the idle screen.
2. Press the encoder button to enter band selection mode.
3. Rotate the encoder to cycle through the available LPF bands.
4. Press the encoder button again to confirm the selected band.
5. Press once more from the confirmation screen to change the selection.

## Firmware Flow

The program starts by blinking the on-board LED, configuring I2C, initializing
the OLED, setting LPF 1 as the default output, and enabling rotary encoder
interrupts. The main loop then handles button presses, applies encoder movement
only while selecting a band, redraws the OLED when the state changes, and keeps
the LED blinking without blocking input handling.

The menu has three states:

- `STATE_IDLE`: waits for the user to start selecting.
- `STATE_SELECTING`: lets the encoder move through the LPF bands.
- `STATE_CONFIRMED`: shows the selected band after applying the GPIO outputs.

## Build Requirements

- Raspberry Pi Pico SDK.
- CMake 3.13 or newer.
- Arm GNU toolchain supported by the Pico SDK.
- A configured Pico development environment, such as the Raspberry Pi Pico
  VS Code extension.

This project is configured for Pico SDK `2.3.0` in `CMakeLists.txt`.

## Building

From the project root:

```powershell
cmake -S . -B build
cmake --build build
```

If you prefer Ninja:

```powershell
cmake -S . -B build-ninja -G Ninja
cmake --build build-ninja
```

The build produces the usual Pico output files, including:

- `lpf_controller.uf2`
- `lpf_controller.elf`
- `lpf_controller.bin`

## Flashing

1. Hold the Pico `BOOTSEL` button while connecting it to the computer over USB.
2. Wait for the Pico to appear as a USB mass storage device.
3. Copy `lpf_controller.uf2` from the build output directory to the Pico.
4. The Pico will reboot and run the firmware.

## Project Files

| File | Purpose |
| --- | --- |
| `main.c` | Application firmware, OLED driver helpers, encoder handling, menu logic, and LPF GPIO control. |
| `CMakeLists.txt` | Pico SDK and build configuration. |
| `pico_sdk_import.cmake` | Pico SDK import helper. |
| `.gitignore` | Ignores generated build artifacts and local files. |

## Notes

- The firmware defaults to LPF 1 on startup.
- The rotary encoder inputs use internal pull-ups.
- The encoder button is debounced in software.
- The OLED text renderer includes uppercase letters, digits, and spaces.
