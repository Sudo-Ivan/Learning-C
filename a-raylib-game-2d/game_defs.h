#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#define WORLD_SIZE 2000
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 450

// Player definitions
#define BEAM_SIZE 4
#define PLAYER_SIZE 30

// AI ship definitions
#define MAX_AI_SHIPS 3
#define AI_SHIP_SIZE 30
#define AI_PATROL_RADIUS 300
#define AI_CHASE_RANGE 400
#define AI_ATTACK_RANGE 200
#define AI_SPEED 3
#define AI_ROTATION_SPEED 3
#define AI_SHOOT_COOLDOWN 1.0f
#define AI_MAX_HEALTH 100

// AI beam definitions
#define MAX_AI_BEAMS 20
#define AI_BEAM_SPEED 8.0f
#define AI_BEAM_LIFETIME 1.0f
#define AI_BEAM_SIZE 4

// Damage definitions
#define AI_BEAM_DAMAGE 10
#define AI_COLLISION_DAMAGE 25

#endif // GAME_DEFS_H 