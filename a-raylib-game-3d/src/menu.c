#include "raylib.h"
#include "menu.h"

static const int BUTTON_WIDTH = 200;
static const int BUTTON_HEIGHT = 50;

void DrawMainMenu(void) {
    int centerX = GetScreenWidth() / 2 - BUTTON_WIDTH / 2;
    int centerY = GetScreenHeight() / 2 - BUTTON_HEIGHT;

    DrawText("3D FPS GAME", centerX - 50, centerY - 100, 40, BLACK);
    
    Rectangle playBtn = {centerX, centerY, BUTTON_WIDTH, BUTTON_HEIGHT};
    DrawRectangleRec(playBtn, GRAY);
    DrawText("PLAY", centerX + 70, centerY + 15, 20, BLACK);
}

void DrawPauseMenu(void) {
    int centerX = GetScreenWidth() / 2 - BUTTON_WIDTH / 2;
    int centerY = GetScreenHeight() / 2 - BUTTON_HEIGHT;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RAYWHITE, 0.8f));
    DrawText("PAUSED", centerX + 20, centerY - 100, 40, BLACK);
    
    Rectangle resumeBtn = {centerX, centerY, BUTTON_WIDTH, BUTTON_HEIGHT};
    DrawRectangleRec(resumeBtn, GRAY);
    DrawText("RESUME", centerX + 60, centerY + 15, 20, BLACK);
}

bool HandleMainMenu(void) {
    int centerX = GetScreenWidth() / 2 - BUTTON_WIDTH / 2;
    int centerY = GetScreenHeight() / 2 - BUTTON_HEIGHT;
    
    Rectangle playBtn = {centerX, centerY, BUTTON_WIDTH, BUTTON_HEIGHT};
    Vector2 mousePoint = GetMousePosition();

    return (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
}

bool HandlePauseMenu(void) {
    int centerX = GetScreenWidth() / 2 - BUTTON_WIDTH / 2;
    int centerY = GetScreenHeight() / 2 - BUTTON_HEIGHT;
    
    Rectangle resumeBtn = {centerX, centerY, BUTTON_WIDTH, BUTTON_HEIGHT};
    Vector2 mousePoint = GetMousePosition();

    return (CheckCollisionPointRec(mousePoint, resumeBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
} 