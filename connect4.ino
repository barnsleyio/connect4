// Connect 4
// P Beard
// Jan 2015

#define LED 13
#define NO_BUTTONS 0b11111111111111

#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST); // Fast I2C / TWI

#include <Adafruit_NeoPixel.h>
#define NEOPIXELPIN A3
Adafruit_NeoPixel strip = Adafruit_NeoPixel(7, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);

const byte btnRow[2] = {2, 3};
const byte btnCol[7] = {4, 5, 6, 7, 8, 9, 10};
unsigned int btns = NO_BUTTONS;
unsigned int lastBtns = NO_BUTTONS;
unsigned int stableBtns = NO_BUTTONS;
unsigned long debounce;
char human = '-';
char robot = '-';
uint32_t humanColour = 0;
uint32_t robotColour = 0;
char board[42];
char screen[7][22];
// Array containing start position of each "win line"
byte lineStart[] = { 0, 7, 14, 21, 28, 35, 0, 1, 2, 3, 4, 5, 6, 14, 7, 0, 1, 2, 3, 3, 4, 5, 6, 13, 20};
// Array containing number of groups of 4 positions on each "win line"
byte lineReps[] = { 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 1, 2, 3, 3, 2, 1, 1, 2, 3, 3, 2, 1};
// Array containing direction offset for each "win line"
byte lineSteps[] = { 1, 1, 1, 1, 1, 1, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 6, 6, 6, 6, 6, 6};

void setup() {
  pinMode(LED, OUTPUT);
  //Serial.begin(9600);
  u8g.setFont(u8g_font_6x10);
  u8g.setFontPosTop();
  for (byte i = 0; i < 42; i++) board[i] = '.';
  for (byte i = 0; i < 7; i++) {
    for (byte j = 0; j < 21; j++) {
      screen[i][j] = '\0';
    }
  }
  strcpy(screen[0], "Your go:");
  displayBoard(board);
  updateScreen();
  pinMode(btnRow[0], INPUT_PULLUP);
  pinMode(btnRow[1], INPUT_PULLUP);
  for (byte col=0; col<=6; col++) {
    pinMode(btnCol[col], OUTPUT);
    digitalWrite(btnCol[col], LOW);
  }
  strip.begin();
  strip.show();
}

void loop() {
  char c;
  byte best;
  int currentScore;
  unsigned int btnsNow = 0;
  // Read all buttons in the 7 cols x 2 rows matrix
  for (byte row=0; row<=1; row++) {
    for (byte col=0; col<=6; col++) {
      pinMode(btnCol[col], INPUT);
      btnsNow = btnsNow << 1 | digitalRead(btnRow[row]);
      pinMode(btnCol[col], OUTPUT);
      digitalWrite(btnCol[col], LOW);
    }
  }
  // Has there been any change in the buttons?
  if (btnsNow != lastBtns) {
    // Note the new button values and the time of the change
    lastBtns = btnsNow;
    debounce = millis();
  }
  // Do we have a new, stable reading?
  if ((millis() - debounce) > 50 && btnsNow != stableBtns) {
    stableBtns = btnsNow;
    btns &= btnsNow;
  }
  // Have we detected some button presses and the timeout period for further changes has expired?
  if ((millis() - debounce) > 500 && btns != NO_BUTTONS) {
    // Work out which column the counter was placed in. Zero indicates cheating!
    unsigned int col = btns & btns >> 7;
    if (col == 0) {
      strcpy(screen[0], "Cheat!");
      updateScreen();
    }
    else {
      // Figure out which colour counter has been dropped
      char counter;
      if ((btns & 0b1111111) == col) counter = 'O'; 
      else counter = 'X';
      // Has human chosen a colour yet?
      if (human == '-') {
        // Human has now chosen a colour and robot chooses the other
        human = counter;
        if (human == 'X') {
          humanColour = strip.Color(32, 32, 0);
          robot = 'O'; 
          robotColour = strip.Color(0, 0, 64);
        }
        else {
          humanColour = strip.Color(0, 0, 64);
          robot = 'X';
          robotColour = strip.Color(32, 32, 0);
        }
      }
      // Has human played with robot's colour?
      if (counter != human) {
        strcpy(screen[0], "Cheat!");
        updateScreen();
      }
      else {
        // Human has played correct colour. Figure out which column
        for (byte i=0; i<=6; i++) {
          strip.setPixelColor(i, 0);
          if (bit(i) == col) {
            c = 6-i;
          }
        }
        // Drop the counter
        strip.setPixelColor(c, humanColour);
        strip.show();
        dropCounter(board, human, c);
        displayBoard(board);
        updateScreen();
        if (score(board, human, robot) == 1000) {
          strcpy(screen[0], "You Won!");
          updateScreen();
          while (true) {
          }
        }
        else {
          //unsigned long t = millis();
          strcpy(screen[0], "My go:");
          updateScreen();
          currentScore = bestMove(board, robot, human, best, 4);
          digitalWrite(LED, LOW);
          //Serial.println(millis() - t);
          for (byte i=0; i<=6; i++) {
            strip.setPixelColor(i, 0);
          }
          strip.setPixelColor(best, robotColour);
          strip.show();
          dropCounter(board, robot, best);
          displayBoard(board);
          if (score(board, robot, human) == 1000) {
            strcpy(screen[0], "You Lost!");
            updateScreen();
            while (true) {
            }
          }
          strcpy(screen[0], "Your go:");
          updateScreen();
        }
      }
    }
    btns = NO_BUTTONS;
  }
}

void displayBoard(char board[]) {
  Serial.println("0123456");
  byte i = 0;
  for(byte row = 0; row <= 5; row++) {
    for(byte col = 0; col <= 6; col++) {
      screen[row+1][col] = board[i++];
    }
  }
  updateScreen();
}

void updateScreen() {
  // U8GLIB picture loop
  u8g.firstPage();
  do {
    uint8_t i, y;
    y = 0; // reference is the top left -1 position of the string
    y--; // correct the -1 position of the drawStr
    for( i = 0; i < 7; i++ ) {
      u8g.drawStr( 0, y, (char *)(screen[i]));
      y += u8g.getFontLineSpacing();
    }
  } 
  while( u8g.nextPage() );
}

byte dropCounter(char board[], char token, byte col) {
  byte i = col;
  // Test if the position one row down is empty
  while (i < 35 && board[i+7] == '.') {
    // Move down a row
    i += 7;
  }
  // Place the token here
  board[i] = token;
  return i;
}

int score(char board[], char token, char oppToken) {
  int totalScore = 0;
  // Scan each "win" line on the board
  for (byte line = 0; line < 25; line++) {
    byte i = lineStart[line];
    byte lineStep = lineSteps[line];
    byte reps = lineReps[line];
    // Scan each group of 4 positions on the "win" line
    for (byte rep = 0; rep < reps; rep++) {
      byte j = i;
      int addScore = 0;
      // Check this group of 4 positions
      for (byte pos = 0; pos < 4; pos++) {
        if (board[j] == token) {
          if (addScore < 0) {
            // A mix of "O" and "X" - no-one can win here
            addScore = 0;
            break;
          }
          else if (addScore == 0) addScore = 1;
          else if (addScore == 1) addScore = 10;
          else if (addScore == 10) addScore = 100;
          else if (addScore == 100) {
            // The game has been won
            totalScore = 1000;
            goto gameWon;
          }
        }
        else if (board[j] == oppToken) {
          if (addScore > 0) {
            // A mix of "O" and "X" - no-one can win here
            addScore = 0;
            break;
          }
          else if (addScore == 0) addScore = -1;
          else if (addScore == -1) addScore = -10;
          else if (addScore == -10) addScore = -100;
          else if (addScore == -100) {
            // The game has been won
            totalScore = -1000;
            goto gameWon;
          }
        }
        // Move to next position in group of 4
        j += lineStep;
      }
      // Move to next start position of next group-of-4 on the win line
      i += lineStep;
      // Add any score to the total score for the board
      totalScore += addScore;
    }
  }
gameWon: 
  return totalScore;
}

int bestMove(char board[], char token, char oppToken, byte &bestCol, byte lookAhead) {
  int bestScore = -9999;
  // Make a copy of the board
  char boardCopy[43];
  for (byte i = 0; i < 43; i++) boardCopy[i] = board[i];
  if (lookAhead > 1) digitalWrite(LED, !digitalRead(LED));
  // Evaluate each choice of column
  for (byte col = 0; col < 7; col++) {
    if (board[col] == '.') {
      // Drop token onto the copy of the board
      byte counterPosition = dropCounter(boardCopy, token, col);
      int thisScore = score(boardCopy, token, oppToken);
      if (thisScore > -1000 && thisScore < 1000 && lookAhead > 0) {
        // Look more moves ahead, but swap places with opponent since it will be their turn next
        byte oppBestCol;
        thisScore = -bestMove(boardCopy, oppToken, token, oppBestCol, lookAhead - 1);
      }
      // Check this choice of column - best so far?
      if (thisScore > bestScore) {
        // Yes, a new best choice found
        bestScore = thisScore;
        bestCol = col;
      }
      // remove the counter again
      boardCopy[counterPosition] = '.';
      // check for any button presses = cheating
      if (digitalRead(btnRow[0]) == LOW || digitalRead(btnRow[1]) == LOW) {
        strcpy(screen[0], "Cheat!");
        updateScreen();
      }
    }
  }
  return bestScore;
}
