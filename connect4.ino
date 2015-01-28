// Connect 4
// P Beard
// Jan 2015

// Used as array of bits for positions of "X" tokens
unsigned long long boardO;
// Used as array of bits for positions of "O" tokens
unsigned long long boardX;

// Array containing start position of each "win line"
byte lineStart[] = { 0,  7, 14, 21, 28, 35,  0,  1,  2,  3,  4,  5,  6, 14,  7,  0,  1,  2,  3,  3,  4,  5,  6, 13, 20};
// Array containing number of groups of 4 positions on each "win line"
byte lineReps[] =  { 4,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  3,  1,  2,  3,  3,  2,  1,  1,  2,  3,  3,  2,  1};
// Array containing direction offset for each "win line"
byte lineSteps[] = { 1,  1,  1,  1,  1,  1,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6};

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("~~~~~~Connect 4~~~~~~");
  Serial.println();
  displayBoard(boardO, boardX);
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
  dropCounter(boardX, boardO, c - '0');
  displayBoard(boardO, boardX);
  
  if (score(boardO, boardX) < -500) {
    Serial.println();
    Serial.println ("****CONGRATULATIONS YOU WIN****");
    Serial.println();
    Serial.println("Press reset for new game");
    while (true) {}
  }
  else {
    
    Serial.println();
    Serial.print("My go:");
    currentScore = bestMove(boardO, boardX, best, 3);
    Serial.println(best);
    Serial.println();
    dropCounter(boardO, boardX, best);
    displayBoard(boardO, boardX);
    
    if (score(boardO, boardX) > 500) {
      Serial.println();
      Serial.println ("****COMISERATIONS YOU LOST****");
      Serial.println();
      Serial.println("Press reset for new game");
      while (true) {}
    }
  }
}

void displayBoard(unsigned long long boardO, unsigned long long boardX) {
  
  unsigned long long mask = 1ULL;
  
  for(byte col = 0; col <= 6; col++) Serial.print(col);
  Serial.println();
  
  for(byte row = 0; row <= 5; row++) {
    for(byte col = 0; col <= 6; col++) {
      if (boardO & mask) Serial.print('O');
      else if (boardX & mask) Serial.print('X');
      else Serial.print('.');
      mask = mask << 1;
    }
    Serial.println();
  }
}

void dropCounter(unsigned long long &boardO, unsigned long long &boardX, byte col) {
  
  unsigned long long mask = 1ULL << col;
  byte row = 0;
  
  while (row < 5) {
    // Test if the position one row down is empty
    unsigned long long mask2 = mask << 7;
    if (((boardO | boardX) & mask2)) break;
    // Move down a row
    row++;
    mask = mask2;
  }
  // Place the token here
  boardO |= mask;
}

int score(unsigned long long &boardO, unsigned long long &boardX) {
  
  int totalScore = 0;
  
  // Scan each "win" line on the board
  for (byte line = 0; line < 25; line++) {
    
    unsigned long long mask = 1ULL << lineStart[line];
    byte lineStep = lineSteps[line];
  
    // Scan each group of 4 positions on the "win" line
    for (byte rep = 0; rep < lineReps[line]; rep++) {
      
      unsigned long long mask2 = mask;
      int addScore = 0;
      
      // Check this group of 4 positions
      for (byte pos = 0; pos < 4; pos++) {
        
        if (boardO & mask2) {
          if (addScore < 0) {
            // A mix of "O" and "X" - no-one can win here
            addScore = 0;
            break;
          }
          else if (addScore == 0) addScore = 1;
          else if (addScore == 1) addScore = 10;
          else if (addScore == 10) addScore = 100;
          else if (addScore == 100) addScore = 1000;
        }
  
        if (boardX & mask2) {
          if (addScore > 0) {
            // A mix of "O" and "X" - no-one can win here
            addScore = 0;
            break;
          }
          else if (addScore == 0) addScore = -1;
          else if (addScore == -1) addScore = -10;
          else if (addScore == -10) addScore = -100;
          else if (addScore == -100) addScore = -1000;
        }
        // Move to next position in group of 4
        mask2 = mask2 << lineStep;
      }
      // Move to next start position of next group-of-4 on the win line
      mask = mask << lineStep;
      // Add any score to the total score for the board
      totalScore += addScore;
    }
  }
 
  return totalScore;
}

int bestMove(unsigned long long &boardO, unsigned long long &boardX, byte &best, byte lookAhead) {
  
  int bestScore = -9999;
  byte best2;
  
  // Evaluate each choice of column
  for (byte col = 0; col < 7; col++) {
    
    // Make a copy of the board
    unsigned long long boardOcopy = boardO;
    unsigned long long boardXcopy = boardX;
    int thisScore;
    // Drop token onto the copy of the board    
    dropCounter(boardOcopy, boardXcopy, col);
    
    if (lookAhead == 0) 
      // Don't look any more moves ahead, just evaluate the board as-is
      thisScore = score(boardOcopy, boardXcopy);
    else
      // Look more moves ahead, but swap places with opponent since it will be their turn next
      thisScore = -bestMove(boardXcopy, boardOcopy, best2, lookAhead - 1);
      
    // Check this choice of column - best so far?
    if (thisScore > bestScore) {
      // Yes, a new best choice found
      bestScore = thisScore;
      best = col;
    }
  }
  return bestScore;
}

