/* Types and rules extracted from the original CampfireHunt.c.
 * No Raylib, Windows or UEFI dependencies. See README_UEFI.md for provenance.
 */
#ifndef CAMPFIRE_GAME_CORE_H
#define CAMPFIRE_GAME_CORE_H
typedef unsigned char GameBool;
typedef struct { float x, y; } Vector2;
#define SCREENWIDTH 1200
#define SCREENHEIGHT 800
#define MARGIN (SCREENWIDTH / 5)
#define TIMELIMIT 360.0f
#define FRAMESIZE 192

//Player
#define PLAYER_MAX_LIFE 3
#define PLAYER_HEALTH 5
#define MAX_CHARACTER_TYPE 4

typedef struct GameScreen{
    GameBool gameOver;
    GameBool pause;
    GameBool showMenu;
    GameBool changesinglecharacter;
    GameBool changemulticharacter;
    GameBool singleplayer;
    GameBool multiplayer;
    GameBool manual;
} Screen;

typedef struct GameStage{
    GameBool stage1;
    GameBool stage2;
    GameBool stage3;
} Stage;

typedef struct Player{
    Vector2 position;
    int life;
    int health;
    float lastAttack;
    int money_collected;
    int goblin_killed;
    GameBool isAttack;
    int stage;
} player;

typedef enum skin{
    warrior_red = 0,
    warrior_blue,
    warrior_yellow,
    warrior_purple
} CharacterType;

//Goblin
#define GOBLIN_HEALTH 4
#define MONSTER_INTERVAL 900 //frames; 15s

typedef struct Monster{
    Vector2 position;
    int health;
    int color;
    Vector2 velocity;
    GameBool attack;
    float lastAttack;
    GameBool dying;
    int deathFrame;
} monster;

typedef enum {
    goblin_red = 0,
    goblin_blue,
    goblin_yellow,
    goblin_purple
} GoblinType;

#define MONMON_HEALTH 3
#define MONMON_INTERVAL 1500 //frames; 25s
#define MONMON_AMOUNT 3

typedef struct Mon{
    Vector2 position;
    int health;
    int color;
    Vector2 velocity;
    GameBool dying;
    int deathFrame;
} monmon;

typedef enum {
    monmon_red = 0,
    monmon_blue,
    monmon_yellow,
    monmon_purple
} MonType;

#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define MAP_WIDTH 85
#define MAP_HEIGHT 90

typedef struct {
    int x;
    int y;
    int tileIndex;
} sTile;

#define MONEY_AMOUNT 10
#define GENERATE_INTERVAL 160 //2.6s
#define MONEY_EXPIRY 420 //7s
typedef struct Money{
    int x;
    int y;
    int expiry;
} moneyAtt;

#define GOBLIN_CAPACITY 8
#define MEAT_INTERVAL 2000
typedef struct {
    player players[3]; /* single player, multiplayer left, multiplayer right */
    Stage stage;
    Screen screen;
    float remainingTime;
    unsigned int randomState;
    int active_money, moneyloop, goblin_spawn, goblinloop;
    int monmon_spawn, monmonloop, meatloop, active_meat;
    moneyAtt money[MONEY_AMOUNT];
    monster goblins[GOBLIN_CAPACITY];
    monmon monmons[MONMON_AMOUNT];
    Vector2 meat;
} GameState;
typedef struct { GameBool left, right, up, down, attack; } GameInput;

int CoreRandom(unsigned int *randomState);
void InitGame(GameState *game);
void InitWorld(sTile world[MAP_WIDTH][MAP_HEIGHT], int numTiles, unsigned int *randomState);
void ShuffleTile(int array[], int n, unsigned int *randomState);
monster GenerateGoblinPosition(Stage *gamestage, unsigned int *randomState);
monmon GenerateMonMonPosition(Stage *gamestage, unsigned int *randomState);
Vector2 GenerateMeatPosition(unsigned int *randomState);
moneyAtt GenerateMoneyPosition(unsigned int *randomState);
void CollectMoney(int *active_money, moneyAtt moneyPosArray[], player *player);
void CollectMeat(player *player, Vector2 meat_post, int *active_meat, int *meatloop);
int CheckGoblinCollision(player player, monster goblinPos);
int CheckMonmonCollision(player player, monmon monmonPos);
void CollideGoblin(player *player, int *goblin_spawn, monster goblinPosArray[], float currentTime);
void CollideMonmon(player *player, int *monmon_spawn, monmon monmonPosArray[], float currentTime);
void UpdateObjects(GameState *game);
void UpdateStage(GameState *game, float deltaTime);
void MovePlayer(player *p, GameInput input);
void UpdatePlayerLife(player *p, Screen *screen);
void UpdateGame(GameState *game, const GameInput inputs[2], float deltaTime, float currentTime);
void UpdateDeathAnimations(GameState *game, int currentFrame, int frameloop);
int CalculateScore(player *player, float *remainingTime);
#endif
