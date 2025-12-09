#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

// Load Saved Player level from profile.txt
int loadPlayerLevel() {
    FILE *f = fopen("profile.txt", "r");
    if (!f) return 1; // default level 1 if no file yet

    int level = 1;
    if (fscanf(f, "%d", &level) != 1) level = 1;
    fclose(f);
    if (level < 1) level = 1;
    if (level > 50) level = 50;
    return level;
}


// --- Mood logic ---
typedef enum { SAD, NEUTRAL, HAPPY } Mood;
const char *moodFaces[] = { ":(", ":|", ":)" };

typedef struct {
    int x, y;
    Mood mood;
} Player;

// --- Obstacle logic ---
#define NUM_QUOTES 15
#define NUM_EXERCISES 8
#define MAX_LINE_LEN 256

typedef enum { NONE, TRAP, PUZZLE, BONUS, POWERUP } ObstacleType;

typedef enum { MENTOR, SHADOW, SAGE } NPCType;

typedef struct {
    int x, y;          // position in maze
    NPCType type;      // archetype
    const char *msgHappy;
    const char *msgNeutral;
    const char *msgSad;
    bool active;       // has this NPC already spoken?
} NPC;

const char *philosophyQuotes[NUM_QUOTES] = {
    "If you look for perfection, you’ll never be content. — Leo Tolstoy",
    "We are what we pretend to be, so we must be careful what we pretend to be. — Kurt Vonnegut",
    "The privilege of a lifetime is to become who you truly are. — C.G. Jung",
    "Become what you are. — Friedrich Nietzsche",
    "The notion that a human being should be constantly happy is a uniquely modern, uniquely destructive idea. — Andrew Weil",
    "You are not too old and it is not too late to dive into your increasing depths where life calmly gives out its own secret. — Rainer Maria Rilke",
    "The menu is not the meal. — Alan Watts",
    "We see the world not as it is, but as we are. — Anais Nin",
    "I dream. Sometimes I think that’s the only right thing to do. — Haruki Murakami",
    "It is not true that people stop pursuing dreams because they grow old, they grow old because they stop pursuing dreams. — Gabriel García Márquez",
    "The struggle between ‘for’ and ‘against’ is the mind’s worst disease. — Sent-ts’an",
    "Life must be understood backward. But it must be lived forward. — Soren Kierkegaard",
    "What labels me, negates me. — Soren Kierkegaard",
    "You have your way. I have my way. As for the right way, the correct way, and the only way, it does not exist. — Friedrich Nietzsche",
    "To live is to suffer; to survive is to find some meaning in the suffering. — Friedrich Nietzsche"
};

const char *philosophyExercises[NUM_EXERCISES] = {
    "Exercise: Name a moment this week that changed your mood. What triggered it?",
    "Exercise: Imagine you wake up in a world where everyone’s mood shapes reality. How would today look different?",
    "Exercise: Think of a friend in a maze of their own—what philosophical advice would you give them right now?",
    "Exercise: Recall a decision you made recently. Was it ruled by your mood or rational thought?",
    "Quick Challenge: Try to imagine a color you’ve never seen. What does it mean for experience to go beyond knowledge? (Mary’s Room)",
    "Thought Experiment: The Veil of Ignorance—Would you choose your current maze path if you didn’t know you were the player?",
    "Exercise: List three things that make your journey meaningful, even when the maze gets tough.",
    "Mini Reflection: After this game, consider what you’d do differently if each mood unlocked a secret bonus or challenge."
};

const char* obstacleNames[] = {" ", "Trap", "Puzzle", "Bonus"};

// --- Direction logic for maze generation ---
const int DX[4] = {-1, 1, 0, 0};
const int DY[4] = {0, 0, -1, 1};

void shuffleDirections(int dirs[4]) {
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = dirs[i];
        dirs[i] = dirs[j];
        dirs[j] = temp;
    }
}

int **allocate2DIntArray(int rows, int cols) {
    int **arr = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        arr[i] = (int *)malloc(cols * sizeof(int));
    }
    return arr;
}

bool **allocate2DBoolArray(int rows, int cols) {
    bool **arr = (bool **)malloc(rows * sizeof(bool *));
    for (int i = 0; i < rows; i++) {
        arr[i] = (bool *)calloc(cols, sizeof(bool));
    }
    return arr;
}

void free2DIntArray(int **arr, int rows) {
    for (int i = 0; i < rows; i++) free(arr[i]);
    free(arr);
}
void free2DBoolArray(bool **arr, int rows) {
    for (int i = 0; i < rows; i++) free(arr[i]);
    free(arr);
}

void carveMaze(int x, int y, int **maze, bool **visited, int rows, int cols) {
    visited[x][y] = true;
    int dirs[4] = {0, 1, 2, 3};
    shuffleDirections(dirs);
    for (int i = 0; i < 4; i++) {
        int nx = x + DX[dirs[i]] * 2;
        int ny = y + DY[dirs[i]] * 2;
        if (nx >= 0 && nx < rows && ny >= 0 && ny < cols && !visited[nx][ny]) {
            maze[x + DX[dirs[i]]][y + DY[dirs[i]]] = 1;
            maze[nx][ny] = 1;
            carveMaze(nx, ny, maze, visited, rows, cols);
        }
    }
}

//Generate maze using recursive backtracking (DFS)
void generateMaze(int **maze, int rows, int cols, int exitX, int exitY) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            maze[i][j] = 0;
    bool **visited = allocate2DBoolArray(rows, cols);
    maze[0][0] = 1;
    carveMaze(0, 0, maze, visited, rows, cols);
    maze[exitX][exitY] = 1;
    free2DBoolArray(visited, rows);
}

// Log "life lesson" line onto journal.txt
void logLifeLesson(const char *message) {
    FILE *f = fopen("journal.txt", "a");
    if (f == NULL) {
        printf("Could not open journal file.\n");
        return;
    }
    fprintf(f, "%s\n", message);
    fclose(f);
}

//Place traps, puzzles, bonuses and power-ups, difficulty level can be chosen with chosenLevel
void placeObstacles(int **maze, int **obstacles, int rows, int cols, int exitX, int exitY, int chosenLevel) {
    int baseDiv = 18;
    int levelFactor = chosenLevel / 5;
    int trapDiv = baseDiv - levelFactor;
    if(trapDiv < 6) trapDiv = 6;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (maze[i][j] == 1 && !(i == 0 && j == 0) && !(i == exitX && j == exitY)) {
                int r = rand() % trapDiv; // You can lower for more obstacles
                if (r == 0) obstacles[i][j] = TRAP;
                else if (r == 1) obstacles[i][j] = PUZZLE;
                else if (r == 2) obstacles[i][j] = BONUS;
                else obstacles[i][j] = NONE;
            } else {
                obstacles[i][j] = NONE;
            }
        }
    }
    int powerupCount = rows * cols / 60; // small number
for (int k = 0; k < powerupCount; k++) {
    int x = rand() % rows;
    int y = rand() % cols;
    if (maze[x][y] == 1 && obstacles[x][y] == NONE &&
        !(x == 0 && y == 0) && !(x == exitX && y == exitY)) {
        obstacles[x][y] = POWERUP;
    }
}
}

void printMazeGeneric(int **maze, int **obstacles, int rows, int cols, int playerX, int playerY, int exitX, int exitY, bool **visited, NPC npcs[], int npcCount) {
    for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {

        bool hasNPC = false;
        for (int k = 0; k < npcCount; k++) {
            if (npcs[k].active && npcs[k].x == i && npcs[k].y == j) {
                hasNPC = true;
                break;
            }
        }

        if (i == playerX && j == playerY) printf("P ");
        else if (i == exitX && j == exitY) printf("E ");
        else if (hasNPC) printf("N ");         // show NPC
        else if (obstacles[i][j] == TRAP) printf("T ");
        else if (obstacles[i][j] == PUZZLE) printf("Q ");
        else if (obstacles[i][j] == BONUS) printf("B ");
        else if (obstacles[i][j] == POWERUP) printf("K "); // K = key/power

        else if (maze[i][j] == 1) {
            if (visited[i][j]) printf("* ");
            else printf(". ");
        } else {
            printf("# ");
        }
    }
    printf("\n");
}
}

void printPlayerStatus(Player player) {
    printf("Player Location: (%d, %d)\n", player.x, player.y);
    printf("Current Mood: %s\n", moodFaces[player.mood]);
}
Mood generateMood() {
    return (Mood)(rand() % 3);
}


void processObstacle(Player *player, int obs,
                     int *trapCount, int *puzzleCount, int *bonusCount, int rows, int cols) {
    if (obs == TRAP) {
        (*trapCount)++;  // count traps

        if (player->mood != HAPPY) {
            printf("You hit a trap! Only Happy players can pass. You lose a turn. Try to cheer up!\n");
            logLifeLesson("You hit a trap while not happy: sometimes you need inner strength before facing certain challenges.");
            player->mood = SAD;
        } else {
            printf("You happily avoided the trap!\n");
            logLifeLesson("You avoided a trap while happy: good moods can help you navigate problems more lightly.");
        }

    } else if (obs == PUZZLE) {
        (*puzzleCount)++;  // count puzzles

        printf("You found a puzzle! It alters your mood.\n");
        logLifeLesson("You faced a puzzle: complex situations can shift how you feel and think.");
        player->mood = (player->mood + 1) % 3;

    } else if (obs == POWERUP) {
    printf("You found a power-up tile!\n");

    if (player->mood == HAPPY) {
        printf("Your joy unlocks a shortcut somewhere in the maze.\n");
        logLifeLesson("Happiness unlocked new paths: positive states can reveal hidden options.");
    } else if (player->mood == SAD) {
        printf("In sadness, the maze feels heavier.\n");
        logLifeLesson("Sadness closed some paths: sometimes our mood narrows our vision.");
    } else {
        printf("Neutral mind, neutral maze: nothing changes, yet.\n");
        logLifeLesson("Neutrality kept the maze steady: not every moment needs change.");
    }
} else if (obs == BONUS) {
        (*bonusCount)++;  // count bonuses

        printf("You found a bonus! Your mood is now Happy!\n");
        logLifeLesson("You found a bonus: good surprises can flip a bad day into a brighter one.");
        player->mood = HAPPY;
    }
    
}

// Randomly show either a philosophical quote or a small exercise
void showRandomPhilosophySupport() {
    int r = rand() % 2; // 0 quote, 1 exercise
    if (r == 0) {
        int idx = rand() % NUM_QUOTES;
        printf("\n--- Philosophical Quote ---\n");
        printf("%s\n", philosophyQuotes[idx]);
        printf("----------------------------\n");
    } else {
        int idx = rand() % NUM_EXERCISES;
        printf("\n--- Philosophical Exercise ---\n");
        printf("%s\n", philosophyExercises[idx]);
        printf("\nDo you want to answer this? (y/n): ");
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {} // clear leftover
        char choice = getchar();
        if (choice == 'y' || choice == 'Y') {
            char answer[256];
            while ((c = getchar()) != '\n' && c != EOF) {} // clear line
            printf("Your reflection: ");
            if (fgets(answer, sizeof(answer), stdin)) {
                // simple response
                printf("\nThanks for sharing. Even small reflections can change how you move in the maze and in life.\n");
                logLifeLesson("Player completed a philosophical exercise and reflected on their journey.");
            }
        } else {
            printf("That's okay. Not every question needs an answer right now.\n");
        }
        printf("----------------------------\n");
    }
}

//Maze wall is changed based on player's mood
void morphMaze(int **maze, int **obstacles, int rows, int cols,
               int morphAmount, bool **preservePath, Player player) {
    for (int n = 0; n < morphAmount; n++) {
        int i = rand() % rows;
        int j = rand() % cols;

        if (preservePath[i][j]) continue;               // not to touch protected cells
        if (player.x == i && player.y == j) continue;   // not to trap player

        // Mood-based rule:
        // SAD  -> more openings
        // HAPPY-> more walls (slightly harder)
        // NEUTRAL -> random toggle
        if (maze[i][j] == 0 && player.mood == SAD) {
            maze[i][j] = 1;      // open a wall
        } else if (maze[i][j] == 1 && player.mood == HAPPY) {
            maze[i][j] = 0;      // to close a path
        } else {
            maze[i][j] = (maze[i][j] == 1) ? 0 : 1; // neutral: toggle
        }
    }

    // Move one random obstacle
    int oi = rand() % rows;
    int oj = rand() % cols;
    if (!preservePath[oi][oj] && maze[oi][oj] == 1 &&
        !(player.x == oi && player.y == oj)) {
        obstacles[oi][oj] = rand() % 3 + 1; // TRAP, PUZZLE, BONUS
    }
}

void initNPCs(NPC npcs[], int npcCount, int rows, int cols, int **maze) {
    // positions picked randomly on paths
    for (int i = 0; i < npcCount; i++) {
        npcs[i].type = (i == 0) ? MENTOR : (i == 1) ? SHADOW : SAGE;
        npcs[i].active = true;

        // to place on a random path tile
        while (1) {
            int rx = rand() % rows;
            int ry = rand() % cols;
            if (maze[rx][ry] == 1 && !(rx == 0 && ry == 0)) {
                npcs[i].x = rx;
                npcs[i].y = ry;
                break;
            }
        }

        // messages by type
        if (npcs[i].type == MENTOR) {
            npcs[i].msgHappy   = "Mentor: You’re glowing today. Use this energy wisely.";
            npcs[i].msgNeutral = "Mentor: Even small steps count. Keep going.";
            npcs[i].msgSad     = "Mentor: It’s okay to move slowly. Just don’t stop.";
        } else if (npcs[i].type == SHADOW) {
            npcs[i].msgHappy   = "Shadow: Are you sure this happiness isn’t just a mask?";
            npcs[i].msgNeutral = "Shadow: Silence is loud, isn’t it?";
            npcs[i].msgSad     = "Shadow: I know the dark corners. But they’re not all you are.";
        } else { // SAGE
            npcs[i].msgHappy   = "Sage: Joy is also data. Notice what makes it arise.";
            npcs[i].msgNeutral = "Sage: Observe your mind like a sky, not the clouds.";
            npcs[i].msgSad     = "Sage: Pain is a teacher. What is it asking you to see?";
        }
    }
}

// NPC talks based on mood and stats, later, asks for reflection line
void speakWithNPC(NPC *npc, Player *player,
                  int steps,
                  int trapCount, int puzzleCount, int philosophyUses);

//Check if player is on an NPC tile and trigger the encounter once
void checkNPCEncounter(Player *player, NPC npcs[], int npcCount, int steps, int trapCount, int puzzleCount, int philosophyUses) {
    for (int i = 0; i < npcCount; i++) {
        if (!npcs[i].active) continue;
        if (npcs[i].x == player->x && npcs[i].y == player->y) {

            speakWithNPC(&npcs[i], player, steps, trapCount, puzzleCount, philosophyUses);
            logLifeLesson("You met an archetype in the maze: guidance appears in many forms when you keep moving.");
            npcs[i].active = false;
        }
    }
}


void showJournal() {
    FILE *f = fopen("journal.txt", "r");
    if (f == NULL) {
        printf("\nNo journal entries yet. Go live a little in the maze first.\n");
        return;
    }

    char buffer[MAX_LINE_LEN];
    printf("\n===== Life Lessons Journal =====\n");
    while (fgets(buffer, MAX_LINE_LEN, f)) {
        // remove newline
        buffer[strcspn(buffer, "\n")] = 0;
        printf("- %s\n", buffer);
    }
    printf("================================\n\n");
    fclose(f);
}

// Generate maze based on chosen level (size grows with level)
// Place mood-based NPC archetypes
// Handle traps/puzzles/bonuses and log "life lessons"

int levelToSize(int level) {
    if (level < 1) level = 1;
    if (level > 50) level = 50;        // levels 1–50

    int size = 9 + level * 2;         
    return size;
}





void saveRunSnapshot(const char *filename,
                     int **maze, int **obstacles,
                     int rows, int cols,
                     Player player, int exitX, int exitY) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Could not open snapshot file.\n");
        return;
    }

    // Header info
    fprintf(f, "PsyMaze Snapshot\n");
    fprintf(f, "Size: %d x %d\n", rows, cols);
    fprintf(f, "Player: (%d,%d) Mood: %s\n", player.x, player.y, moodFaces[player.mood]);
    fprintf(f, "Exit: (%d,%d)\n", exitX, exitY);
    fprintf(f, "Maze:\n");

    // Maze + obstacles
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i == player.x && j == player.y)      fputc('P', f);
            else if (i == exitX && j == exitY)       fputc('E', f);
            else if (obstacles[i][j] == TRAP)        fputc('T', f);
            else if (obstacles[i][j] == PUZZLE)      fputc('Q', f);
            else if (obstacles[i][j] == BONUS)       fputc('B', f);
            else if (maze[i][j] == 1)                fputc('.', f);
            else                                     fputc('#', f);
        }
        fputc('\n', f);
    }

    fclose(f);
    printf("Run snapshot saved to %s\n", filename);
}

void speakWithNPC(NPC *npc, Player *player,
                  int steps,
                  int trapCount, int puzzleCount, int philosophyUses) {
    const char *baseMsg = NULL;
    if (player->mood == HAPPY)      baseMsg = npc->msgHappy;
    else if (player->mood == NEUTRAL) baseMsg = npc->msgNeutral;
    else                             baseMsg = npc->msgSad;

    printf("\n--- NPC Encounter ---\n");

    // Base line depending on archetype & mood
    printf("%s\n", baseMsg);

    // comments based on stats
    if (steps > 150) {
        printf("NPC: You've been wandering a long time... what keeps you going?\n");
    }
    if (trapCount > 0) {
        printf("NPC: I see you've survived some traps. What did they teach you?\n");
    }
    if (philosophyUses >= 3) {
        printf("NPC: You seek wisdom often. That curiosity is your true power.\n");
    }

    printf("NPC: Before you go, answer this in one line:\n");
    printf("     What is one thing you learned in this maze so far?\n");

    char answer[256];
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {} // clear leftover
    printf("Your reflection: ");
    if (fgets(answer, sizeof(answer), stdin)) {
        answer[strcspn(answer, "\n")] = 0;
        printf("NPC: Thank you. Even small reflections change how you move.\n");
        logLifeLesson("NPC reflection from player:");
        logLifeLesson(answer);
    }
    printf("----------------------\n");
}
void askEndOfSessionReflection() {
    printf("\nDo you want to write a short reflection about this run? (y/n): ");

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {} // clear leftover input
    char choice = getchar();

    if (choice == 'y' || choice == 'Y') {
        char line[256];
        while ((c = getchar()) != '\n' && c != EOF) {} // clear line
        printf("Write your reflection (one line): ");
        if (fgets(line, sizeof(line), stdin)) {
            line[strcspn(line, "\n")] = 0; // remove newline

            FILE *f = fopen("journal.txt", "a");
            if (f) {
                fprintf(f, "[End-of-session reflection] %s\n", line);
                fclose(f);
                printf("Reflection saved to journal.\n");
            } else {
                printf("Could not open journal file to save reflection.\n");
            }
        }
    } else {
        printf("No reflection this time. That’s okay.\n");
    }
}


void waitForEnter() {
    printf("\nPress Enter to continue...\n");
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {} // clear line
}


//Summarize session stats to a text file (steps, moods, obstacles, philosphy)
void saveSessionSummary(const char *filename,
                        int steps,
                        int moodCounts[3],
                        int trapCount, int puzzleCount, int bonusCount,
                        int philosophyUses) {
    FILE *f = fopen(filename, "a");
    if (!f) {
        printf("Could not open summary file.\n");
        return;
    }

    fprintf(f, "=== New Session ===\n");
    fprintf(f, "Steps: %d\n", steps);
    fprintf(f, "Moods: Sad=%d Neutral=%d Happy=%d\n",
            moodCounts[SAD], moodCounts[NEUTRAL], moodCounts[HAPPY]);
    fprintf(f, "Obstacles: Traps=%d Puzzles=%d Bonuses=%d\n",
            trapCount, puzzleCount, bonusCount);
    fprintf(f, "Philosophy uses: %d\n\n", philosophyUses);

    fclose(f);
}

int showAchievementsAndComputeXP(int steps,
                                 int moodCounts[3],
                                 int trapCount, int puzzleCount, int bonusCount,
                                 int philosophyUses) {
    int xp = 0;

    printf("\n===== ACHIEVEMENTS =====\n");

    if (steps < 50) {
        printf("Efficient Explorer (finished in under 50 steps)\n");
        xp += 30;
    } else if (steps > 200) {
        printf("Persistent Wanderer (kept going despite a long path)\n");
        xp += 20;
    }

    if (moodCounts[HAPPY] > moodCounts[SAD]) {
        printf("Bringer of Light (more happy moods than sad)\n");
        xp += 20;
    }

    if (philosophyUses >= 3) {
        printf("Reflective Seeker (used philosophy support 3+ times)\n");
        xp += 25;
    }

    if (puzzleCount >= 2) {
        printf("Riddle Breaker (solved multiple puzzles)\n");
        xp += 15;
    }

    if (trapCount == 0) {
        printf("Untouched by Traps (avoided all traps)\n");
        xp += 30;
    }

    if (trapCount > 0 && moodCounts[SAD] > 0) {
        printf("Resilient Soul (kept going despite traps and sadness)\n");
        xp += 15;
    }

    printf("Total XP this run: %d\n", xp);
    printf("========================\n");

    return xp;
}

void savePlayerLevel(int level) {
    FILE *f = fopen("profile.txt", "w");
    if (!f) {
        printf("Could not save player level.\n");
        return;
    }
    fprintf(f, "%d\n", level);
    fclose(f);
}

void showSpeedrunMedal(int steps) {
    printf("\n===== SPEEDRUN RESULT =====\n");
    if (steps <= 40) {
        printf("Medal: GOLD – You found a very efficient path through the maze!\n");
    } else if (steps <= 80) {
        printf("Medal: SILVER – Balanced between exploration and efficiency.\n");
    } else if (steps <= 140) {
        printf("Medal: BRONZE – You made it, with plenty of detours.\n");
    } else {
        printf("Medal: EXPLORER – You took your time and saw a lot of the maze.\n");
    }
    printf("===========================\n");
}




int main() {
   int baseLevel = loadPlayerLevel();
printf("Saved player level (from previous runs): %d\n", baseLevel);

int chosenLevel;
printf("Choose level to play (1–50): ");
scanf("%d", &chosenLevel);
if (chosenLevel < 1) chosenLevel = 1;
if (chosenLevel > 50) chosenLevel = 50;


int size = levelToSize(chosenLevel);
int rows = size, cols = size;
int exitX = rows - 1, exitY = cols - 1;

printf("Starting level %d -> maze size %d x %d\n", chosenLevel, rows, cols);


    srand(time(NULL));
    int moodCounts[3] = {0, 0, 0};   // SAD, NEUTRAL, HAPPY
    int trapCount = 0;
    int puzzleCount = 0;
    int bonusCount = 0;
    int philosophyUses = 0;
    int steps = 0;


    int **maze = allocate2DIntArray(rows, cols);
    int **obstacles = allocate2DIntArray(rows, cols);
    bool **visited = allocate2DBoolArray(rows, cols);
    generateMaze(maze, rows, cols, exitX, exitY);
    placeObstacles(maze, obstacles, rows, cols, exitX, exitY, chosenLevel);
    int npcCount = 3;
    NPC npcs[3];
    initNPCs(npcs, npcCount, rows, cols, maze);

    Player player = {0, 0, NEUTRAL};
    visited[player.x][player.y] = true;
    Mood oldMood = player.mood;
    bool **preservePath = (bool **)malloc(rows * sizeof(bool *));
for (int i = 0; i < rows; i++) {
    preservePath[i] = (bool *)calloc(cols, sizeof(bool));
}




    // To track previous direction
char move;
char lastMove = 'd'; // default (right), update each move

while (player.x != exitX || player.y != exitY) {
    showRandomPhilosophySupport();
    waitForEnter();
    printMazeGeneric(maze, obstacles, rows, cols, player.x, player.y, exitX, exitY, visited, npcs, npcCount);
    printPlayerStatus(player);
    printf("\nMove (w/a/s/d), 'j' to jump, 'l' for journal, or 's' to save: ");
    scanf(" %c", &move);

    int newX = player.x;
    int newY = player.y;
    int jumpX = player.x;
    int jumpY = player.y;

    // Standard moves
    if (move == 'w')      { newX--; lastMove = 'w'; }
    else if (move == 's') { newX++; lastMove = 's'; }
    else if (move == 'a') { newY--; lastMove = 'a'; }
    else if (move == 'd') { newY++; lastMove = 'd'; }
    // ----- JUMP FEATURE -----
    else if (move == 'j') {
        // set jumpX/jumpY two tiles ahead in lastMove direction
        if (lastMove == 'w')      { jumpX -= 2; }
        else if (lastMove == 's') { jumpX += 2; }
        else if (lastMove == 'a') { jumpY -= 2; }
        else if (lastMove == 'd') { jumpY += 2; }
        // to check jumping rules
        int midX = (player.x + jumpX) / 2;
        int midY = (player.y + jumpY) / 2;
        // will only allow jump over #, not over traps/puzzles/bonus
        if (jumpX >= 0 && jumpX < rows && jumpY >= 0 && jumpY < cols &&
            maze[midX][midY] == 0 && maze[jumpX][jumpY] == 1 &&
            (obstacles[midX][midY] == NONE)) 
        {
            player.x = jumpX;
            player.y = jumpY;
            visited[player.x][player.y] = true;
            steps++;
            printf("You jumped over a wall!\n");
            // might add a mood boost here, later, when? I don't know too
            processObstacle(&player, obstacles[player.x][player.y], &trapCount, &puzzleCount, &bonusCount, rows, cols);
    
                oldMood = player.mood;
                player.mood = generateMood();
                if (oldMood != player.mood) {
                    printf("\nThe maze feels different... mood shift is morphing the labyrinth!\n");
                    morphMaze(maze, obstacles, rows, cols, 3, preservePath, player);
                }
                continue; // next loop, already applied move
        }
        // If blocked by letter obstacle or invalid, trigger mini-game or block
        else if (jumpX >= 0 && jumpX < rows && jumpY >= 0 && jumpY < cols &&
                 (obstacles[midX][midY] != NONE)) 
        {
            printf("Oops! You tried to jump a special tile. Solve this word puzzle to proceed!\n");
            // (Call mini word game))
            // (If failed, don't move, else allow)
            continue;
        } else {
            printf("Invalid jump!\n");
            continue;
        }
    }
    else if(move == 'l') {
        showJournal();
        continue;
    }
    else if(move =='s'){
        saveRunSnapshot("run_snapshot.txt", maze, obstacles, rows, cols, player, exitX, exitY);
        continue;
    }
    else if (move == 'h') { showRandomPhilosophySupport();
        philosophyUses++;
        continue; }
    else { printf("Invalid input!\n"); continue; }

    // Standard move processing continues here...
    if (newX >= 0 && newX < rows && newY >= 0 && newY < cols && maze[newX][newY] == 1) {
        player.x = newX;
        player.y = newY;
        visited[player.x][player.y] = true;
        steps++;
        processObstacle(&player, obstacles[player.x][player.y], &trapCount, &puzzleCount, &bonusCount, rows, cols);

        checkNPCEncounter(&player, npcs, npcCount, steps, trapCount, puzzleCount, philosophyUses);
        oldMood = player.mood;
        player.mood = generateMood();
        moodCounts[player.mood]++;
        if (oldMood != player.mood) {
            printf("\nThe maze feels different... mood shift is morphing the labyrinth!\n");
            morphMaze(maze, obstacles, rows, cols, 3, preservePath, player);
        }
    } else {
        printf("Invalid move!\n");
    }
}

    printMazeGeneric(maze, obstacles, rows, cols, player.x, player.y, exitX, exitY, visited, npcs, npcCount);
    printPlayerStatus(player);
    printf("\nCongratulations! You reached the exit.\n");

    printf("\n===== SESSION ANALYTICS =====\n");
    printf("Total steps taken: %d\n", steps);
    printf("Mood counts:\n");
    printf("  Sad:     %d\n", moodCounts[SAD]);
    printf("  Neutral: %d\n", moodCounts[NEUTRAL]);
    printf("  Happy:   %d\n", moodCounts[HAPPY]);
    printf("Obstacles encountered:\n");
    printf("  Traps:   %d\n", trapCount);
    printf("  Puzzles: %d\n", puzzleCount);
    printf("  Bonuses: %d\n", bonusCount);
    printf("Philosophy uses (quotes/exercises): %d\n", philosophyUses);

// simple ASCII bar for mood
int totalMoods = moodCounts[SAD] + moodCounts[NEUTRAL] + moodCounts[HAPPY];
if (totalMoods > 0) {
    printf("\nMood distribution (ASCII):\n");
    printf("Sad:     ");
    for (int i = 0; i < moodCounts[SAD]; i++) printf("*");
    printf("\nNeutral: ");
    for (int i = 0; i < moodCounts[NEUTRAL]; i++) printf("*");
    printf("\nHappy:   ");
    for (int i = 0; i < moodCounts[HAPPY]; i++) printf("*");
    printf("\n");
}
printf("===== END OF SESSION =====\n");

showSpeedrunMedal(steps);

int earnedXP = showAchievementsAndComputeXP(steps, moodCounts,
                                            trapCount, puzzleCount, bonusCount,
                                            philosophyUses);
printf("You earned %d XP this session!\n", earnedXP);

int newLevel = baseLevel;
if (earnedXP >= 60) newLevel += 2;
else if (earnedXP >= 30) newLevel += 1;
// cap
if (newLevel > 50) newLevel = 50;

printf("Player level went from %d to %d.\n", baseLevel, newLevel);
savePlayerLevel(newLevel);



saveSessionSummary("session_stats.txt",
                   steps, moodCounts,
                   trapCount, puzzleCount, bonusCount,
                   philosophyUses);

askEndOfSessionReflection();



    // Free all dynamic memory
    free2DIntArray(maze, rows);
    free2DIntArray(obstacles, rows);
    free2DBoolArray(visited, rows);
    free2DBoolArray(preservePath, rows);
    return 0;
}


