# T114 Validator

Hardware validation application for the Heltec Mesh Node T114 v2.0 board.

## Purpose

This sample tests the major on-board peripherals through an interactive UI
controlled by the user button. Use it to verify a board after assembly or
to confirm a Zephyr build works correctly on real hardware.

## Tests performed

- **Button** — Detects short and long presses
- **LED** — Blinks the green user LED
- **NeoPixel** — Cycles RGB on the two WS2812B LEDs
- **TFT** — Displays a color pattern on the ST7789V
- **ADC** — Reads battery voltage via the voltage divider
- **Temperature** — Reads the nRF52840 die temperature
- **BLE** — Initializes the Bluetooth controller
- **LoRa** — Configures the SX1262 modem and sends a packet
- **QSPI** — Probes the MX25R1635F flash (skipped if not populated)

## Building

```
west build -b heltec_mesh_node_t114/nrf52840/uf2 -s samples/t114_validator
```

The board overlay enables the `vext` regulator so NeoPixel, LoRa and GNSS
receive power. On the upstream board definition `vext` is disabled by default.

## Flashing

Enter UF2 bootloader (double-press reset) and copy:

```
cp -X build/zephyr/zephyr.uf2 /Volumes/HT-n5262/
```

## Controls

- **Short press** — Move selection / cancel
- **Long press** — Enter / run / confirm

## Modes

- **Auto Test** — Runs all tests sequentially and shows a pass/fail summary
- **Manual** — Choose individual tests from a menu

## Requirements

- Zephyr v4.4+
- `CONFIG_LVGL=y`
- `CONFIG_REGULATOR=y` (for vext)
- Heltec Mesh Node T114 board support

## License

Apache-2.0
