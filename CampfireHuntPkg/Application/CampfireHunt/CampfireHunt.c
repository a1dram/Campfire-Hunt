#include <Uefi.h>
#include <Library/UefiLib.h>
#include "GameCore.h"

/* MSVC's floating point marker; the core needs no hosted C runtime. */
int _fltused = 0;

static GameState Game;
static sTile World[MAP_WIDTH][MAP_HEIGHT];

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  GameInput Inputs[2] = {{0}, {0}};
  int Frame;
  int Score;
  (VOID)ImageHandle;
  (VOID)SystemTable;

  Print (L"Campfire Hunt: original gameplay core, headless UEFI port\r\n");
  InitGame (&Game);
  InitWorld (World, 6, &Game.randomState);
  Game.screen.showMenu = 0;
  Game.screen.singleplayer = 1;
  /* Deterministic diagnostic simulation, not a graphics/input frontend.
   * Each tick uses the original 60 FPS frame counters and movement rules. */
  for (Frame = 0; Frame < 21610 && !Game.screen.gameOver; ++Frame) {
    Inputs[0].left = (GameBool)((Frame / 180) % 2);
    Inputs[0].right = (GameBool)!Inputs[0].left;
    Inputs[0].attack = 1;
    UpdateGame (&Game, Inputs, 1.0f / 60.0f, (float)Frame / 60.0f);
    UpdateDeathAnimations (&Game, (Frame / 6) % 6, Frame / 36);
  }
  Score = CalculateScore (&Game.players[0], &Game.remainingTime);
  Print (L"Frames=%d stage=%d health=%d lives=%d coins=%d kills=%d score=%d\r\n",
         Frame, Game.players[0].stage, Game.players[0].health,
         Game.players[0].life, Game.players[0].money_collected,
         Game.players[0].goblin_killed, Score);
  Print (L"Objects: goblins=%d monmons=%d coins=%d; map=%d x %d, tile=%d\r\n",
         Game.goblin_spawn, Game.monmon_spawn, Game.active_money,
         MAP_WIDTH, MAP_HEIGHT, World[0][0].tileIndex);
  return EFI_SUCCESS;
}
