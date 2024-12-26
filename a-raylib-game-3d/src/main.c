#include "raylib.h"
#include "game.h"
#include "menu.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable 4x MSAA
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "3D FPS Game");
    SetTargetFPS(60);

    GameState gameState = MENU;
    InitGame();

    while (!WindowShouldClose()) {
        switch(gameState) {
            case MENU:
                if (HandleMainMenu()) gameState = GAMEPLAY;
                break;
            case GAMEPLAY:
                if (IsKeyPressed(KEY_ESCAPE)) gameState = PAUSE;
                UpdateGame();
                break;
            case PAUSE:
                if (HandlePauseMenu()) gameState = GAMEPLAY;
                break;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            
            switch(gameState) {
                case MENU:
                    DrawMainMenu();
                    break;
                case GAMEPLAY:
                    DrawGame();
                    break;
                case PAUSE:
                    DrawGame();
                    DrawPauseMenu();
                    break;
            }
        EndDrawing();
    }

    UnloadGame();
    CloseWindow();
    return 0;
} 