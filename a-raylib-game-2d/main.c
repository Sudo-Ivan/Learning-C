#include "raylib.h"
#include "raymath.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 450
#define PLAYER_SPEED 5
#define COIN_SIZE 20
#define MAX_COINS 5
#define MAX_ASTEROIDS 8
#define ASTEROID_SPEED 2
#define COIN_SPEED 2

typedef struct {
    Vector2 position;
    float size;
    Color color;
} Player;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
} Coin;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float size;
    bool active;
} Asteroid;

Vector2 GetRandomSpawnPosition(void) {
    Vector2 pos;
    if (GetRandomValue(0, 1)) {
        pos.x = GetRandomValue(0, 1) ? -50 : WINDOW_WIDTH + 50;
        pos.y = GetRandomValue(-50, WINDOW_HEIGHT + 50);
    } else {
        pos.x = GetRandomValue(-50, WINDOW_WIDTH + 50);
        pos.y = GetRandomValue(0, 1) ? -50 : WINDOW_HEIGHT + 50;
    }
    return pos;
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space Collector");
    SetTargetFPS(60);

    Player player = {
        .position = (Vector2){WINDOW_WIDTH/2, WINDOW_HEIGHT/2},
        .size = 30,
        .color = RED
    };

    Coin coins[MAX_COINS] = {0};
    Asteroid asteroids[MAX_ASTEROIDS] = {0};
    int score = 0;
    bool gameOver = false;

    for (int i = 0; i < MAX_COINS; i++) {
        coins[i].position = GetRandomSpawnPosition();
        coins[i].velocity = (Vector2){
            GetRandomValue(-COIN_SPEED, COIN_SPEED),
            GetRandomValue(-COIN_SPEED, COIN_SPEED)
        };
        coins[i].active = true;
    }

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].position = GetRandomSpawnPosition();
        asteroids[i].velocity = (Vector2){
            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED),
            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED)
        };
        asteroids[i].size = GetRandomValue(20, 40);
        asteroids[i].active = true;
    }

    while (!WindowShouldClose()) {
        if (!gameOver) {
            if (IsKeyDown(KEY_RIGHT)) player.position.x += PLAYER_SPEED;
            if (IsKeyDown(KEY_LEFT)) player.position.x -= PLAYER_SPEED;
            if (IsKeyDown(KEY_UP)) player.position.y -= PLAYER_SPEED;
            if (IsKeyDown(KEY_DOWN)) player.position.y += PLAYER_SPEED;

            player.position.x = Clamp(player.position.x, player.size/2, WINDOW_WIDTH - player.size/2);
            player.position.y = Clamp(player.position.y, player.size/2, WINDOW_HEIGHT - player.size/2);

            for (int i = 0; i < MAX_COINS; i++) {
                if (coins[i].active) {
                    coins[i].position = Vector2Add(coins[i].position, coins[i].velocity);
                    if (coins[i].position.x < -50) coins[i].position.x = WINDOW_WIDTH + 50;
                    if (coins[i].position.x > WINDOW_WIDTH + 50) coins[i].position.x = -50;
                    if (coins[i].position.y < -50) coins[i].position.y = WINDOW_HEIGHT + 50;
                    if (coins[i].position.y > WINDOW_HEIGHT + 50) coins[i].position.y = -50;

                    if (CheckCollisionCircles(player.position, player.size/2,
                        coins[i].position, COIN_SIZE/2)) {
                        coins[i].position = GetRandomSpawnPosition();
                        coins[i].velocity = (Vector2){
                            GetRandomValue(-COIN_SPEED, COIN_SPEED),
                            GetRandomValue(-COIN_SPEED, COIN_SPEED)
                        };
                        score++;
                    }
                }
            }

            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (asteroids[i].active) {
                    asteroids[i].position = Vector2Add(asteroids[i].position, asteroids[i].velocity);
                    if (asteroids[i].position.x < -50) asteroids[i].position.x = WINDOW_WIDTH + 50;
                    if (asteroids[i].position.x > WINDOW_WIDTH + 50) asteroids[i].position.x = -50;
                    if (asteroids[i].position.y < -50) asteroids[i].position.y = WINDOW_HEIGHT + 50;
                    if (asteroids[i].position.y > WINDOW_HEIGHT + 50) asteroids[i].position.y = -50;

                    if (CheckCollisionCircles(player.position, player.size/2,
                        asteroids[i].position, asteroids[i].size/2)) {
                        gameOver = true;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (!gameOver) {
            DrawCircleV(player.position, player.size/2, player.color);

            for (int i = 0; i < MAX_COINS; i++) {
                if (coins[i].active) {
                    DrawCircleV(coins[i].position, COIN_SIZE/2, GOLD);
                }
            }

            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (asteroids[i].active) {
                    DrawCircleV(asteroids[i].position, asteroids[i].size/2, GRAY);
                }
            }

            DrawText(TextFormat("Score: %d", score), 20, 20, 20, WHITE);
        } else {
            DrawText("GAME OVER", WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 30, 40, RED);
            DrawText(TextFormat("Final Score: %d", score), WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 + 20, 30, WHITE);
            DrawText("Press ENTER to restart", WINDOW_WIDTH/2 - 120, WINDOW_HEIGHT/2 + 60, 20, WHITE);

            if (IsKeyPressed(KEY_ENTER)) {
                score = 0;
                gameOver = false;
                player.position = (Vector2){WINDOW_WIDTH/2, WINDOW_HEIGHT/2};
                
                for (int i = 0; i < MAX_COINS; i++) {
                    coins[i].position = GetRandomSpawnPosition();
                    coins[i].velocity = (Vector2){
                        GetRandomValue(-COIN_SPEED, COIN_SPEED),
                        GetRandomValue(-COIN_SPEED, COIN_SPEED)
                    };
                }
                
                for (int i = 0; i < MAX_ASTEROIDS; i++) {
                    asteroids[i].position = GetRandomSpawnPosition();
                    asteroids[i].velocity = (Vector2){
                        GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED),
                        GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED)
                    };
                }
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
} 