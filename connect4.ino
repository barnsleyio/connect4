// Connect 4
// P Beard
// Jan 2015

char board[43];

// Array containing start position of each "win line"
byte lineStart[] = { 0,  7, 14, 21, 28, 35,  0,  1,  2,  3,  4,  5,  6, 14,  7,  0,  1,  2,  3,  3,  4,  5,  6, 13, 20};
// Array containing number of groups of 4 positions on each "win line"
byte lineReps[] =  { 4,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  3,  1,  2,  3,  3,  2,  1,  1,  2,  3,  3,  2,  1};
// Array containing direction offset for each "win line"
byte lineSteps[] = { 1,  1,  1,  1,  1,  1,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6};

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("~~~~~~Connect 4~~~~~~");
  Serial.println();
  for (byte i = 0; i < 43; i++) board[i] = '.';
  displayBoard(board);
}

void loop() {
  char c;
  byte best;
  int currentScore;
  
  Serial.println();
  Serial.print("Your go:");
  while (Serial.available() < 1) {} // wait for key code
  c = Serial.read();
  Serial.println(c);
  Serial.println();
  while (Serial.available() < 1) {} // wait for CR code
  Serial.read(); // discard CR code
  dropCounter(board, 'O', c - '0');
  displayBoard(board);
  
  if (score(board, 'X', 'O') == -1000) {
    Serial.println();
    Serial.println ("****CONGRATULATIONS YOU WIN**** ");
    Serial.println();
    Serial.println("Press reset for new game");
    while (true) {}
  }
  else {
    
    Serial.println();
    Serial.print("My go:");
    unsigned long t = millis();
    currentScore = bestMove(board, 'X', 'O', best, 4);
    Serial.println(millis() - t);
    Serial.println(best);
    Serial.println();
    dropCounter(board, 'X', best);
    displayBoard(board);
    
    if (score(board, 'X', 'O') == 1000) {
      Serial.println();
      Serial.println ("****COMISERATIONS YOU LOST****");
      Serial.println();
      Serial.println("Press reset for new game");
      while (true) {}
    }
  }
}

void displayBoard(char board[]) {
  
  Serial.println("0123456");
  
  byte i = 0;
  
  for(byte row = 0; row <= 5; row++) {
    for(byte col = 0; col <= 6; col++) {
      Serial.print(board[i++]);
    }
    Serial.println();
  }
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
  gameWon: return totalScore;
}

int bestMove(char board[], char token, char oppToken, byte &bestCol, byte lookAhead) {
  
  int bestScore = -9999;
  
  // Make a copy of the board
  char boardCopy[43];
  for (byte i = 0; i < 43; i++) boardCopy[i] = board[i];
  
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
    }
  }
  return bestScore;
}





