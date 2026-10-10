# XIAO ESP32-S3 Sense WALL-E Robot

A small WALL-E desk robot built on the Seeed Studio XIAO ESP32-S3 Sense. It talks
and listens through XiaoZhi, sees through the Sense camera, shows its face on a
1.54" screen, and drives on two tracks. The AI can drive it by voice through the
`self.walle.move` and `self.walle.stop` MCP tools.

The wiring follows Huy Vector's WALL-E AI build. This board runs the XiaoZhi
firmware, not Huy Vector's own firmware.

## Hardware

| Part | Notes |
|---|---|
| Seeed Studio XIAO ESP32-S3 Sense | 8 MB flash, 8 MB octal PSRAM, onboard PDM mic and camera |
| Camera | Stock OV2640, or Seeed's OV5640 module for the XIAO ESP32S3 Sense. Both are auto-detected |
| 1.54" 240x240 ST7789 SPI display | 8-pin module: GND VCC SCL SDA RST DC CS BL |
| MAX98357A I2S amplifier + small speaker | The XIAO has no speaker output of its own |
| MX1508 dual H-bridge (IN1-IN4) | Drives the two track motors |
| 2 x N20 gear motors | Left and right track |
| 2 x 18650 cells in parallel + USB-C charge/5 V boost module + switch | Powers the XIAO and the motor driver |

## Wiring

XIAO header pins map to GPIOs as D0=1, D1=2, D2=3, D3=4, D4=5, D5=6, D6=43,
D7=44, D8=7, D9=8, D10=9.

| XIAO pin | GPIO | Connects to |
|---|---|---|
| D0 | 1 | MX1508 IN4 (right track) |
| D1 | 2 | MX1508 IN3 (right track) |
| D2 | 3 | MX1508 IN2 (left track) |
| D3 | 4 | MX1508 IN1 (left track) |
| D4 | 5 | MAX98357A DIN |
| D5 | 6 | MAX98357A BCLK |
| D6 | 43 | MAX98357A LRC |
| D7 | 44 | Display DC |
| D8 | 7 | Display RST |
| D9 | 8 | Display SDA |
| D10 | 9 | Display SCL |
| 3V3 | - | Display VCC and BL, MAX98357A VIN |
| GND | - | Display GND and CS, MAX98357A GND, MX1508 -, battery - |
| 5V (VUSB) | - | 5 V from the boost module, through the power switch |

The MX1508 `+` takes 5 V from the boost module, and its motor A and motor B
outputs go to the two N20 motors.

Fixed onboard connections (no wiring needed): PDM mic CLK=GPIO42, DATA=GPIO41;
camera XCLK=10, SIOD=40, SIOC=39, D0-D7=15,17,18,16,14,12,11,48, VSYNC=38,
HREF=47, PCLK=13; user LED=GPIO21.

### Notes

- D6 (GPIO43) and D7 (GPIO44) are the UART0 pins. This build sends logs to the
  XIAO's USB-C port (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`), so those pins are
  free for the amplifier and display.
- Display CS is tied to GND, so the display uses SPI mode 3. The backlight is
  tied to 3V3 and is always on.
- Don't use the Sense board's SD card slot. It shares GPIO7, GPIO8 and GPIO9
  with the display.
- If a track runs backwards, set `MOTOR_LEFT_INVERT` or `MOTOR_RIGHT_INVERT` in
  `config.h`. If "left" turns the robot right, set `MOTOR_SWAP_SIDES`.
- Every move stops on its own after at most 5 seconds, so a misheard command
  can't send the robot off the desk.

## Build

```sh
python3 scripts/build.py xiao-esp32s3-walle
```

To flip or mirror the camera image, set the `camera_vflip` / `camera_hmirror`
build options in `config.json`.

Flash `build/merged-binary.bin` to the XIAO at offset 0x0 over its USB-C port.
If the board doesn't enter download mode, hold BOOT while plugging in USB.

## MCP tools

| Tool | Arguments |
|---|---|
| `self.walle.move` | `direction`: `forward`, `backward`, `left`, `right`; `speed`: 1-100; `duration_ms`: 100-5000 |
| `self.walle.stop` | none |
