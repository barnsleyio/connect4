// Connect 4
// For Wemos D1 mini
// P Beard
// Nov 2016
// Uses ATtiny84 running SwitchMatrixScanner scetch to scan switch matrix

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

#define SLAVE_RX D1
#define SLAVE_TX D2

// Serial connection with slave MCU
SoftwareSerial slaveMCU(SLAVE_RX, SLAVE_TX, false, 64);

extern "C" {
#include "user_interface.h"
}

#define LED D4
#define NO_BUTTONS 0b1111111111111111
#define NEOPIXELPIN D5
//#define PCF_POWER D3
//#define SWITCH_MATRIX 0x20

Adafruit_NeoPixel strip = Adafruit_NeoPixel(7, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);
WiFiServer server(80);

//const char* ssid = "granary";
//const char* password = "sparkym00se";
const char* ssid = "MYWIFI5B75";
const char* password = "MYWIFI5063";

unsigned int btns = NO_BUTTONS;
unsigned int lastBtns = NO_BUTTONS;
unsigned int stableBtns = NO_BUTTONS;
unsigned long debounce;
long movesConsidered = 0;

const char token[2] = { 'O', 'X' };
enum {human, robot} whosTurn = human;
byte robotMove;
enum {ongoing, won, cheatingDetected} gameState = ongoing;

const byte blue = 0;
const byte yellow = 1;
const byte red = 2;
const uint32_t neoPixelColour[3] = { strip.Color(0, 0, 64), strip.Color(32, 32, 0), strip.Color(64, 0, 0) };
byte playerColour[2] = {0, 0};
unsigned long lastUpdate;
int animPos = 0;
int animDir = 1;

char board[42];
// Array containing start position of each "win line"
byte lineStart[] = { 0, 7, 14, 21, 28, 35, 0, 1, 2, 3, 4, 5, 6, 14, 7, 0, 1, 2, 3, 3, 4, 5, 6, 13, 20};
// Array containing number of groups of 4 positions on each "win line"
byte lineReps[] = { 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 1, 2, 3, 3, 2, 1, 1, 2, 3, 3, 2, 1};
// Array containing direction offset for each "win line"
byte lineSteps[] = { 1, 1, 1, 1, 1, 1, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 6, 6, 6, 6, 6, 6};

/*
  byte readSwitchRow(byte rowMask) {

  //The switches are arranged in a 4x4 matrix connected to a pcf8574 i2c i/o chip
  //The matrix rows are connected to pins P4-P7 and columns to pins P0-P3
  //rowMask should contain a zero bit in the position for the row to be read
  //If all 4 row bits in rowMask are zero, all rows are read simulaneously, which means you can tell
  //if any switch in a column is pressed, but not which row it is in.

  Wire.beginTransmission(SWITCH_MATRIX);
  Wire.write(rowMask);
  Wire.endTransmission();
  Wire.requestFrom(SWITCH_MATRIX, 1, true);
  //Wait for the response from the pcf chip
  while (Wire.available() == 0);
  //Get the result and mask off the row mask
  return (Wire.read() & 0b00001111);

  }


  unsigned int readSwitches() {

  //First do a quick check to see if any switches are pressed
  if (readSwitchRow(0b00001111) == 0b00001111) {
    //No switches pressed
    return NO_BUTTONS;
  }
  else {
    //One or more switches pressed. Scan the matrix by row to get the full picture
    unsigned int s = 0;
    s = (s << 4) | readSwitchRow(0b11011111);
    s = (s << 4) | readSwitchRow(0b01111111);
    s = (s << 4) | readSwitchRow(0b11101111);
    s = (s << 4) | readSwitchRow(0b10111111);
    //In the result, a zero bit indicates that the switch is pressed
    return s;
  }

  }
*/

unsigned int readSwitches() {

  static unsigned int currentSwitchStates = NO_BUTTONS;
  int maxTries = 0;

  while (maxTries == 0) {
    // Request switch status
    slaveMCU.print("?");
    maxTries = 10;
    while (slaveMCU.available() < 2 && maxTries > 0) {
      delay(1);
      maxTries--;
    }
  }

  byte l = slaveMCU.read();
  byte h = slaveMCU.read();
  if (currentSwitchStates != word(h, l)) {
    currentSwitchStates = word(h, l);
    Serial.print("Button readings received: ");
    Serial.println(currentSwitchStates, BIN);
  }

  return currentSwitchStates;
}

byte dropCounter(char board[], char token, byte col) {

  byte i = col;
  // Test if the position one row down is empty
  while (i < 35 && board[i + 7] == '.') {
    // Move down a row
    i += 7;
  }
  // Place the token here
  board[i] = token;
  return i;

}

void printBoard() {

  Serial.println("Counter dropped:");
  for (int row = 0; row < 6; row++) {
    for (int col = 0; col < 7; col++) {
      Serial.print(board[row * 7 + col]);
    }
    Serial.println();
  }

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

int bestMove(char board[], char token, char oppToken, byte & bestCol, byte lookAhead) {

  int bestScore = -9999;

  movesConsidered++;
  // Make a copy of the board
  char boardCopy[43];
  for (byte i = 0; i < 43; i++) boardCopy[i] = board[i];
  if (lookAhead == 2) {
    digitalWrite(LED, !digitalRead(LED));
    yield();
  }
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
     // if (readSwitches() != NO_BUTTONS) {
        //Serial.println("Cheat! You dropped a counter when robot was thinking.");
        //gameState = cheatingDetected;
      //}
    }
  }
  return bestScore;

}

void clearGame() {
  for (byte i = 0; i < 42; i++) board[i] = '.';
  gameState = ongoing;
}

void setup() {

  Serial.begin(115200);
  slaveMCU.begin(57600);
  //Power up the  pcf chip
  //pinMode(PCF_POWER, OUTPUT);
  //digitalWrite(PCF_POWER, HIGH);
  //Wire.begin();
  pinMode(LED, OUTPUT);
  strip.begin();
  //strip.show();

  //WiFi.softAP(ssid, password, 8);

  //IPAddress myIP = WiFi.softAPIP();
  //Serial.print("AP IP address: ");
  //Serial.println(myIP);
  //server.on("/", handleRoot);
  //server.begin();
  //Serial.println("HTTP server started");

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  clearGame();
  Serial.println("Your go:");

}

void handleWebPageRequests() {

  // Check if a web client has connected
  WiFiClient client = server.available();
  if (client) {

    // Wait until the client sends some data
    Serial.print("new client request: ");
    //int timeout = 3000;
    //while (!client.available() && timeout > 0) {
    //  delay(1);
    //  timeout--;

    //}

    //if (timeout == 0) {
    //  Serial.println("timed out!");
    //}
    //else {

    // Return the response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Refresh: 5");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>Connect 4</h1>");
    client.println("<table border=1 style='font-family:monospace;'>");
    client.println();
    byte i = 0;
    for (byte row = 0; row <= 5; row++) {
      client.print("<TR>");
      for (byte col = 0; col <= 6; col++) {
        client.print("<TD>");
        if (board[i] == 'O') client.print("<font color='orange'>");
        else if (board[i] == 'X') client.print("<font color='blue'>");
        client.print(board[i]);
        if (board[i] != '.') client.print("</font>");
        client.print("</TD>");
        i++;
      }
      client.print("</TR>");
    }
    client.println("</table>");

    if (gameState == ongoing) {
      if (whosTurn == human) {
        client.print("<p>Your move!");
      }
      else {
        client.print("<p>Please drop counter for robot in column ");
        client.print(robotMove + 1);
        client.print(".");
      }
    }
    else if (gameState == won) {
      if (whosTurn == human) {
        client.print("<p>You won!");
      }
      else {
        client.print("<p>You lost!");
      }
    }
    else if (gameState == cheatingDetected) {
      client.print("<p>Cheating Detected!");
    }

    client.println("</body>");
    client.println("</html>");

    client.flush();

    Serial.println("Client disonnected");
    Serial.println("");
  }
  //}
}

void animateLeds() {

  if (millis() - lastUpdate > 100) {
    lastUpdate = millis();
    uint32_t colour;
    if (gameState == ongoing) {
      if (whosTurn == human) {
        // switch off previously animated led
        strip.setPixelColor(animPos, 0);
        // move to next animation position
        if (animDir == 1 && animPos == 6 || animDir == -1 && animPos == 0) animDir = - animDir;
        animPos += animDir;
        // Have player colours been chosen yet?
        if (playerColour[human] == 0 && playerColour[robot] == 0)
          // alternate colours
          colour = neoPixelColour[animPos & 1];
        else
          // use player's chosen colour
          colour = neoPixelColour[playerColour[whosTurn]];
        // light the next led in the animation
        strip.setPixelColor(animPos, colour);
      }
    }
    else {
      colour = 0;
      if (gameState == cheatingDetected) colour = neoPixelColour[red];
      else if (animDir > 0) colour = neoPixelColour[playerColour[whosTurn]];
      for (byte c = 0; c <= 6; c++) strip.setPixelColor(c, colour);
      animDir = - animDir;
    }
    strip.show();
  }

}


void loop() {

  int columnActivated = -1;
  byte counter = yellow;
  int currentScore;

  handleWebPageRequests();
  animateLeds();

  unsigned int btnsNow = readSwitches();
  // Has there been any change in the buttons?
  if (btnsNow != lastBtns) {
    Serial.print("Button change detected @ ");
    Serial.println(millis());
    // Note the new button values and the time of the change
    btns &= btnsNow;
    lastBtns = btnsNow;
    debounce = millis();
  }
  // Do we have a new, stable reading?
  //if ((millis() - debounce) > 5 && btnsNow != stableBtns) {
    //stableBtns = btnsNow;
    //btns &= btnsNow;
    //Serial.print("Updating stable buttons @ ");
    //Serial.println(millis());
  //}
  // Have we detected some button presses and the timeout period for further changes has expired?
  if ((millis() - debounce) > 500 && btnsNow == NO_BUTTONS && btns != NO_BUTTONS) {
    Serial.print("btnsNow="); Serial.print(btnsNow, BIN);
    Serial.print(" btns="); Serial.print(btns, BIN);
    Serial.print(" stableBtns="); Serial.println(stableBtns, BIN);
    // Work out which column the counter was placed in.
    for (byte b = 0; b <= 7; b++) {
      if (bitRead(btns, b) == 0) {
        columnActivated = 7 - b;
        counter = blue;
        Serial.print("blue counter dropped in column ");
        Serial.println(columnActivated);
      }
      else if (bitRead(btns, b + 8) == 0) {
        columnActivated = 7 - b;
        counter = yellow;
        Serial.print("yellow counter dropped in column ");
        Serial.println(columnActivated);
      }
    }
    btns = NO_BUTTONS;
  }

  if (columnActivated >= 0) {

    // Have player colours been chosen yet?
    if (playerColour[human] == 0 && playerColour[robot] == 0) {
      // Colours have now been chosen
      playerColour[human] = counter;
      playerColour[robot] = 1 - counter;
    }

    // Has the correct colour token been dropped?
    if (counter != playerColour[whosTurn]) {
      Serial.println("Cheat! You played the wrong colour of counter.");
      //updateScreen();
      gameState = cheatingDetected;
    }
    else {
      // Drop the counter
      Serial.println("Dropping Counter");
      dropCounter(board, token[whosTurn], columnActivated);
      printBoard();

      //displayBoard(board);
      //updateScreen();

      // Has the game been won?
      currentScore = score(board, 'O', 'X');
      if (currentScore == 1000) {
        Serial.println("You Won!");
        //updateScreen();
        gameState = won;
      }
      else if (currentScore == -1000) {
        Serial.println("You lost!");
        //updateScreen();
        gameState = won;
      }
      else {


        if (whosTurn == human) {

          // Indicate the move the human made
          strip.setPixelColor(columnActivated, neoPixelColour[playerColour[human]]);
          strip.show();

          // The robot takes a go
          whosTurn = robot;
          Serial.println("My go:");
          //updateScreen();

          // Robot decides its move
          Serial.print("Robot is thinking...");
          unsigned long start = millis();
          movesConsidered = 0;
          currentScore = bestMove(board, 'X', 'O', robotMove, 5);
          Serial.print("OK, it considered ");
          Serial.print(movesConsidered);
          Serial.print(" moves in ");
          Serial.print(millis() - start);
          Serial.print("ms and chose column ");
          Serial.println(robotMove);
          digitalWrite(LED, HIGH);

          // Robot indicates its chosen move and waits for counter to drop
          strip.setPixelColor(columnActivated, 0);
          strip.setPixelColor(robotMove, neoPixelColour[playerColour[robot]]);
          strip.show();

          strip.show();
        }
        else {

          // it is still the robot's go
          // Has the counter been dropped into the robot's chosen column?
          if (columnActivated >= 0 && columnActivated != robotMove) {
            Serial.println("Cheat! You dropped robot's counter in wrong column.");
            //updateScreen();
            gameState = cheatingDetected;
          }
          else {
            // Clear the led indicating the robot's move
            strip.setPixelColor(robotMove, 0);
            strip.show();

            // Humans's turn
            whosTurn = human;
            Serial.println("Your go:");
            //updateScreen();
          }
        }
      }
    }
  }
}


