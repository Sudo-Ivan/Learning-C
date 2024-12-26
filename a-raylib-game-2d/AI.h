#ifndef AI_H
#define AI_H

#include "raylib.h"

// Function declarations
void InitAI(void);
void UpdateAI(Vector2 playerPos, float deltaTime, bool* gameOver);
void DrawAIShips(void);
void DamageAIShip(int index, float damage);
bool CanAIShipShoot(int index);
void ResetAIShootTimer(int index);
Vector2 GetAIShipPosition(int index);
float GetAIShipRotation(int index);
void UnloadAI(void);
void InitAIBeams(void);
void UpdateAIBeams(Vector2 playerPos, float deltaTime);
void DrawAIBeams(void);
void ShootAIBeam(int shipIndex);
bool IsAIBeamActive(int index);
Vector2 GetAIBeamPosition(int index);

#endif // AI_H 