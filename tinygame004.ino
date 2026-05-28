#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Display setup (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// NeoPixel setup
Adafruit_NeoPixel pixels(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Game constants
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_SIZE 6

// Tetromino definitions
// 4x4 bitmasks for 7 pieces x 4 rotations
const uint16_t tetrominoes[7][4] = {
  // I
  {0x0F00, 0x2222, 0x0F00, 0x2222},
  // J
  {0x44C0, 0x8E00, 0x6440, 0x0E20},
  // L
  {0x4460, 0x0E80, 0xC440, 0x2E00},
  // O
  {0xCC00, 0xCC00, 0xCC00, 0xCC00},
  // S
  {0x06C0, 0x8C40, 0x06C0, 0x8C40},
  // T
  {0x0E40, 0x4C40, 0x4E00, 0x4640},
  // Z
  {0x0C60, 0x4C80, 0x0C60, 0x4C80}
};

// Game state
uint8_t board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
int currentShape, currentRot, currentX, currentY;
unsigned long lastDropTime = 0;
int dropInterval = 800; // MS between drops
bool gameOver = false;
int score = 0;
int linesClearedTotal = 0;

// Button state for debouncing
bool btnRightState = true, lastBtnRightState = true;
bool btnLeftState = true, lastBtnLeftState = true;
bool btnRotState = true, lastBtnRotState = true;
bool btnDropState = true, lastBtnDropState = true;

// Timing
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(SERIAL_BAUD);

  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for most 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setRotation(1); // Rotate 90 degrees CW. Resolution becomes 64x128
  display.clearDisplay();
  display.display();

  // Initialize input
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);

  // Initialize output
  pinMode(BUZZER_PIN, OUTPUT);
  pixels.begin();
  pixels.clear();
  pixels.show();

  // Seed random
  randomSeed(analogRead(0));

  spawnPiece();
}

bool checkCollision(int shapeIdx, int rot, int x, int y) {
  uint16_t shape = tetrominoes[shapeIdx][rot];
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      if (shape & (1 << (15 - (row * 4 + col)))) {
        int boardX = x + col;
        int boardY = y + row;
        if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT) return true; // walls & floor
        if (boardY >= 0 && board[boardY][boardX]) return true; // placed blocks
      }
    }
  }
  return false;
}

void placePiece() {
  uint16_t shape = tetrominoes[currentShape][currentRot];
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      if (shape & (1 << (15 - (row * 4 + col)))) {
        int boardY = currentY + row;
        int boardX = currentX + col;
        if (boardY >= 0 && boardY < BOARD_HEIGHT && boardX >= 0 && boardX < BOARD_WIDTH) {
          board[boardY][boardX] = 1;
        }
      }
    }
  }
}

void checkLines() {
  int linesClearedNow = 0;
  for (int row = BOARD_HEIGHT - 1; row >= 0; --row) {
    bool full = true;
    for (int col = 0; col < BOARD_WIDTH; ++col) {
      if (!board[row][col]) {
        full = false;
        break;
      }
    }
    if (full) {
      linesClearedNow++;
      // Shift rows down
      for (int r = row; r > 0; --r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
          board[r][c] = board[r-1][c];
        }
      }
      // Clear top row
      for (int c = 0; c < BOARD_WIDTH; ++c) board[0][c] = 0;
      row++; // Check this row again as it got shifted down
    }
  }

  if (linesClearedNow > 0) {
    linesClearedTotal += linesClearedNow;
    score += (linesClearedNow * linesClearedNow) * 100;
    dropInterval = max(100, 800 - (linesClearedTotal * 10)); // Speed up

    // Play line clear sound
    tone(BUZZER_PIN, 800, 100);
    delay(100);
    tone(BUZZER_PIN, 1200, 150);

    // Flash green
    pixels.fill(pixels.Color(0, 255, 0));
    pixels.show();
    delay(100);
    pixels.clear();
    pixels.show();
  }
}

void spawnPiece() {
  currentShape = random(7);
  currentRot = 0;
  currentX = 3;
  currentY = -1; // Start slightly above

  if (checkCollision(currentShape, currentRot, currentX, currentY)) {
    gameOver = true;
    // Play Game Over sound
    tone(BUZZER_PIN, 300, 300);
    delay(300);
    tone(BUZZER_PIN, 200, 400);

    // Flash red
    pixels.fill(pixels.Color(255, 0, 0));
    pixels.show();
  }
}

void playMoveSound() {
  tone(BUZZER_PIN, 440, 20);
}

void handleInput() {
  int readingRight = digitalRead(BUTTON_UP); // Logical Move Right
  int readingLeft = digitalRead(BUTTON_DOWN); // Logical Move Left
  int readingRot = digitalRead(BUTTON_LEFT); // Logical Rotate
  int readingDrop = digitalRead(BUTTON_RIGHT); // Logical Drop

  unsigned long now = millis();

  // Move Right
  if (readingRight != lastBtnRightState) lastDebounceTime[0] = now;
  if ((now - lastDebounceTime[0]) > debounceDelay) {
    if (readingRight != btnRightState) {
      btnRightState = readingRight;
      if (btnRightState == LOW) {
        if (!checkCollision(currentShape, currentRot, currentX + 1, currentY)) {
          currentX++;
          playMoveSound();
        }
      }
    }
  }
  lastBtnRightState = readingRight;

  // Move Left
  if (readingLeft != lastBtnLeftState) lastDebounceTime[1] = now;
  if ((now - lastDebounceTime[1]) > debounceDelay) {
    if (readingLeft != btnLeftState) {
      btnLeftState = readingLeft;
      if (btnLeftState == LOW) {
        if (!checkCollision(currentShape, currentRot, currentX - 1, currentY)) {
          currentX--;
          playMoveSound();
        }
      }
    }
  }
  lastBtnLeftState = readingLeft;

  // Rotate
  if (readingRot != lastBtnRotState) lastDebounceTime[2] = now;
  if ((now - lastDebounceTime[2]) > debounceDelay) {
    if (readingRot != btnRotState) {
      btnRotState = readingRot;
      if (btnRotState == LOW) {
        int nextRot = (currentRot + 1) % 4;
        if (!checkCollision(currentShape, nextRot, currentX, currentY)) {
          currentRot = nextRot;
          playMoveSound();
        }
      }
    }
  }
  lastBtnRotState = readingRot;

  // Hard Drop
  if (readingDrop != lastBtnDropState) lastDebounceTime[3] = now;
  if ((now - lastDebounceTime[3]) > debounceDelay) {
    if (readingDrop != btnDropState) {
      btnDropState = readingDrop;
      if (btnDropState == LOW) {
        while (!checkCollision(currentShape, currentRot, currentX, currentY + 1)) {
          currentY++;
        }
        lastDropTime = 0; // Force lock instantly
        playMoveSound();
      }
    }
  }
  lastBtnDropState = readingDrop;
}

void render() {
  display.clearDisplay();

  // Draw border (grid is 60x120, centered in 64x128)
  int offsetX = 2; // (64 - 60) / 2
  int offsetY = 4; // (128 - 120) / 2

  display.drawRect(offsetX - 1, offsetY - 1, BOARD_WIDTH * BLOCK_SIZE + 2, BOARD_HEIGHT * BLOCK_SIZE + 2, SSD1306_WHITE);

  // Draw board
  for (int r = 0; r < BOARD_HEIGHT; ++r) {
    for (int c = 0; c < BOARD_WIDTH; ++c) {
      if (board[r][c]) {
        display.fillRect(offsetX + c * BLOCK_SIZE, offsetY + r * BLOCK_SIZE, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
      }
    }
  }

  // Draw current piece
  if (!gameOver) {
    uint16_t shape = tetrominoes[currentShape][currentRot];
    for (int row = 0; row < 4; ++row) {
      for (int col = 0; col < 4; ++col) {
        if (shape & (1 << (15 - (row * 4 + col)))) {
          int py = currentY + row;
          int px = currentX + col;
          if (py >= 0) {
            display.fillRect(offsetX + px * BLOCK_SIZE, offsetY + py * BLOCK_SIZE, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
          }
        }
      }
    }
  } else {
    // Game over text
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 50);
    display.println("GAME");
    display.setCursor(5, 60);
    display.println("OVER");
    display.setCursor(5, 80);
    display.print("Scr:");
    display.println(score);
  }

  display.display();
}

void loop() {
  if (gameOver) {
    // Wait for any button to restart
    if (digitalRead(BUTTON_UP) == LOW || digitalRead(BUTTON_DOWN) == LOW || 
        digitalRead(BUTTON_LEFT) == LOW || digitalRead(BUTTON_RIGHT) == LOW) {
      delay(50); // debounce
      while(digitalRead(BUTTON_UP) == LOW || digitalRead(BUTTON_DOWN) == LOW || 
            digitalRead(BUTTON_LEFT) == LOW || digitalRead(BUTTON_RIGHT) == LOW);
      delay(50);
      
      // Reset game
      memset(board, 0, sizeof(board));
      score = 0;
      linesClearedTotal = 0;
      dropInterval = 800;
      gameOver = false;
      pixels.clear();
      pixels.show();
      spawnPiece();
    }
    return;
  }

  handleInput();

  unsigned long now = millis();
  if (now - lastDropTime > dropInterval) {
    lastDropTime = now;
    if (!checkCollision(currentShape, currentRot, currentX, currentY + 1)) {
      currentY++;
    } else {
      placePiece();
      checkLines();
      spawnPiece();
    }
  }

  render();
}
