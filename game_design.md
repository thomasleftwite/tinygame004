# Tetris on XIAO ESP32C3 - Game Design

## Overview
A classic Tetris implementation for the XIAO ESP32C3, using an OLED display for graphics, physical buttons for input, and utilizing the onboard NeoPixels and buzzer for an enhanced experience.

## Important notice
- Use the game machine rotate 90 degrees CW, because it is not possible to rotate the OLED screen.
- That causes the button functions to be swapped, so:
  - UP and DOWN buttons are used Move Right and Move Left
  - LEFT and RIGHT buttons are used Rotate and Drop.
## Hardware Specifications
- **Microcontroller**: XIAO ESP32C3
- **Display**: SSD1306 OLED via I2C (SDA=6, SCL=7)
  - **Resolution**: 128x64
    - In this game, use the OLED 64 width and 128 height. 
- **Input (Buttons)**:
  - UP (Pin 20) - Move Right
  - DOWN (Pin 8) - Move Left
  - LEFT (Pin 9) - Rotate
  - RIGHT (Pin 10) - Hard Drop
- **Audio**: Buzzer (Pin 5)
- **Visuals**: 3x NeoPixels (Pin 2)

## Game Mechanics
- **Grid Size**: Standard 10 columns by 20 rows.
  - *Note: If 128x64, blocks will be rendered as 6x6 pixels to fit the screen vertically (6x10 = 60 pixels).*
- **Tetrominoes**: All 7 classic shapes (I, J, L, O, S, T, Z) with standard rotations.
- **Scoring**: Points awarded for lines cleared. More points for clearing multiple lines at once.
- **Speed**: Game speed increases slightly as the player's score or level increases.

## Software Architecture
- **Libraries Required**:
  - `Wire.h`
  - `Adafruit_GFX.h`
  - `Adafruit_SSD1306.h`
  - `Adafruit_NeoPixel.h`
- **Main Loop (`tinygame004.ino`)**:
  - `setup()`: Initialize hardware and libraries.
  - `loop()`:
    - Handle button inputs (debounced).
    - Update game logic (piece falling, collision detection, line clears).
    - Render screen.

## Feedback Mechanisms (To Be Clarified)
- **Buzzer Audio**:
  - [x] Sound for Piece Move/Rotate
  - [x] Sound for Line Clear
  - [x] Sound for Game Over
- **NeoPixel Effects**:
  - [x] Flash green on line clear
  - [x] Flash red on game over
