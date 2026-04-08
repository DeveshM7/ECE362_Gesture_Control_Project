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


ECE 362 — Purdue University
