//  Concepts used:
//    Structs          (struct Player, struct Enemy)
//    Arrays           (2D char maze)
//    Pointers         (struct Player *p, -> operator)
//    Loops            (while, do-while, for)
//    Conditions       (if/else if)
//    Switch           (key input, ghost direction)
//    Functions        (modular functions)
//    Strings & I/O    (scanf, printf, sprintf, strcpy)
//    File I/O         (fopen, fprintf, fscanf)
//    Recursion        (recursive pellet counter)
//    #define constants
//    Random numbers   (rand, srand)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>       // kbhit(), getch() — Windows only 
#include <windows.h>     //COORD, CONSOLE_CURSOR_INFO, Sleep() 

//   THESE ARE ALL EASY-TO-CHANGE CONSTANTS IN ONE PLACE

//Maze size 

#define ROWS        10          // number of rows in the maze            
#define COLS        19          // number of columns in the maze         

//     Player starting position (row 1, col 1 — just inside top-left corner) 

#define PACMAN_START_X   1          // Pac-Man's starting column             
#define PACMAN_START_Y   1          // Pac-Man's starting row                

//     Ghost starting position (bottom-right open area) 
#define GHOST_START_X   17          // ghost's starting column               
#define GHOST_START_Y    8          // ghost's starting row                  

//   Ghost movement 
#define GHOST_DIRECTIONS 4          // possible moves: up / down / left / right 

//  Game speed 
#define GAME_SPEED_MS  150          // milliseconds per frame; lower = faster   
#define EXIT_DELAY_MS 1500          // pause before the program closes (ms)     

//   Player name buffer 
#define NAME_LEN        30          // max characters for a player name      

// Score display buffer size 
#define SCORE_BUF_LEN   50          // size of the score text buffer         

//  High score file 
#define HIGHSCORE_FILE  "highscore.txt"   // file where the best score is saved 

//   STRUCTS

struct Player
{
    int  x;
    int  y;
    int  points;
    char name[NAME_LEN];    // uses NAME_LEN constant
};

struct Enemy
{
    int x;
    int y;
};

// GLOBAL OBJECTS
// Start positions useS named constants instead of raw numbers.

struct Player pacman = {PACMAN_START_X, PACMAN_START_Y, 0, "Pacman"};
struct Enemy  ghost  = {GHOST_START_X,  GHOST_START_Y};


//   MAZE GRIDS
//   +1 on COLS for the null terminator '\0' that ends every C string row.
//  'P' = Pac-Man start,  'G' = Ghost start,  '.' = pellet,  '#' = wall

char maze[ROWS][COLS + 1] =
{
    "###################",
    "#P....#...........#",
    "#.###.#.####.###..#",
    "#.....#....#......#",
    "#.########.#.###..#",
    "#.................#",
    "#.###.#####.###...#",
    "#...#.......#.....#",
    "#...............G.#",
    "###################"
};

char originalMaze[ROWS][COLS + 1]; // backup copy is restored by resetGame() 


//   FORWARD DECLARATIONS
//   It tells the compiler these functions exist before we define them,
//   so saveHighScore() can call loadHighScore() even though
//   loadHighScore() is written after it.
 
int  loadHighScore(void);
void saveHighScore(struct Player *p, int current);


//   gotoxy — moves the console cursor to column x, row y.
//   Why we need this:
//     Instead of clearing the whole screen every frame (which
//     causes flicker), we jump back to (0,0) and overwrite the
//     same characters.  The player sees a smooth redraw.

void gotoxy(int x, int y)
{
    // COORD is a Windows struct that stores (column, row) 
    COORD coord;
    coord.X = x;    // column 
    coord.Y = y;    // row    


//      GetStdHandle(STD_OUTPUT_HANDLE) returns the handle (reference)
//      to the console window.  SetConsoleCursorPosition then moves
//      the blinking cursor to the coordinate we gave it.
 
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}


//   hideCursor — makes the blinking text cursor invisible.
//   Why: without this, the cursor jumps around the screen
//   every frame as we redraw the maze, which looks messy.

void hideCursor(void)
{ 
//      CONSOLE_CURSOR_INFO holds two settings:
//        dwSize   — cursor size as a % (1 = smallest possible)
//       bVisible — TRUE shows it, FALSE hides it
     
    CONSOLE_CURSOR_INFO info;
    info.dwSize   = 1;
    info.bVisible = FALSE;  // FALSE is a Windows constant equal to 0 

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}


//   saveHighScore — writes the player's score to a text file
//                   only if it beats the current best score.
//   Parameter:
//     p       — pointer to the Player struct (uses -> operator)
//     current — the score to compare and possibly save

void saveHighScore(struct Player *p, int current)
{
    int best = loadHighScore();   // read the existing best first 

    if (current > best)           // only overwrite if new score is better 
    {
        // "w" creates the file if missing; overwrites if it already exists 
        FILE *fp = fopen(HIGHSCORE_FILE, "w");
        if (fp != NULL)
        {
            // Write one line: "PlayerName Score" */
            fprintf(fp, "%s %d\n", p->name, current);
            fclose(fp);
            printf("\n  *** New High Score saved: %d ***\n", current);
        }
    }
}

//   loadHighScore — reads the best score from the text file.
//   Returns 0 if the file does not exist yet (first run).

int loadHighScore(void)
{
    int  score = 0;
    char savedName[NAME_LEN];   // temporary — we only use the number 

    // "r" = open for reading; returns NULL if file does not exist 
    FILE *fp = fopen(HIGHSCORE_FILE, "r");
    if (fp != NULL)
    {
        fscanf(fp, "%29s %d", savedName, &score);
        fclose(fp);
    }
    return score;   // 0 if file did not exist 
}

//   countPelletsRecursive — counts remaining '.' pellets in the
//   maze using recursion instead of nested for-loops.
// 
//   Parameters:
//     r — current row  (start at 0)
//     c — current col  (start at 0)
// 
//   How recursion works here:
//     Base case 1: r == ROWS  ? all rows checked, return 0
//     Base case 2: c == COLS  ? end of row, move to next row
//     Recursive step: count this cell + recurse to next column

int countPelletsRecursive(int r, int c)
{
    // Base case: we have checked every row 
    if (r == ROWS)
        return 0;

    // End of this row — wrap to the beginning of the next row 
    if (c == COLS)
        return countPelletsRecursive(r + 1, 0);

    // 1 if this cell has a pellet, 0 if not 
    int thisCellHasPellet = (maze[r][c] == '.') ? 1 : 0;

    // Add this cell's result and keep scanning to the right 
    return thisCellHasPellet + countPelletsRecursive(r, c + 1);
}

//   renderMaze — draws the maze to the console every frame.
// 
//   Uses gotoxy(0,0) to jump back to the top-left corner and
//   overwrite the previous frame — no screen flicker this way.

void renderMaze(void)
{
    int  row, col;
    char scoreText[SCORE_BUF_LEN];  // buffer for the score line 

    // Build the score string in memory first, then print it once 
    sprintf(scoreText, "Player: %s   Points: %d", pacman.name, pacman.points);

    // Jump cursor to top-left — overwrites last frame without clearing 
    gotoxy(0, 0);

    for (row = 0; row < ROWS; row++)
    {
        for (col = 0; col < COLS; col++)
        {
            char cell = maze[row][col];

            if      (cell == '#') printf("#");
            else if (cell == '.') printf(".");
            else if (cell == 'P') printf("C");   // Pac-Man displayed as C 
            else if (cell == 'G') printf("G");   // Ghost 
            else                  printf(" ");   // empty space 
        }
        printf("\n");
    }

    printf("%s   \n", scoreText);
    printf("High Score: %d   \n", loadHighScore());
    printf("Controls: Arrow Keys to move, Q to quit\n");
}

//   movePacman — moves Pac-Man one step in the given direction.
// 
//   Parameters:
//     p     — pointer to the Player struct (demonstrates ->)
//     stepX — columns to move: -1 left, +1 right, 0 none
//     stepY — rows to move:    -1 up,   +1 down,  0 none

void movePacman(struct Player *p, int stepX, int stepY)
{
    // Calculate the cell we want to move into 
    int nextX = p->x + stepX;
    int nextY = p->y + stepY;

    // Wall check — '#' blocks movement 
    if (maze[nextY][nextX] == '#')
        return;

    // Eating a pellet earns one point 
    if (maze[nextY][nextX] == '.')
        p->points++;    // -> modifies the struct through the pointer 

    // Erase old position, update stored position, place at new position 
    maze[p->y][p->x] = ' ';
    p->x = nextX;
    p->y = nextY;
    maze[p->y][p->x] = 'P';
}

 
//   moveEnemy — moves the ghost one step in a random direction.
//   Uses rand() % GHOST_DIRECTIONS to pick 0/1/2/3 randomly.
//   If the chosen direction is blocked by a wall '#', the ghost
//   simply skips its turn (it does NOT walk through walls).
 
void moveEnemy(void)
{
    int direction;
    int nextX = ghost.x;
    int nextY = ghost.y;

    direction = rand() % GHOST_DIRECTIONS;  // 0, 1, 2, or 3 

    switch (direction)
    {
        case 0: nextY--; break;   // up    
        case 1: nextY++; break;   // down  
        case 2: nextX--; break;   // left  
        case 3: nextX++; break;   // right 
    }

    // Skip the move if the target cell is a wall 
    if (maze[nextY][nextX] == '#')
        return;

    // Erase old position, update stored position, draw at new position 
    maze[ghost.y][ghost.x] = ' ';
    ghost.x = nextX;
    ghost.y = nextY;
    maze[ghost.y][ghost.x] = 'G';
}


//   resetGame — restores the maze to its original state and
//   puts Pac-Man and the ghost back at their starting positions.
//   Called at the start of every new game so the player can
//   replay without restarting the program.
// 
void resetGame(void)
{
    int r;

    // Copy each original row back into the active maze 
    for (r = 0; r < ROWS; r++)
        strcpy(maze[r], originalMaze[r]);

    // Reset player position using pointer — demonstrates 
    struct Player *p = &pacman;
    p->x      = PACMAN_START_X;    // named constant, not a raw number 
    p->y      = PACMAN_START_Y;
    p->points = 0;

    // Reset ghost position using named constants 
    ghost.x = GHOST_START_X;
    ghost.y = GHOST_START_Y;
}


//   runGame — the main game loop.
//  Runs until the player wins, loses, or presses Q.
//   Returns:
//     0 — player quit  (Q pressed)
//     1 — game over    (ghost caught Pac-Man)
//    2 — win          (all pellets eaten)

int runGame(void)
{
    int keyPressed;

    while (1)
    {
        renderMaze();

        // Check: ghost caught Pac-Man? 
        if (pacman.x == ghost.x && pacman.y == ghost.y)
        {
            printf("\n*** GAME OVER! Final Points: %d ***\n", pacman.points);
            saveHighScore(&pacman, pacman.points);  // pointer passed here 
            return 1;
        }

//        Check: all pellets eaten? (uses recursion) 
//          countPelletsRecursive(0, 0) starts at row 0, col 0
//          and recursively scans the entire maze.
         
        if (countPelletsRecursive(0, 0) == 0)
        {
            printf("\n*** YOU WIN! Final Points: %d ***\n", pacman.points);
            saveHighScore(&pacman, pacman.points);
            return 2;
        }

        //  Read keyboard input (non-blocking with kbhit) 
        if (kbhit())    // kbhit() returns 1 only if a key is waiting 
        {
            keyPressed = getch();

            
//              Arrow keys produce TWO bytes:
//              Byte 1 — always 0 or 224  (a marker, not a real character)
//              Byte 2 — direction code:  72=Up  80=Down  75=Left  77=Right
//              We call getch() again to read the second byte.
             
            if (keyPressed == 0 || keyPressed == 224)
            {
                keyPressed = getch();   // read the actual direction byte 

                switch (keyPressed)
                {
                    case 72: movePacman(&pacman,  0, -1); break;  // Up    
                    case 80: movePacman(&pacman,  0,  1); break;  // Down  
                    case 75: movePacman(&pacman, -1,  0); break;  // Left  
                    case 77: movePacman(&pacman,  1,  0); break;  // Right 
                }
            }

            // Q / q quits immediately 
            if (keyPressed == 'q' || keyPressed == 'Q')
                return 0;
        }

        moveEnemy();

        // Pause between frames — GAME_SPEED_MS controls game speed 
        Sleep(GAME_SPEED_MS);
    }
}

// main — program entry point
 
int main(void)
{
    char again;
    int  r;

    // Seed the random number generator so ghost paths differ each run 
    srand(time(NULL));
    hideCursor();

    // Save the original maze so resetGame() can restore it later 
    for (r = 0; r < ROWS; r++)
        strcpy(originalMaze[r], maze[r]);

    system("cls");
    printf("Enter your name: ");

    
//      %29s limits input to NAME_LEN-1 characters (29).
//      The last byte is always reserved for '\0' (the string terminator).
//      Plain %s without a limit risks a buffer overflow security bug.
     
    scanf("%29s", pacman.name);

    printf("\nCurrent High Score: %d\n", loadHighScore());
    printf("Press any key to start...\n");
    getch();

    // do-while: plays at least once; repeats if the player says y/Y
    do
    {
        system("cls");
        resetGame();
        runGame();

        printf("Play again? (y/n): ");
        again = getch();
        printf("\n");

    } while (again == 'y' || again == 'Y');

    printf("\nThanks for playing, %s! Goodbye.\n", pacman.name);
    Sleep(EXIT_DELAY_MS);   // named constant

    return 0;
}
