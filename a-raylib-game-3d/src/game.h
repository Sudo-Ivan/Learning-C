#ifndef GAME_H
#define GAME_H

#include "raylib.h"

typedef enum {
    MENU,
    GAMEPLAY,
    PAUSE
} GameState;

void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void UnloadGame(void);

#endif 