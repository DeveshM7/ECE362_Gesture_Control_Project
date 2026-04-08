# ECE 362 Final Project — Gesture-Controlled Game

A gesture-controlled game running on the **Raspberry Pi RP2350**. Players control a character on an LCD screen by swiping their hand (UP, DOWN, LEFT, RIGHT) over an APDS-9960 gesture sensor.

## Hardware

| Component | Part |
|---|---|
| Microcontroller | Raspberry Pi RP2350 |
| Gesture Sensor | APDS-9960 |
| Display | LCD (SPI) |

## How It Works

The APDS-9960 uses an IR LED and four directional photodiodes to detect hand swipes. Gesture data is read over I2C and mapped to character movement on the LCD.

## Project Structure

```
src/
├── apds9960.c/h    — APDS-9960 sensor driver
├── gesture.c/h     — gesture reading logic
├── i2c_hal.c/h     — I2C hardware abstraction (Pico SDK)
├── lcd.c/h         — LCD display driver
└── main.c          — game loop
```

## Building

Requires the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).

```bash
mkdir build && cd build
cmake ..
make
```

Flash the resulting `.uf2` file to the RP2350 by holding BOOTSEL while connecting USB.

## Wiring

| APDS-9960 | RP2350 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GP4 |
| SCL | GP5 |

> **Note:** The APDS-9960 is 3.3V only. Do not connect to 5V.

## Team

ECE 362 — Purdue University
