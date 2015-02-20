// Connect 4
// P Beard
// Jan 2015

/*  Program currently compiles but will declare a win for the human player on the first move. Not sure why this is
    happening but I guess it must be an issue with the score or bestMove function.
    
    Aside from it not working it works fine :D...
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void displayBoard(uint8_t board[42]);
void dropCounter(uint8_t board[42], char token, uint8_t col);
int score(uint8_t board[42], char token, char oppToken);
int bestMove(uint8_t board[42], char token, char oppToken, uint8_t *bestCol, uint8_t lookAhead);

int main(void)
{
  uint8_t lookAhead, playerInput, gameIsPlaying, board[42], best, i;
  uint32_t currentScore;

  while(1)
  {
    gameIsPlaying = 1;
    system("clear");
    printf("\n~~~~~~Connect 4~~~~~~\n");
    for (i = 0; i < 42; i++)
    {
      board[i] = '.';
    }
    printf("Please select look ahead level 1-4 recomended.");
    scanf("%c", &lookAhead);
    getchar();//kill the return key.
    displayBoard(board);

    while(gameIsPlaying)
    {
      printf("Your go: ");
      scanf("%c", &playerInput);
      playerInput -= '0';//get numeric value instead of the charictar value
      getchar();//kill the return key

      dropCounter(board, 'O', playerInput);
      displayBoard(board);

      if (score(board, 'X', 'O') == -1000)
      {
        printf("\n****CONGRATULATIONS YOU WIN****\n");
        printf("Press return for new game");
        getchar();
        gameIsPlaying = 0;
        continue;
      }
      else
      {
        printf("My go: ");

        currentScore = bestMove(board, 'X', 'O', &best, lookAhead);
        printf("%d\n", best);

        dropCounter(board, 'X', best);
        displayBoard(board);
        printf("\n%d\n", currentScore);

        if (score(board, 'X', 'O') == 1000)
        {
          printf("\n****COMISERATIONS YOU LOST****\n");
          printf("Press return for new game");
          getchar();//wait for return press
          gameIsPlaying = 0;
          continue;
        }
      }
    }
  }
  return 0;
}

void displayBoard(uint8_t board[42])
{
  uint8_t position = 0, row, col;

  printf("|0|1|2|3|4|5|6|\n");
  for(row = 0; row <= 5; row++)
  {
    for(col = 0; col <= 6; col++)
    {
      printf("|%c", board[position]);
      position++;
    }
    printf("|\n");
  }
}

void dropCounter(uint8_t board[42], char token, uint8_t col)
{
  // Test if the position one row down is empty
  while((col < 35) && (board[col+7] == '.'))
  {
    col += 7;// Move down a row
  }
  board[col] = token;// Place the token here
}

int score(uint8_t board[42], char token, char oppToken)
{
  uint8_t winLineStart[25] = //start: Array containing start position of each "win line"
    { 0,  7, 14, 21, 28, 35,  0,  1,  2,  3,  4,  5,  6, 14,  7,  0,  1,  2,  3,  3,  4,  5,  6, 13, 20 };
  uint8_t winLineReps[25] = //reps: Array containing number of groups of 4 positions on each "win line"
    { 0,  7, 14, 21, 28, 35,  0,  1,  2,  3,  4,  5,  6, 14,  7,  0,  1,  2,  3,  3,  4,  5,  6, 13, 20 };
  uint8_t winLineSteps[25] =//steps: Array containing direction offset for each "win line"
    { 1,  1,  1,  1,  1,  1,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6 };
  uint8_t lineCount, lineStart, lineReps, pos, lineStep, position;
  int totalScore = 0, addScore = 0, testScore = 0;

  // Scan each "win" line on the board
  for (lineCount = 0; lineCount < 25; lineCount++)
  {
    lineStart = winLineStart[lineCount];
    lineStep = winLineSteps[lineCount];

    // Scan each group of 4 positions on the "win" line
    for(lineReps = 0; lineReps < winLineReps[lineCount]; lineReps++)
    {
      position = lineStart;

      // Check this group of 4 positions
      for(pos = 0; pos < 4; pos++)
      {
        testScore = addScore;
        if(board[position] == token)
        {
          if(testScore < 0)
          { // A mix of "O" and "X" - no-one can win here
            addScore = 0;
            break;
          }
          else
          {
            switch(testScore)
            {
            case 100:
              return 1000;
            case  10:
              addScore = 100;
              break;
            case   1:
              addScore = 10;
              break;
            case   0:
              addScore = 1;
              break;
            }
          }
        }
        if(board[position] == oppToken)
        {
          if(testScore > 0)
          { // A mix of "O" and "X" - no-one can win here
            addScore = 0;
            break;
          }
          else
          {
            switch(testScore)
            {
            case -100:
              return -1000;
            case  -10:
              addScore = -100;
              break;
            case   -1:
              addScore = -10;
              break;
            case    0:
              addScore = -1;
              break;
            }
          }
        }
        position += lineStep;// Move to next position in group of 4
      }
      lineStart += lineStep;// Move to next start position of next group-of-4 on the win line
      totalScore += addScore;// Add any score to the total score for the board
    }
  }
  return totalScore;
}

int bestMove(uint8_t board[42], char token, char oppToken, uint8_t *bestCol, uint8_t lookAhead)
{
  uint8_t col;
  int bestScore = -9999;

  // Evaluate each choice of column
  for (col = 0; col < 7; col++)
  {
    if (board[col] == '.')
    {
      int thisScore;

      // Make a copy of the board
      uint8_t boardCopy[42], i;
      for (i = 0; i < 42; i++)
      {
        boardCopy[i] = board[i];
      }

      // Drop token onto the copy of the board
      dropCounter(boardCopy, token, col);
      thisScore = score(boardCopy, token, oppToken);

      if (abs(thisScore) < 1000 && lookAhead > 0)
      {
        // Look more moves ahead, but swap places with opponent since it will be their turn next
        uint8_t oppBestCol;
        thisScore = -bestMove(boardCopy, oppToken, token, &oppBestCol, (lookAhead - 1));
      }

      // Check this choice of column - best so far?
      if (thisScore > bestScore)
      {
        // Yes, a new best choice found
        bestScore = thisScore;
        *bestCol = col;
      }
    }
  }
  return bestScore;
}
