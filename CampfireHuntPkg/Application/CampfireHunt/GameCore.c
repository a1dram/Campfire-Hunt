/* Gameplay extracted from CampfireHunt.c (Windows source left unchanged).
 * Texture/sound/camera arguments removed; time and random state are explicit.
 */
#include "GameCore.h"

/* Counters keep the original frame units; saturation avoids signed overflow
 * when a full object array prevents its spawn counter from being reset. */
static void AdvanceCounter(int *counter) {
    if (*counter < 2147483647) ++*counter;
}

/* MSVC rand() recurrence, with explicit per-game state and no C runtime. */
int CoreRandom(unsigned int *randomState) {
    *randomState = *randomState * 214013u + 2531011u;
    return (int)((*randomState >> 16) & 32767u);
}

/* Original: GenerateGoblinPosition */
monster GenerateGoblinPosition(Stage *gamestage, unsigned int *randomState) {
    monster goblinPos = {0};
    goblinPos.position.x = (float)(MARGIN + CoreRandom(randomState) % (SCREENWIDTH - 2 * (MARGIN + 50) ));
    goblinPos.position.y = (float)(MARGIN + CoreRandom(randomState) % (SCREENHEIGHT - 2 * (MARGIN + 50) ));
    goblinPos.color = CoreRandom(randomState) % MAX_CHARACTER_TYPE;
    if (gamestage->stage2){
        goblinPos.health = GOBLIN_HEALTH;
    } else {
        goblinPos.health = GOBLIN_HEALTH + 1;
    }
    
    goblinPos.velocity.x = (float)(CoreRandom(randomState) % 3 - 1);
    goblinPos.velocity.y = (float)(CoreRandom(randomState) % 3 - 1);
    goblinPos.attack = 0;
    goblinPos.dying = 0;
    return goblinPos;
}

/* Original: GenerateMonMonPosition */
monmon GenerateMonMonPosition(Stage *gamestage, unsigned int *randomState) {
    monmon monmonPos = {0};
    monmonPos.position.x = (float)(MARGIN + CoreRandom(randomState) % (SCREENWIDTH - 2 * (MARGIN + 50) ));
    monmonPos.position.y = (float)(MARGIN + CoreRandom(randomState) % (SCREENHEIGHT - 2 * (MARGIN + 50) ));
    monmonPos.color = CoreRandom(randomState) % MAX_CHARACTER_TYPE;
    monmonPos.health = MONMON_HEALTH;
    monmonPos.dying = 0;
    if (gamestage->stage1){
        monmonPos.velocity.x = (float)(CoreRandom(randomState) % 3 - 1);
        monmonPos.velocity.y = (float)(CoreRandom(randomState) % 3 - 1);
    } else {
        monmonPos.velocity.x = (CoreRandom(randomState) % 3 - 1) * 1.5f; 
        monmonPos.velocity.y = (CoreRandom(randomState) % 3 - 1) * 1.5f;
    }

    return monmonPos;
}

/* Original: GenerateMeatPosition */
Vector2 GenerateMeatPosition(unsigned int *randomState){
    Vector2 meatPos;
    meatPos.x = (float)(MARGIN + CoreRandom(randomState) % (SCREENWIDTH - 2 * MARGIN));
    meatPos.y = (float)(MARGIN + CoreRandom(randomState) % (SCREENHEIGHT - 2 * MARGIN));
    return meatPos;
}

/* Original: GenerateMoneyPosition */
moneyAtt GenerateMoneyPosition(unsigned int *randomState) {
    moneyAtt moneyPos;
    moneyPos.x = MARGIN + CoreRandom(randomState) % (SCREENWIDTH - 2 * MARGIN);
    moneyPos.y = MARGIN + CoreRandom(randomState) % (SCREENHEIGHT - 2 * MARGIN);
    moneyPos.expiry = MONEY_EXPIRY;
    return moneyPos;
}

/* Original: ShuffleTile */
void ShuffleTile(int array[], int n, unsigned int *randomState) { //Use for shuffling the tiles
    for (int i = n - 1; i > 0; i--) {
        int j = CoreRandom(randomState) % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

/* Original: CollectMeat */
void CollectMeat(player *player, Vector2 meat_post, int *active_meat, int *meatloop){
    if (*active_meat > 0 && (player->position.x < meat_post.x + 60 &&
        player->position.x > meat_post.x &&
        player->position.y < meat_post.y + 60 &&
        player->position.y > meat_post.y)) {
        *active_meat = 0;
        *meatloop = 0;

        player->health = PLAYER_HEALTH;
    }
}

/* Original: CheckGoblinCollision */
int CheckGoblinCollision(player player, monster goblinPos) {
    return (player.position.x < goblinPos.position.x + 90 && //right
            player.position.x > goblinPos.position.x - 5 && //left
            player.position.y < goblinPos.position.y + 85 && //bottom
            player.position.y > goblinPos.position.y + 10); //top
}

/* Original: CollideGoblin */
void CollideGoblin(player *player, int *goblin_spawn, monster goblinPosArray[], float currentTime) {
    for (int i = 0; i < *goblin_spawn; i++) {

        if (CheckGoblinCollision(*player, goblinPosArray[i])){
            if (currentTime - goblinPosArray[i].lastAttack > 2.0f) { 
                goblinPosArray[i].attack = 1;
                player->health -= 1;
                goblinPosArray[i].lastAttack = currentTime;
            }
            if (player->isAttack) {
                goblinPosArray[i].health -= 1; 
                player->lastAttack = currentTime;
            }
        }
        
        if (goblinPosArray[i].health <= 0 && !goblinPosArray[i].dying) {

            goblinPosArray[i].dying = 1;
            goblinPosArray[i].attack = 0;
            goblinPosArray[i].velocity.x = 0;
            goblinPosArray[i].velocity.y = 0;
            goblinPosArray[i].deathFrame = 0;
            (player->goblin_killed)++;
        }
    }
}

/* Original: CheckMonmonCollision */
int CheckMonmonCollision(player player, monmon monmonPos) {
    return (player.position.x < monmonPos.position.x + 80 && //right
            player.position.x > monmonPos.position.x - 5 && //left
            player.position.y < monmonPos.position.y + 85 && //bottom
            player.position.y > monmonPos.position.y + 10); //top
}

/* Original: CollideMonmon */
void CollideMonmon(player *player, int *monmon_spawn, monmon monmonPosArray[], float currentTime) {
    for (int i = 0; i < *monmon_spawn; i++) {

        if (CheckMonmonCollision(*player, monmonPosArray[i])){
            if (player->isAttack) { 
                monmonPosArray[i].health -= 1;
                player->lastAttack = currentTime;
            }
        }
        if (monmonPosArray[i].health <= 0 && !monmonPosArray[i].dying) {
            monmonPosArray[i].dying = 1;
            monmonPosArray[i].velocity.x = 0;
            monmonPosArray[i].velocity.y = 0;
            monmonPosArray[i].deathFrame = 0;
            (player->money_collected) += 3;

        }
    }
}

/* Original: CalculateScore */
int CalculateScore(player *player, float *remainingTime){
    int calc_score = 0;
    calc_score += player->goblin_killed * 500; 
    calc_score += player->money_collected * 100;
    if (player->stage == 2){
        calc_score += 100;
        calc_score += player->life * 100;
    } else if (player->stage == 3){
        calc_score += 500;
        calc_score += player->life * 300;
    }
    
    if (*remainingTime <= 0){
        calc_score += 500;
    }

    return calc_score;
}

/* Original UpdateObjects: fixed-capacity arrays replace unsafe local realloc.
 * Limits are still five goblins in stage 2, eight in stage 3, three monmons.
 */
void UpdateObjects(GameState *game) {
    int *active_money = &game->active_money, *moneyloop = &game->moneyloop;
    int *monmonloop = &game->monmonloop, *monmon_spawn = &game->monmon_spawn;
    int *meatloop = &game->meatloop, *active_meat = &game->active_meat;
    int *goblinloop = &game->goblinloop, *goblin_spawn = &game->goblin_spawn;
    int goblin_amount;
    Stage *gamestage = &game->stage;
    monster *goblinPosArray = game->goblins;
    monmon *monmonPosArray = game->monmons;
    moneyAtt *moneyPosArray = game->money;
    Vector2 *meat_post = &game->meat;

    if (*moneyloop >= GENERATE_INTERVAL && *active_money < MONEY_AMOUNT) {
        moneyPosArray[*active_money] = GenerateMoneyPosition(&game->randomState);
        (*active_money)++;
        *moneyloop = 0;
    }
    AdvanceCounter(moneyloop);

    if (*monmonloop >= MONMON_INTERVAL && *monmon_spawn < MONMON_AMOUNT){
        monmonPosArray[*monmon_spawn] = GenerateMonMonPosition(gamestage, &game->randomState);
        (*monmon_spawn)++;
        *monmonloop = 0;
    }
    AdvanceCounter(monmonloop);

    for (int i = 0; i < *monmon_spawn; i++){
        monmonPosArray[i].position.x += monmonPosArray[i].velocity.x;
        monmonPosArray[i].position.y += monmonPosArray[i].velocity.y;

        if (monmonPosArray[i].position.x < MARGIN || monmonPosArray[i].position.x > SCREENWIDTH - MARGIN - 100) {
            monmonPosArray[i].velocity.x *= -1;
        }
        if (monmonPosArray[i].position.y < MARGIN || monmonPosArray[i].position.y > SCREENHEIGHT - MARGIN - 100) {
            monmonPosArray[i].velocity.y *= -1;
        }
    }

    if (gamestage->stage2 || gamestage->stage3) {
        if (gamestage->stage3){
            goblin_amount = 8;
        } else {
            goblin_amount = 5;
        }


        if (*meatloop >= MEAT_INTERVAL && !*active_meat){
            *meat_post = GenerateMeatPosition(&game->randomState);
            *active_meat = 1;
        }
        AdvanceCounter(meatloop);

        if (*goblinloop >= MONSTER_INTERVAL && *goblin_spawn < goblin_amount) {
            goblinPosArray[*goblin_spawn] = GenerateGoblinPosition(gamestage, &game->randomState);
            (*goblin_spawn)++;
            *goblinloop = 0;
        }
        AdvanceCounter(goblinloop);

        for (int i = 0; i < *goblin_spawn; i++){
            goblinPosArray[i].position.x += goblinPosArray[i].velocity.x;
            goblinPosArray[i].position.y += goblinPosArray[i].velocity.y;

            if (goblinPosArray[i].position.x < MARGIN || goblinPosArray[i].position.x > SCREENWIDTH - MARGIN - 100) {
                goblinPosArray[i].velocity.x *= -1; 
            }
            if (goblinPosArray[i].position.y < MARGIN || goblinPosArray[i].position.y > SCREENHEIGHT - MARGIN - 100) {
                goblinPosArray[i].velocity.y *= -1; 
            }
        }
    }
}

/* Original InitGame, excluding cameras and rendered map. */
void InitGame(GameState *game) {
    player *player1 = &game->players[0], *player2 = &game->players[1], *player3 = &game->players[2];
    unsigned char *bytes = (unsigned char *)game;
    unsigned int i;
    for (i = 0; i < sizeof(*game); ++i) bytes[i] = 0;
    game->remainingTime = TIMELIMIT;
    game->randomState = 1;
    game->screen.showMenu = 1;
    //Initialize singleplayer
    player1->position = (Vector2){SCREENWIDTH/2, SCREENHEIGHT/2};
    player1->life = PLAYER_MAX_LIFE;
    player1->health = PLAYER_HEALTH;
    player1->money_collected = 0;
    player1->goblin_killed = 0;
    player1->stage = 1;

    //Initialize multiplayer
    player2->position = (Vector2){SCREENWIDTH/4, SCREENHEIGHT/2};
    player2->life = PLAYER_MAX_LIFE;
    player2->health = PLAYER_HEALTH;
    player2->money_collected = 0;
    player2->goblin_killed = 0;
    player2->stage = 1;
    player3->position = (Vector2){SCREENWIDTH*3/4, SCREENHEIGHT/2};
    player3->life = PLAYER_MAX_LIFE;
    player3->health = PLAYER_HEALTH;
    player3->money_collected = 0;
    player3->goblin_killed = 0;
    player3->stage = 1;

}

/* Original map distribution: 100 dirt tiles, then random grass, shuffled.
 * Caller supplies storage so the UEFI entry point need not put it on its stack.
 */
void InitWorld(sTile world[MAP_WIDTH][MAP_HEIGHT], int numTiles, unsigned int *randomState) {
    if (numTiles < 2) return;
    int tileMap[MAP_WIDTH * MAP_HEIGHT];
    for (int i = 0; i < 100; i++) { //Dirt tile
        tileMap[i] = numTiles - 1;  
    }
    for (int i = 100; i < MAP_WIDTH * MAP_HEIGHT; i++) {
        tileMap[i] = (CoreRandom(randomState) % (numTiles - 1));  
    }

    ShuffleTile(tileMap, MAP_WIDTH * MAP_HEIGHT, randomState);

    int idx = 0;
    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_HEIGHT; j++) {
            world[i][j].x = i; 
            world[i][j].y = j; 
            world[i][j].tileIndex = tileMap[idx++];
        }
    }

}

/* Original CollectMoney: process each entry once, including after compaction.
 * The original could remove the last expired coin twice (count became -1),
 * or skip the coin moved into the removed slot. Reward remains one coin.
 */
void CollectMoney(int *active_money, moneyAtt moneyPosArray[], player *p) {
    int i = 0;
    while (i < *active_money) {
        int j;
        int collected;
        --moneyPosArray[i].expiry;
        collected = p->position.x < moneyPosArray[i].x + 60 &&
                    p->position.x > moneyPosArray[i].x &&
                    p->position.y < moneyPosArray[i].y + 60 &&
                    p->position.y > moneyPosArray[i].y;
        if (collected || moneyPosArray[i].expiry <= 0) {
            if (collected) ++p->money_collected;
            for (j = i; j < *active_money - 1; ++j)
                moneyPosArray[j] = moneyPosArray[j + 1];
            --*active_money;
        } else {
            ++i;
        }
    }
}

/* Stage thresholds from UpdateSingleplayerGame / UpdateMultiplayerGame. */
void UpdateStage(GameState *game, float deltaTime) {
    int first = game->screen.multiplayer ? 1 : 0;
    int last = game->screen.multiplayer ? 2 : 0;
    int i;
    game->remainingTime -= deltaTime;
    if (game->remainingTime >= 300) {
        game->stage.stage1 = 1;
    } else if (game->remainingTime >= 120) {
        game->stage.stage1 = 0;
        game->stage.stage2 = 1;
        for (i = first; i <= last; ++i) game->players[i].stage = 2;
    } else if (game->remainingTime > 0) {
        game->stage.stage2 = 0;
        game->stage.stage3 = 1;
        for (i = first; i <= last; ++i) game->players[i].stage = 3;
    } else {
        game->stage.stage3 = 0;
        game->remainingTime = 0;
        game->screen.gameOver = 1;
    }
}

/* Input represents the original key states; displacement is one unit/frame. */
void MovePlayer(player *p, GameInput input) {
    if (input.left) p->position.x -= 1;
    if (input.right) p->position.x += 1;
    if (input.up) p->position.y -= 1;
    if (input.down) p->position.y += 1;
}

static void ClampPlayer(player *p) {
    if (p->position.x < MARGIN) p->position.x = MARGIN;
    else if (p->position.x > SCREENWIDTH - MARGIN) p->position.x = SCREENWIDTH - MARGIN;
    if (p->position.y < MARGIN) p->position.y = MARGIN;
    else if (p->position.y > SCREENHEIGHT - MARGIN) p->position.y = SCREENHEIGHT - MARGIN;
}

void UpdatePlayerLife(player *p, Screen *screen) {
    if (p->health <= 0) {
        --p->life;
        p->health = PLAYER_HEALTH;
    }
    if (p->life <= 0) screen->gameOver = 1;
}

/* Both original update paths share the same rules. Camera/audio and physical
 * input polling are replaced by parameters; collection retains main() order.
 */
void UpdateGame(GameState *game, const GameInput inputs[2], float deltaTime, float currentTime) {
    int first = game->screen.multiplayer ? 1 : 0;
    int count = game->screen.multiplayer ? 2 : 1;
    int i;
    for (i = 0; i < count; ++i) game->players[first + i].isAttack = 0;
    if (!game->screen.gameOver && !game->screen.pause) {
        UpdateStage(game, deltaTime);
        UpdateObjects(game);
        for (i = 0; i < count; ++i) {
            player *p = &game->players[first + i];
            MovePlayer(p, inputs[i]);
            if (inputs[i].attack && currentTime - p->lastAttack > 0.8f) p->isAttack = 1;
        }
        for (i = 0; i < count; ++i) {
            player *p = &game->players[first + i];
            CollideGoblin(p, &game->goblin_spawn, game->goblins, currentTime);
            CollideMonmon(p, &game->monmon_spawn, game->monmons, currentTime);
        }
        for (i = 0; i < count; ++i) {
            player *p = &game->players[first + i];
            ClampPlayer(p);
            UpdatePlayerLife(p, &game->screen);
        }
    }
    /* In the original these calls are outside pause/gameOver checks, and
     * multiplayer decrements coin expiry once per player. Preserve that. */
    for (i = 0; i < count; ++i) {
        player *p = &game->players[first + i];
        CollectMeat(p, game->meat, &game->active_meat, &game->meatloop);
        CollectMoney(&game->active_money, game->money, p);
    }
}

/* State changes originally embedded in DrawMap, without drawing calls. */
void UpdateDeathAnimations(GameState *game, int currentFrame, int frameloop) {
    int i, j;
    for (i = 0; i < game->monmon_spawn; ++i) {
        if (game->monmons[i].dying) {
            game->monmons[i].deathFrame = currentFrame % 14;
            if (currentFrame % 14 == 0) {
                for (j = i; j < game->monmon_spawn - 1; ++j) game->monmons[j] = game->monmons[j + 1];
                --game->monmon_spawn;
                --i;
            }
        }
    }
    for (i = 0; i < game->goblin_spawn; ++i) {
        if (game->goblins[i].dying) {
            game->goblins[i].deathFrame = currentFrame % 14;
            if (currentFrame % 14 == 0) {
                for (j = i; j < game->goblin_spawn - 1; ++j) game->goblins[j] = game->goblins[j + 1];
                --game->goblin_spawn;
                --i;
            }
        } else if (game->goblins[i].attack && frameloop % 3 == 0) {
            game->goblins[i].attack = 0;
        }
    }
}
