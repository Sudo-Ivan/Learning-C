#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include "game_defs.h"
#include "AI.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 450
#define PLAYER_SPEED 5
#define COIN_SIZE 20
#define MAX_COINS 5
#define MAX_ASTEROIDS 8
#define ASTEROID_SPEED 2
#define COIN_SPEED 2
#define WORLD_SIZE 2000
#define HUD_MARGIN 10
#define HUD_TEXT_SIZE 16
#define PLAYER_SPEED_DIAGONAL (PLAYER_SPEED * 0.707f) // For smoother diagonal movement
#define PLAYER_BASE_ROTATION 0.0f  // 0 degrees
#define BEAM_SPEED 10.0f
#define BEAM_LIFETIME 0.5f
#define ASTEROID_HITS 3
#define MAX_BEAMS 20
#define SHOOT_COOLDOWN 0.2f
#define MAX_STATIONS 5
#define STATION_MIN_SIZE 120
#define STATION_MAX_SIZE 180
#define MIN_STATION_DISTANCE 500  // Minimum distance between stations
#define PLAYER_MAX_HEALTH 100
#define HEALTH_BAR_WIDTH 200
#define HEALTH_BAR_HEIGHT 20

typedef struct {
    Vector2 position;
    float size;
    Color color;
    Texture2D texture;
    float rotation;
    float health;
} Player;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    Texture2D texture;
} Coin;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float size;
    bool active;
    Texture2D texture;
    int hits;
} Asteroid;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    bool active;
} Beam;

typedef struct {
    Vector2 position;
    float size;
    bool active;
    Texture2D texture;
} Station;

static Player player;
static Coin coins[MAX_COINS];
static Asteroid asteroids[MAX_ASTEROIDS];
static Camera2D camera = { 0 };
static bool gamePaused = false;
static int score = 0;
static bool gameOver = false;
static Texture2D jetEffect;
static Texture2D beamEffect;
static Sound engineSound;
static Sound engineBoostSound;
static Sound laserSound;
static Beam beams[MAX_BEAMS];
static float shootTimer = 0.0f;
static float engineVolume = 0.0f;
static bool engineSoundPlaying = false;
static Station stations[MAX_STATIONS];
static Texture2D stationTexture;
#define AUDIO_FADE_SPEED 0.1f
static const char* crashReason = NULL;

Vector2 GetRandomSpawnPosition(void) {
    Vector2 pos;
    float spawnDistance = WINDOW_WIDTH;  // Distance from camera view to spawn
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    
    pos.x = player.position.x + cosf(angle) * spawnDistance;
    pos.y = player.position.y + sinf(angle) * spawnDistance;
    
    return pos;
}

void ResetGame(void) {
    player.position = (Vector2){WINDOW_WIDTH/2, WINDOW_HEIGHT/2};
    player.rotation = PLAYER_BASE_ROTATION;
    score = 0;
    gameOver = false;

    // Reset coins
    for (int i = 0; i < MAX_COINS; i++) {
        coins[i].position = GetRandomSpawnPosition();
        coins[i].velocity = (Vector2){
            GetRandomValue(-COIN_SPEED, COIN_SPEED),
            GetRandomValue(-COIN_SPEED, COIN_SPEED)
        };
        coins[i].active = true;
    }

    // Reset beams
    for (int i = 0; i < MAX_BEAMS; i++) {
        beams[i].active = false;
    }

    // Reset asteroids with hits
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].position = GetRandomSpawnPosition();
        asteroids[i].velocity = (Vector2){
            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED),
            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED)
        };
        asteroids[i].size = GetRandomValue(20, 40);
        asteroids[i].active = true;
        asteroids[i].hits = 0;
    }

    // Reset stations
    for (int i = 0; i < MAX_STATIONS; i++) {
        do {
            stations[i].position = GetRandomSpawnPosition();
            // Check distance from other stations
            bool tooClose = false;
            for (int j = 0; j < i; j++) {
                if (Vector2Distance(stations[i].position, stations[j].position) < MIN_STATION_DISTANCE) {
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) break;
        } while (1);
        
        stations[i].size = GetRandomValue(STATION_MIN_SIZE, STATION_MAX_SIZE);
        stations[i].active = true;
    }

    player.health = PLAYER_MAX_HEALTH;
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space Collector");
    InitAudioDevice();
    InitAI();
    InitAIBeams();
    SetTargetFPS(60);

    // Load textures
    Texture2D backgroundTexture = LoadTexture("assets/background/background.jpg");
    Texture2D shipTexture = LoadTexture("assets/ship/ship.png");
    Texture2D goldTexture = LoadTexture("assets/gold/gold.png");
    Texture2D asteroidTexture = LoadTexture("assets/asteroids/asteroid-pixel-1.png");
    jetEffect = LoadTexture("assets/effects/jet.png");
    beamEffect = LoadTexture("assets/effects/beam.png");
    stationTexture = LoadTexture("assets/stations/station1.png");

    // Load sounds
    engineSound = LoadSound("assets/sounds/engine_idle.wav");
    engineBoostSound = LoadSound("assets/sounds/engine_boost.wav");
    laserSound = LoadSound("assets/sounds/laser_shoot.wav");

    // Set sound volume
    SetSoundVolume(engineSound, 0.5f);
    SetSoundVolume(engineBoostSound, 0.7f);
    SetSoundVolume(laserSound, 0.6f);

    // Initialize player
    player = (Player){
        .position = (Vector2){WINDOW_WIDTH/2, WINDOW_HEIGHT/2},
        .size = 30,
        .color = RED,
        .texture = shipTexture,
        .rotation = PLAYER_BASE_ROTATION,
        .health = PLAYER_MAX_HEALTH
    };

    // Initialize camera
    camera.offset = (Vector2){ WINDOW_WIDTH/2.0f, WINDOW_HEIGHT/2.0f };
    camera.target = player.position;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Initialize game objects
    for (int i = 0; i < MAX_COINS; i++) {
        coins[i].position = GetRandomSpawnPosition();
        coins[i].velocity = (Vector2){
            GetRandomValue(-COIN_SPEED, COIN_SPEED),
            GetRandomValue(-COIN_SPEED, COIN_SPEED)
        };
        coins[i].active = true;
        coins[i].texture = goldTexture;
    }

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].position = GetRandomSpawnPosition();
        asteroids[i].velocity = (Vector2){
            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED),
            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED)
        };
        asteroids[i].size = GetRandomValue(20, 40);
        asteroids[i].active = true;
        asteroids[i].texture = asteroidTexture;
    }

    for (int i = 0; i < MAX_STATIONS; i++) {
        do {
            stations[i].position = GetRandomSpawnPosition();
            bool tooClose = false;
            for (int j = 0; j < i; j++) {
                if (Vector2Distance(stations[i].position, stations[j].position) < MIN_STATION_DISTANCE) {
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) break;
        } while (1);
        
        stations[i].size = GetRandomValue(STATION_MIN_SIZE, STATION_MAX_SIZE);
        stations[i].active = true;
        stations[i].texture = stationTexture;
    }

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        shootTimer -= deltaTime;
        bool isMoving = false;  // Declare at start of loop

        if (gameOver && IsKeyPressed(KEY_ENTER)) {
            ResetGame();
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            gamePaused = !gamePaused;
        }

        if (!gameOver && !gamePaused) {
            // Update isMoving based on input
            isMoving = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) || 
                       IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN);

            // Smooth audio transitions
            if (isMoving) {
                engineVolume = fminf(engineVolume + AUDIO_FADE_SPEED, 1.0f);
                if (!engineSoundPlaying) {
                    PlaySound(engineSound);
                    engineSoundPlaying = true;
                }
            } else {
                engineVolume = fmaxf(engineVolume - AUDIO_FADE_SPEED, 0.0f);
                if (engineVolume <= 0.0f && engineSoundPlaying) {
                    StopSound(engineSound);
                    engineSoundPlaying = false;
                }
            }

            if (engineSoundPlaying) {
                SetSoundVolume(engineSound, engineVolume * 0.5f);
            }

            // Update camera target to follow player
            camera.target = player.position;

            // Update player movement and rotation
            if ((IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT)) && 
                (IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN))) {
                float moveSpeed = PLAYER_SPEED_DIAGONAL;
                
                if (IsKeyDown(KEY_RIGHT)) player.position.x += moveSpeed;
                if (IsKeyDown(KEY_LEFT)) player.position.x -= moveSpeed;
                if (IsKeyDown(KEY_UP)) player.position.y -= moveSpeed;
                if (IsKeyDown(KEY_DOWN)) player.position.y += moveSpeed;
            } else {
                if (IsKeyDown(KEY_RIGHT)) player.position.x += PLAYER_SPEED;
                if (IsKeyDown(KEY_LEFT)) player.position.x -= PLAYER_SPEED;
                if (IsKeyDown(KEY_UP)) player.position.y -= PLAYER_SPEED;
                if (IsKeyDown(KEY_DOWN)) player.position.y += PLAYER_SPEED;
            }

            // Update rotation based on movement
            if (IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT)) {
                if (IsKeyDown(KEY_UP)) player.rotation = PLAYER_BASE_ROTATION + 45;
                else if (IsKeyDown(KEY_DOWN)) player.rotation = PLAYER_BASE_ROTATION + 135;
                else player.rotation = PLAYER_BASE_ROTATION + 90;
            }
            else if (IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT)) {
                if (IsKeyDown(KEY_UP)) player.rotation = PLAYER_BASE_ROTATION - 45;
                else if (IsKeyDown(KEY_DOWN)) player.rotation = PLAYER_BASE_ROTATION - 135;
                else player.rotation = PLAYER_BASE_ROTATION - 90;
            }
            else if (IsKeyDown(KEY_UP)) player.rotation = PLAYER_BASE_ROTATION;
            else if (IsKeyDown(KEY_DOWN)) player.rotation = PLAYER_BASE_ROTATION + 180;

            // Shooting mechanics
            if (IsKeyDown(KEY_SPACE) && shootTimer <= 0) {
                for (int i = 0; i < MAX_BEAMS; i++) {
                    if (!beams[i].active) {
                        // Calculate beam starting position at front of ship
                        float offsetDistance = player.size * 0.75f;
                        beams[i].position.x = player.position.x + cosf((player.rotation - 90) * DEG2RAD) * offsetDistance;
                        beams[i].position.y = player.position.y + sinf((player.rotation - 90) * DEG2RAD) * offsetDistance;
                        
                        // Calculate beam velocity based on ship rotation
                        float angle = (player.rotation - 90) * DEG2RAD;
                        beams[i].velocity.x = cosf(angle) * BEAM_SPEED;
                        beams[i].velocity.y = sinf(angle) * BEAM_SPEED;
                        
                        beams[i].lifetime = BEAM_LIFETIME;
                        beams[i].active = true;
                        PlaySound(laserSound);
                        shootTimer = SHOOT_COOLDOWN;
                        break;
                    }
                }
            }

            // Update beams
            for (int i = 0; i < MAX_BEAMS; i++) {
                if (beams[i].active) {
                    beams[i].position = Vector2Add(beams[i].position, beams[i].velocity);
                    beams[i].lifetime -= deltaTime;

                    if (beams[i].lifetime <= 0) {
                        beams[i].active = false;
                        continue;
                    }

                    // Check collision with asteroids
                    for (int j = 0; j < MAX_ASTEROIDS; j++) {
                        if (asteroids[j].active && 
                            CheckCollisionCircles(beams[i].position, 4,
                                               asteroids[j].position, asteroids[j].size/2)) {
                            beams[i].active = false;
                            asteroids[j].hits++;
                            
                            if (asteroids[j].hits >= ASTEROID_HITS) {
                                asteroids[j].position = GetRandomSpawnPosition();
                                asteroids[j].velocity = (Vector2){
                                    GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED),
                                    GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED)
                                };
                                asteroids[j].hits = 0;
                                score += 5;  // Bonus points for destroying asteroid
                            }
                            break;
                        }
                    }
                }
            }

            // Update coins and asteroids only when not paused
            for (int i = 0; i < MAX_COINS; i++) {
                if (coins[i].active) {
                    coins[i].position = Vector2Add(coins[i].position, coins[i].velocity);
                    
                    float distToPlayer = Vector2Distance(coins[i].position, player.position);
                    if (distToPlayer > WINDOW_WIDTH * 1.5) {
                        coins[i].position = GetRandomSpawnPosition();
                        coins[i].velocity = (Vector2){
                            GetRandomValue(-COIN_SPEED, COIN_SPEED),
                            GetRandomValue(-COIN_SPEED, COIN_SPEED)
                        };
                    }

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
                    
                    float distToPlayer = Vector2Distance(asteroids[i].position, player.position);
                    if (distToPlayer > WINDOW_WIDTH * 1.5) {
                        asteroids[i].position = GetRandomSpawnPosition();
                        asteroids[i].velocity = (Vector2){
                            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED),
                            GetRandomValue(-ASTEROID_SPEED, ASTEROID_SPEED)
                        };
                    }

                    if (CheckCollisionCircles(player.position, player.size/2,
                        asteroids[i].position, asteroids[i].size/2)) {
                        player.health -= 25;  // Asteroid damage
                        if (player.health <= 0) {
                            gameOver = true;
                            crashReason = "You crashed into an asteroid!";
                        }
                    }
                }
            }

            // Add station collision check
            for (int i = 0; i < MAX_STATIONS; i++) {
                if (stations[i].active && 
                    CheckCollisionCircles(player.position, player.size/2,
                                        stations[i].position, stations[i].size/2)) {
                    gameOver = true;
                    crashReason = "You crashed into a space station, dummy!";
                }
            }

            // Update AI
            UpdateAI(player.position, deltaTime, &gameOver);

            // Add AI beam update
            UpdateAIBeams(player.position, deltaTime);

            // Add player-AI collision check
            for (int i = 0; i < MAX_AI_SHIPS; i++) {
                if (CheckCollisionCircles(player.position, player.size/2,
                    GetAIShipPosition(i), AI_SHIP_SIZE/2)) {
                    player.health -= AI_COLLISION_DAMAGE;
                    if (player.health <= 0) {
                        gameOver = true;
                        crashReason = "You crashed into an enemy ship!";
                    }
                }
            }

            // Add AI beam collision with player
            for (int i = 0; i < MAX_AI_BEAMS; i++) {
                if (IsAIBeamActive(i) && CheckCollisionCircles(
                    player.position, player.size/2,
                    GetAIBeamPosition(i), AI_BEAM_SIZE/2)) {
                    player.health -= AI_BEAM_DAMAGE;
                    if (player.health <= 0) {
                        gameOver = true;
                        crashReason = "Destroyed by enemy fire!";
                    }
                }
            }

            // Add player beam collision with AI ships (in the beam update loop)
            for (int i = 0; i < MAX_BEAMS; i++) {
                if (beams[i].active) {
                    for (int j = 0; j < MAX_AI_SHIPS; j++) {
                        if (CheckCollisionCircles(GetAIShipPosition(j), AI_SHIP_SIZE/2,
                            beams[i].position, BEAM_SIZE/2)) {
                            DamageAIShip(j, 20);  // Player beam damage
                            beams[i].active = false;
                            score += 2;  // Bonus points for hitting enemy ships
                            break;
                        }
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);

        if (!gameOver) {
            // Draw background with tiling
            float bgX = -((int)camera.target.x % backgroundTexture.width);
            float bgY = -((int)camera.target.y % backgroundTexture.height);
            
            for (int x = -1; x < 2; x++) {
                for (int y = -1; y < 2; y++) {
                    DrawTexture(backgroundTexture, 
                        bgX + x * backgroundTexture.width + camera.target.x - WINDOW_WIDTH/2, 
                        bgY + y * backgroundTexture.height + camera.target.y - WINDOW_HEIGHT/2, 
                        WHITE);
                }
            }

            // Draw player with jet effect
            if (isMoving) {
                // Calculate jet position based on player's direction
                Vector2 jetPos = player.position;
                float jetRotation = player.rotation;
                
                // Move jet effect to back of ship based on rotation (180 degrees opposite)
                float offsetDistance = player.size * 0.75f;
                jetPos.x += cosf((jetRotation + 90) * DEG2RAD) * offsetDistance;
                jetPos.y += sinf((jetRotation + 90) * DEG2RAD) * offsetDistance;

                // Draw jet with fade based on engine volume
                Rectangle jetSource = (Rectangle){ 0, 0, jetEffect.width, jetEffect.height };
                Rectangle jetDest = (Rectangle){ 
                    jetPos.x - player.size/2, 
                    jetPos.y - player.size/2,
                    player.size, 
                    player.size 
                };
                Color jetColor = (Color){ 255, 255, 255, (unsigned char)(engineVolume * 255) };
                DrawTexturePro(jetEffect, jetSource, jetDest, 
                    (Vector2){ player.size/2, player.size/2 }, 
                    jetRotation, jetColor);
            }

            // Draw player
            Rectangle playerSource = (Rectangle){ 0, 0, player.texture.width, player.texture.height };
            Rectangle playerDest = (Rectangle){ 
                player.position.x - player.size/2, 
                player.position.y - player.size/2,
                player.size, 
                player.size 
            };
            DrawTexturePro(player.texture, playerSource, playerDest, 
                (Vector2){ player.size/2, player.size/2 }, 
                player.rotation, WHITE);

            // Draw coins
            for (int i = 0; i < MAX_COINS; i++) {
                if (coins[i].active) {
                    Rectangle coinSource = (Rectangle){ 0, 0, goldTexture.width, goldTexture.height };
                    Rectangle coinDest = (Rectangle){ 
                        coins[i].position.x - COIN_SIZE/2, 
                        coins[i].position.y - COIN_SIZE/2,
                        COIN_SIZE, 
                        COIN_SIZE 
                    };
                    DrawTexturePro(coins[i].texture, coinSource, coinDest, (Vector2){0, 0}, 0, WHITE);
                }
            }

            // Draw asteroids
            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (asteroids[i].active) {
                    Rectangle sourceRec = (Rectangle){ 0, 0, asteroidTexture.width, asteroidTexture.height };
                    Rectangle destRec = (Rectangle){ 
                        asteroids[i].position.x - asteroids[i].size/2, 
                        asteroids[i].position.y - asteroids[i].size/2,
                        asteroids[i].size, 
                        asteroids[i].size 
                    };
                    DrawTexturePro(asteroids[i].texture, sourceRec, destRec, (Vector2){0, 0}, 0, WHITE);
                }
            }

            // Draw beams with correct direction
            for (int i = 0; i < MAX_BEAMS; i++) {
                if (beams[i].active) {
                    Rectangle beamSource = (Rectangle){ 0, 0, beamEffect.width, beamEffect.height };
                    Rectangle beamDest = (Rectangle){ 
                        beams[i].position.x - 8, 
                        beams[i].position.y - 4,
                        16, 
                        8 
                    };
                    // Calculate beam rotation based on velocity
                    float beamRotation = atan2f(beams[i].velocity.y, beams[i].velocity.x) * RAD2DEG;
                    DrawTexturePro(beamEffect, beamSource, beamDest, 
                        (Vector2){ 8, 4 }, beamRotation, WHITE);
                }
            }

            // Draw stations
            for (int i = 0; i < MAX_STATIONS; i++) {
                if (stations[i].active) {
                    Rectangle sourceRec = (Rectangle){ 0, 0, stationTexture.width, stationTexture.height };
                    Rectangle destRec = (Rectangle){ 
                        stations[i].position.x - stations[i].size/2, 
                        stations[i].position.y - stations[i].size/2,
                        stations[i].size, 
                        stations[i].size 
                    };
                    DrawTexturePro(stations[i].texture, sourceRec, destRec, (Vector2){0, 0}, 0, WHITE);
                }
            }

            // Draw AI ships
            DrawAIShips();

            // Draw AI beams
            DrawAIBeams();
        }

        EndMode2D();

        // Draw HUD elements
        if (!gameOver) {
            // Health bar
            DrawRectangle(
                WINDOW_WIDTH - HEALTH_BAR_WIDTH - HUD_MARGIN,
                HUD_MARGIN,
                HEALTH_BAR_WIDTH,
                HEALTH_BAR_HEIGHT,
                RED
            );
            DrawRectangle(
                WINDOW_WIDTH - HEALTH_BAR_WIDTH - HUD_MARGIN,
                HUD_MARGIN,
                (HEALTH_BAR_WIDTH * player.health) / PLAYER_MAX_HEALTH,
                HEALTH_BAR_HEIGHT,
                GREEN
            );

            // Score HUD
            DrawRectangle(HUD_MARGIN, HUD_MARGIN, 100, 25, Fade(BLACK, 0.5f));
            DrawText(TextFormat("SCORE: %d", score), 
                HUD_MARGIN + 5, 
                HUD_MARGIN + 5, 
                HUD_TEXT_SIZE, 
                WHITE);
        } else {
            // Game Over HUD
            int hudWidth = 300;
            int hudHeight = 150;
            int hudX = (WINDOW_WIDTH - hudWidth) / 2;
            int hudY = (WINDOW_HEIGHT - hudHeight) / 2;

            // Draw semi-transparent HUD background
            DrawRectangle(hudX, hudY, hudWidth, hudHeight, Fade(BLACK, 0.8f));

            // Draw game over text
            const char* gameOverText = "GAME OVER";
            const char* crashMessage = crashReason ? crashReason : "You crashed!";
            const char* scoreText = TextFormat("Final Score: %d", score);
            const char* restartText = "Press ENTER to restart";

            int gameOverWidth = MeasureText(gameOverText, 40);
            int crashWidth = MeasureText(crashMessage, 20);
            int scoreWidth = MeasureText(scoreText, 30);
            int restartWidth = MeasureText(restartText, 20);

            DrawText(gameOverText, 
                hudX + (hudWidth - gameOverWidth) / 2, 
                hudY + 20, 
                40, RED);
            DrawText(crashMessage,
                hudX + (hudWidth - crashWidth) / 2,
                hudY + 60,
                20, WHITE);
            DrawText(scoreText, 
                hudX + (hudWidth - scoreWidth) / 2, 
                hudY + 90, 
                30, WHITE);
            DrawText(restartText, 
                hudX + (hudWidth - restartWidth) / 2, 
                hudY + 120, 
                20, GRAY);
        }

        // Draw pause menu
        if (gamePaused) {
            DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Fade(BLACK, 0.7f));
            const char* pausedText = "PAUSED";
            const char* continueText = "Press ESC to continue";
            
            int pausedWidth = MeasureText(pausedText, 40);
            int continueWidth = MeasureText(continueText, 20);
            
            DrawText(pausedText, 
                (WINDOW_WIDTH - pausedWidth) / 2, 
                WINDOW_HEIGHT / 2 - 30, 
                40, WHITE);
            DrawText(continueText, 
                (WINDOW_WIDTH - continueWidth) / 2, 
                WINDOW_HEIGHT / 2 + 20, 
                20, GRAY);
        }

        #ifdef _DEBUG
            DrawFPS(WINDOW_WIDTH - 80, WINDOW_HEIGHT - 20);
        #endif

        EndDrawing();
    }

    // Unload resources
    UnloadTexture(backgroundTexture);
    UnloadTexture(shipTexture);
    UnloadTexture(goldTexture);
    UnloadTexture(asteroidTexture);
    UnloadTexture(jetEffect);
    UnloadTexture(beamEffect);
    UnloadTexture(stationTexture);
    UnloadSound(engineSound);
    UnloadSound(engineBoostSound);
    UnloadSound(laserSound);
    CloseAudioDevice();
    CloseWindow();
    UnloadAI();
    return 0;
} 