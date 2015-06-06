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

const byte human = 0;
const byte robot = 1;
const char token[2] = { 'O', 'X' };
byte whosTurn = human;
byte robotMove;

const byte blue = 0;
const byte yellow = 1;
const uint32_t neoPixelColour[2] = { strip.Color(0,0,64), strip.Color(32,32,0) };
byte playerColour[2] = {0, 0};

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
      byte counter;
      if ((btns & 0b1111111) == col) counter = blue; else counter = yellow;
      
      // Have player colours been chosen yet?
      if (playerColour[human] == 0 && playerColour[robot] == 0) {
        // Colours have now been chosen
        playerColour[human] = counter;
        playerColour[robot] = 1 - counter;
      }
      
      // Has the correct colour token been dropped?
      if (counter != playerColour[whosTurn]) {
        strcpy(screen[0], "Cheat!");
        updateScreen();
      }
      else {
        
        // The correct colour has bee dropped. Figure out which column
        for (byte i=0; i<=6; i++) {
          strip.setPixelColor(i, 0);
          if (bit(i) == col) {
            c = 6-i;
          }
        }
        // Drop the counter
        dropCounter(board, token[whosTurn], c);
        displayBoard(board);
        updateScreen();
        
        // Has the game been won?
        currentScore = score(board, 'O', 'X');
        if (currentScore == 1000) {
          strcpy(screen[0], "You Won!");
          updateScreen();
          while (true) {}
        }
        else if (currentScore == -1000) {
          strcpy(screen[0], "You lost!");
          updateScreen();
          while (true) {}
        }
        else {
          
          // The game continues
          if (whosTurn == human) {
            
            // Indicate the move the human made
            strip.setPixelColor(c, neoPixelColour[playerColour[human]]);
            strip.show();

            // The robot takes a go
            whosTurn = robot;
            strcpy(screen[0], "My go:");
            updateScreen();
            
            // Robot decides its move
            currentScore = bestMove(board, 'X', 'O', robotMove, 4);
            digitalWrite(LED, LOW);
            
            // Robot indicates its chosen move and waits for counter to drop
            strip.setPixelColor(robotMove, neoPixelColour[playerColour[robot]]);
            strip.show();
          }
          else {
            
            // it is still the robot's go
            // Has the counter been dropped into the robot's chosen column?
            if (c != robotMove) {
              strcpy(screen[0], "Cheat!");
              updateScreen();
            }
            else {
              // Clear the led indicating the robot's move
              strip.setPixelColor(robotMove, 0);
              strip.show();
              
              // Humans's turn
              whosTurn = human;
              strcpy(screen[0], "Your go:");
              updateScreen();
            }
          }
        }
      }
    btns = NO_BUTTONS;
    }
  }
}

void displayBoard(char board[]) {
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
