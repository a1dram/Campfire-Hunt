#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include "GameCore.h"
#include "../Build/tests/original_reference.h"

static int Checks;
#define CHECK(x) do { ++Checks; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static GameState Game;
static sTile World[MAP_WIDTH][MAP_HEIGHT];

static void Initialization(void) {
    int i;
    memset(&Game, 0xa5, sizeof(Game));
    InitGame(&Game);
    CHECK(Game.remainingTime == 360 && Game.screen.showMenu && !Game.screen.gameOver);
    CHECK(Game.active_money == 0 && Game.goblin_spawn == 0 && Game.monmon_spawn == 0);
    CHECK(!Game.stage.stage1 && !Game.stage.stage2 && !Game.stage.stage3);
    for (i = 0; i < 3; ++i) {
        CHECK(Game.players[i].health == 5 && Game.players[i].life == 3);
        CHECK(Game.players[i].money_collected == 0 && Game.players[i].goblin_killed == 0);
        CHECK(Game.players[i].stage == 1 && Game.players[i].lastAttack == 0);
        CHECK(Game.players[i].position.y == 400);
    }
    CHECK(Game.players[0].position.x == 600 && Game.players[1].position.x == 300 && Game.players[2].position.x == 900);
}

static void GeneratorsAndReference(void) {
    unsigned int seed = 1;
    int i, stage;
    srand(1);
    for (i = 0; i < 10000; ++i) CHECK(CoreRandom(&seed) == rand());
    for (stage = 1; stage <= 3; ++stage) {
        Stage s = {(GameBool)(stage == 1), (GameBool)(stage == 2), (GameBool)(stage == 3)};
        for (i = 1; i <= 1000; ++i) {
            monster g, rg;
            monmon m, rm;
            moneyAtt coin, rc;
            Vector2 meat, rt;
            seed = (unsigned int)i;
            srand((unsigned int)i);
            g = GenerateGoblinPosition(&s, &seed);
            rg = ReferenceGenerateGoblinPosition(&s);
            CHECK(g.position.x == rg.position.x && g.position.y == rg.position.y && g.health == rg.health && g.color == rg.color);
            CHECK(g.velocity.x == rg.velocity.x && g.velocity.y == rg.velocity.y);
            CHECK(g.position.x >= 240 && g.position.x <= 859 && g.position.y >= 240 && g.position.y <= 459);
            CHECK(g.health == (stage == 2 ? 4 : 5) && g.color >= 0 && g.color < 4);
            CHECK(g.lastAttack == 0 && !g.dying && !g.attack && g.deathFrame == 0);
            m = GenerateMonMonPosition(&s, &seed);
            rm = ReferenceGenerateMonMonPosition(&s);
            CHECK(m.position.x == rm.position.x && m.position.y == rm.position.y && m.color == rm.color);
            CHECK(m.velocity.x == rm.velocity.x && m.velocity.y == rm.velocity.y && m.health == 3);
            CHECK(m.velocity.x >= -1.5f && m.velocity.x <= 1.5f && !m.dying);
            coin = GenerateMoneyPosition(&seed);
            rc = ReferenceGenerateMoneyPosition();
            CHECK(coin.x == rc.x && coin.y == rc.y && coin.expiry == rc.expiry);
            CHECK(coin.x >= 240 && coin.x <= 959 && coin.y >= 240 && coin.y <= 559 && coin.expiry == 420);
            meat = GenerateMeatPosition(&seed);
            rt = ReferenceGenerateMeatPosition();
            CHECK(meat.x == rt.x && meat.y == rt.y);
        }
    }
}

static void CombatAndReference(void) {
    player p = {0}, rp;
    monster g = {0}, rg;
    monmon m = {0}, rm;
    Screen screen = {0};
    int count = 1, n;
    p.position = (Vector2){250, 260}; p.health = 5; p.life = 3;
    g.position = m.position = (Vector2){240, 240}; g.health = 4; m.health = 3;
    CHECK(CheckGoblinCollision(p, g) == ReferenceCheckGoblinCollision(p, g));
    CHECK(CheckMonmonCollision(p, m) == ReferenceCheckMonmonCollision(p, m));
    CollideGoblin(&p, &count, &g, 2.0f);
    CHECK(p.health == 5 && !g.attack); /* Strictly greater than 2 seconds. */
    rp = p; rg = g; ReferenceTime = 2.01f;
    CollideGoblin(&p, &count, &g, ReferenceTime);
    ReferenceCollideGoblin(&rp, &count, &rg);
    CHECK(p.health == 4 && p.health == rp.health && g.lastAttack == rg.lastAttack && g.attack == rg.attack);
    CollideGoblin(&p, &count, &g, 4.0f); CHECK(p.health == 4);
    CollideGoblin(&p, &count, &g, 4.02f); CHECK(p.health == 3);
    p.isAttack = 1;
    for (n = 0; n < 4; ++n) {
        rp = p; rg = g; ReferenceTime = 4.1f;
        CollideGoblin(&p, &count, &g, ReferenceTime);
        ReferenceCollideGoblin(&rp, &count, &rg);
        CHECK(p.health == rp.health && p.goblin_killed == rp.goblin_killed && g.health == rg.health && g.dying == rg.dying);
    }
    CHECK(p.goblin_killed == 1 && g.dying && g.velocity.x == 0 && !g.attack);
    CollideGoblin(&p, &count, &g, 4.1f); CHECK(p.goblin_killed == 1);
    for (n = 0; n < 3; ++n) {
        rp = p; rm = m; ReferenceTime = 5;
        CollideMonmon(&p, &count, &m, ReferenceTime);
        ReferenceCollideMonmon(&rp, &count, &rm);
        CHECK(m.health == rm.health && m.dying == rm.dying && p.money_collected == rp.money_collected);
    }
    CHECK(p.money_collected == 3 && m.dying);
    CollideMonmon(&p, &count, &m, 5); CHECK(p.money_collected == 3);
    p.health = 0; UpdatePlayerLife(&p, &screen); CHECK(p.life == 2 && p.health == 5 && !screen.gameOver);
    p.health = -2; UpdatePlayerLife(&p, &screen); CHECK(p.life == 1 && p.health == 5);
    p.health = 0; UpdatePlayerLife(&p, &screen); CHECK(p.life == 0 && screen.gameOver);
    p.position = (Vector2){235, 260}; CHECK(!CheckGoblinCollision(p, g));
    p.position.x = 330; CHECK(!CheckGoblinCollision(p, g));
    p.position = (Vector2){250, 250}; CHECK(!CheckGoblinCollision(p, g));
}

static void CoinsAndHealing(void) {
    player p = {0}, rp;
    moneyAtt coins[3] = {{240,240,420}, {240,240,1}, {500,400,1}};
    int count = 3, active = 1, loop = 100, ra, rl;
    Vector2 meat = {240,240};
    p.position = (Vector2){250,260}; p.health = 1; p.life = 2;
    CollectMoney(&count, coins, &p);
    CHECK(count == 0 && p.money_collected == 2); /* Adjacent removal and expiry. */
    CollectMoney(&count, coins, &p); CHECK(count == 0);
    coins[0] = (moneyAtt){240,240,1}; count = 1;
    CollectMoney(&count, coins, &p); CHECK(count == 0 && p.money_collected == 3);
    coins[0] = (moneyAtt){240,240,2}; count = 1; p.position.x = 240;
    CollectMoney(&count, coins, &p); CHECK(count == 1 && coins[0].expiry == 1 && p.money_collected == 3);
    CollectMoney(&count, coins, &p); CHECK(count == 0);
    p.position.x = 250; rp = p; ra = active; rl = loop;
    CollectMeat(&p, meat, &active, &loop);
    ReferenceCollectMeat(&rp, meat, &ra, &rl);
    CHECK(p.health == 5 && p.life == 2 && active == 0 && loop == 0);
    CHECK(p.health == rp.health && active == ra && loop == rl);
}

static void StagesScoresAndMovement(void) {
    GameState reference;
    GameInput input[2] = {{0}, {0}};
    int tick, stage, life;
    InitGame(&Game); InitGame(&reference);
    for (tick = 0; tick < 362; ++tick) {
        UpdateStage(&Game, 1); ReferenceStage(&reference, 1);
        CHECK(Game.remainingTime == reference.remainingTime && Game.players[0].stage == reference.players[0].stage);
        CHECK(Game.stage.stage1 == reference.stage.stage1 && Game.stage.stage2 == reference.stage.stage2 && Game.stage.stage3 == reference.stage.stage3);
        CHECK(Game.screen.gameOver == reference.screen.gameOver);
    }
    InitGame(&Game); UpdateStage(&Game, 60); CHECK(Game.stage.stage1 && Game.players[0].stage == 1);
    UpdateStage(&Game, 0.01f); CHECK(Game.stage.stage2 && Game.players[0].stage == 2);
    Game.remainingTime = 120; UpdateStage(&Game, 0); CHECK(Game.stage.stage2);
    UpdateStage(&Game, 0.01f); CHECK(Game.stage.stage3 && Game.players[0].stage == 3);
    for (stage = 1; stage <= 3; ++stage) for (life = 0; life <= 3; ++life) {
        float remaining = 0;
        int expected = 2*500 + 7*100 + (stage == 2 ? 100 + life*100 : stage == 3 ? 500 + life*300 : 0) + 500;
        player p = {0}; p.stage = stage; p.life = life; p.goblin_killed = 2; p.money_collected = 7;
        CHECK(CalculateScore(&p, &remaining) == expected);
        CHECK(CalculateScore(&p, &remaining) == ReferenceCalculateScore(&p, &remaining));
    }
    InitGame(&Game); input[0].left = input[0].up = 1;
    UpdateGame(&Game, input, 1.0f/60.0f, 0); CHECK(Game.players[0].position.x == 599 && Game.players[0].position.y == 399);
    Game.players[0].position = (Vector2){240,240}; UpdateGame(&Game, input, 0, 0);
    CHECK(Game.players[0].position.x == 240 && Game.players[0].position.y == 240);
    input[0].left = input[0].up = 0; input[0].right = input[0].down = 1;
    Game.players[0].position = (Vector2){960,560}; UpdateGame(&Game, input, 0, 0);
    CHECK(Game.players[0].position.x == 960 && Game.players[0].position.y == 560);
    Game.screen.pause = 1; UpdateGame(&Game, input, 100, 0); CHECK(Game.remainingTime > 359 && Game.moneyloop == 3);
    InitGame(&Game); Game.screen.multiplayer = 1; UpdateGame(&Game, input, 61, 1);
    CHECK(Game.players[0].stage == 1 && Game.players[1].stage == 2 && Game.players[2].stage == 2);
    CHECK(Game.players[1].position.x == 301 && Game.players[2].position.x == 900);
    Game.players[2].health = 0; Game.players[2].life = 1; UpdateGame(&Game, input, 0, 1);
    CHECK(Game.screen.gameOver && Game.players[2].life == 0);
}

static void SpawnCountersAndMap(void) {
    int i, j, dirt = 0, a[20], b[20];
    unsigned int seed = 1;
    InitGame(&Game); Game.stage.stage1 = 1;
    for (i = 0; i < 160; ++i) UpdateObjects(&Game);
    CHECK(Game.active_money == 0 && Game.moneyloop == 160);
    UpdateObjects(&Game); CHECK(Game.active_money == 1 && Game.moneyloop == 1);
    Game.monmonloop = 1500; UpdateObjects(&Game); CHECK(Game.monmon_spawn == 1 && Game.monmonloop == 1);
    CHECK(Game.goblin_spawn == 0 && Game.meatloop == 0);
    Game.stage.stage1 = 0; Game.stage.stage2 = 1;
    Game.goblinloop = 900; Game.meatloop = 2000; UpdateObjects(&Game);
    CHECK(Game.goblin_spawn == 1 && Game.goblins[0].health == 4 && Game.active_meat == 1);
    for (i = 0; i < 10000; ++i) UpdateObjects(&Game);
    CHECK(Game.goblin_spawn == 5 && Game.monmon_spawn == 3 && Game.active_money == 10);
    Game.stage.stage2 = 0; Game.stage.stage3 = 1;
    for (i = 0; i < 3000; ++i) UpdateObjects(&Game);
    CHECK(Game.goblin_spawn == 8 && Game.goblins[7].health == 5);
    Game.moneyloop = Game.monmonloop = Game.goblinloop = Game.meatloop = INT_MAX;
    UpdateObjects(&Game); CHECK(Game.moneyloop == INT_MAX && Game.monmonloop == INT_MAX && Game.goblinloop == INT_MAX && Game.meatloop == INT_MAX);
    Game.goblins[0].position = (Vector2){239,239}; Game.goblins[0].velocity = (Vector2){-1,-1};
    UpdateObjects(&Game); CHECK(Game.goblins[0].velocity.x == 1 && Game.goblins[0].velocity.y == 1);
    Game.goblins[0].dying = Game.monmons[0].dying = 1;
    UpdateDeathAnimations(&Game, 1, 1); CHECK(Game.goblin_spawn == 8 && Game.monmon_spawn == 3);
    UpdateDeathAnimations(&Game, 0, 3); CHECK(Game.goblin_spawn == 7 && Game.monmon_spawn == 2);
    for (i = 0; i < 20; ++i) a[i] = b[i] = i;
    srand(1); ReferenceShuffleTile(a, 20); ShuffleTile(b, 20, &seed);
    CHECK(memcmp(a, b, sizeof(a)) == 0);
    InitWorld(World, 6, &seed);
    for (i = 0; i < 85; ++i) for (j = 0; j < 90; ++j) {
        CHECK(World[i][j].x == i && World[i][j].y == j && World[i][j].tileIndex >= 0 && World[i][j].tileIndex < 6);
        if (World[i][j].tileIndex == 5) ++dirt;
    }
    CHECK(dirt == 100);
}

int main(void) {
    Initialization(); GeneratorsAndReference(); CombatAndReference(); CoinsAndHealing();
    StagesScoresAndMovement(); SpawnCountersAndMap();
    printf("PASS: %d checks; 6 gameplay groups, original-source differential references; Windows host only\n", Checks);
    return 0;
}
