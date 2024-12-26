#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <math.h>
#include "game_defs.h"

typedef enum {
    AI_PATROL,
    AI_CHASE,
    AI_ATTACK,
    AI_RETREAT
} AIState;

typedef struct {
    Vector2 position;
    Vector2 patrolCenter;
    float rotation;
    float health;
    bool active;
    AIState state;
    float shootTimer;
    Texture2D texture;
    Color color;
} AIShip;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    bool active;
} AIBeam;

static AIShip aiShips[MAX_AI_SHIPS];
static Texture2D aiShipTexture;
static AIBeam aiBeams[MAX_AI_BEAMS];

void InitAI(void) {
    aiShipTexture = LoadTexture("assets/ship/enemy.png");
    
    for (int i = 0; i < MAX_AI_SHIPS; i++) {
        aiShips[i].position = (Vector2){
            GetRandomValue(-WORLD_SIZE/2, WORLD_SIZE/2),
            GetRandomValue(-WORLD_SIZE/2, WORLD_SIZE/2)
        };
        aiShips[i].patrolCenter = aiShips[i].position;
        aiShips[i].rotation = GetRandomValue(0, 360);
        aiShips[i].health = AI_MAX_HEALTH;
        aiShips[i].active = true;
        aiShips[i].state = AI_PATROL;
        aiShips[i].shootTimer = 0;
        aiShips[i].texture = aiShipTexture;
        aiShips[i].color = RED;
    }
}

void InitAIBeams(void) {
    for (int i = 0; i < MAX_AI_BEAMS; i++) {
        aiBeams[i].active = false;
    }
}

void ShootAIBeam(int shipIndex) {
    for (int i = 0; i < MAX_AI_BEAMS; i++) {
        if (!aiBeams[i].active) {
            Vector2 shipPos = aiShips[shipIndex].position;
            float rotation = aiShips[shipIndex].rotation;
            
            // Calculate beam starting position at front of ship
            float offsetDistance = AI_SHIP_SIZE * 0.75f;
            aiBeams[i].position.x = shipPos.x + cosf((rotation - 90) * DEG2RAD) * offsetDistance;
            aiBeams[i].position.y = shipPos.y + sinf((rotation - 90) * DEG2RAD) * offsetDistance;
            
            // Calculate beam velocity based on ship rotation
            float angle = (rotation - 90) * DEG2RAD;
            aiBeams[i].velocity.x = cosf(angle) * AI_BEAM_SPEED;
            aiBeams[i].velocity.y = sinf(angle) * AI_BEAM_SPEED;
            
            aiBeams[i].lifetime = AI_BEAM_LIFETIME;
            aiBeams[i].active = true;
            break;
        }
    }
}

void UpdateAI(Vector2 playerPos, float deltaTime, bool* gameOver) {
    for (int i = 0; i < MAX_AI_SHIPS; i++) {
        if (!aiShips[i].active) continue;

        aiShips[i].shootTimer -= deltaTime;
        float distToPlayer = Vector2Distance(aiShips[i].position, playerPos);

        // Add shooting logic in AI_ATTACK state
        if (aiShips[i].state == AI_ATTACK && aiShips[i].shootTimer <= 0) {
            ShootAIBeam(i);
            aiShips[i].shootTimer = AI_SHOOT_COOLDOWN;
        }

        // State machine
        switch(aiShips[i].state) {
            case AI_PATROL:
                if (distToPlayer < AI_CHASE_RANGE) {
                    aiShips[i].state = AI_CHASE;
                }
                // Patrol in a circle around patrol center
                {
                    float patrolAngle = aiShips[i].rotation * DEG2RAD;
                    Vector2 targetPos = {
                        aiShips[i].patrolCenter.x + cosf(patrolAngle) * AI_PATROL_RADIUS,
                        aiShips[i].patrolCenter.y + sinf(patrolAngle) * AI_PATROL_RADIUS
                    };
                    aiShips[i].rotation += AI_ROTATION_SPEED;
                    aiShips[i].position = Vector2MoveTowards(
                        aiShips[i].position, 
                        targetPos, 
                        AI_SPEED
                    );
                }
                break;

            case AI_CHASE:
                if (distToPlayer > AI_CHASE_RANGE * 1.2f) {
                    aiShips[i].state = AI_PATROL;
                } else if (distToPlayer < AI_ATTACK_RANGE) {
                    aiShips[i].state = AI_ATTACK;
                }
                // Move towards player
                aiShips[i].position = Vector2MoveTowards(
                    aiShips[i].position,
                    playerPos,
                    AI_SPEED
                );
                break;

            case AI_ATTACK:
                if (distToPlayer > AI_ATTACK_RANGE) {
                    aiShips[i].state = AI_CHASE;
                } else if (aiShips[i].health < AI_MAX_HEALTH * 0.3f) {
                    aiShips[i].state = AI_RETREAT;
                }
                // Attack logic (shooting) is handled in the main game loop
                break;

            case AI_RETREAT:
                if (aiShips[i].health > AI_MAX_HEALTH * 0.5f) {
                    aiShips[i].state = AI_PATROL;
                }
                // Move away from player
                {
                    Vector2 retreatDir = Vector2Subtract(aiShips[i].position, playerPos);
                    retreatDir = Vector2Normalize(retreatDir);
                    aiShips[i].position = Vector2Add(
                        aiShips[i].position,
                        Vector2Scale(retreatDir, AI_SPEED)
                    );
                }
                break;
        }

        // Update rotation to face movement direction or player
        if (aiShips[i].state != AI_PATROL) {
            Vector2 direction = Vector2Subtract(playerPos, aiShips[i].position);
            aiShips[i].rotation = atan2f(direction.y, direction.x) * RAD2DEG + 90;
        }
    }
}

void UpdateAIBeams(Vector2 playerPos, float deltaTime) {
    for (int i = 0; i < MAX_AI_BEAMS; i++) {
        if (aiBeams[i].active) {
            aiBeams[i].position = Vector2Add(aiBeams[i].position, aiBeams[i].velocity);
            aiBeams[i].lifetime -= deltaTime;

            if (aiBeams[i].lifetime <= 0) {
                aiBeams[i].active = false;
                continue;
            }

            // Check collision with player
            if (CheckCollisionCircles(playerPos, PLAYER_SIZE/2,
                aiBeams[i].position, AI_BEAM_SIZE/2)) {
                aiBeams[i].active = false;
                // Player damage is handled in main.c
            }
        }
    }
}

void DrawAIShips(void) {
    for (int i = 0; i < MAX_AI_SHIPS; i++) {
        if (!aiShips[i].active) continue;

        // Draw health bar
        Vector2 healthBarPos = {
            aiShips[i].position.x - AI_SHIP_SIZE/2,
            aiShips[i].position.y - AI_SHIP_SIZE
        };
        float healthPercentage = aiShips[i].health / AI_MAX_HEALTH;
        DrawRectangle(
            healthBarPos.x,
            healthBarPos.y,
            AI_SHIP_SIZE,
            5,
            RED
        );
        DrawRectangle(
            healthBarPos.x,
            healthBarPos.y,
            AI_SHIP_SIZE * healthPercentage,
            5,
            GREEN
        );

        // Draw ship
        Rectangle sourceRec = {0, 0, aiShipTexture.width, aiShipTexture.height};
        Rectangle destRec = {
            aiShips[i].position.x - AI_SHIP_SIZE/2,
            aiShips[i].position.y - AI_SHIP_SIZE/2,
            AI_SHIP_SIZE,
            AI_SHIP_SIZE
        };
        DrawTexturePro(
            aiShips[i].texture,
            sourceRec,
            destRec,
            (Vector2){AI_SHIP_SIZE/2, AI_SHIP_SIZE/2},
            aiShips[i].rotation,
            aiShips[i].color
        );
    }
}

void DrawAIBeams(void) {
    for (int i = 0; i < MAX_AI_BEAMS; i++) {
        if (aiBeams[i].active) {
            DrawCircle(aiBeams[i].position.x, aiBeams[i].position.y, AI_BEAM_SIZE/2, RED);
        }
    }
}

void DamageAIShip(int index, float damage) {
    if (!aiShips[index].active) return;
    
    aiShips[index].health -= damage;
    if (aiShips[index].health <= 0) {
        aiShips[index].active = false;
    } else if (aiShips[index].health < AI_MAX_HEALTH * 0.3f) {
        aiShips[index].state = AI_RETREAT;
    }
}

bool CanAIShipShoot(int index) {
    return aiShips[index].active && 
           aiShips[index].state == AI_ATTACK && 
           aiShips[index].shootTimer <= 0;
}

void ResetAIShootTimer(int index) {
    aiShips[index].shootTimer = AI_SHOOT_COOLDOWN;
}

Vector2 GetAIShipPosition(int index) {
    return aiShips[index].position;
}

float GetAIShipRotation(int index) {
    return aiShips[index].rotation;
}

void UnloadAI(void) {
    UnloadTexture(aiShipTexture);
}

bool IsAIBeamActive(int index) {
    if (index >= 0 && index < MAX_AI_BEAMS) {
        return aiBeams[index].active;
    }
    return false;
}

Vector2 GetAIBeamPosition(int index) {
    if (index >= 0 && index < MAX_AI_BEAMS) {
        return aiBeams[index].position;
    }
    return (Vector2){0, 0};
} 