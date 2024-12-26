#include "game.h"
#include "raymath.h"
#include "rlgl.h"

static Camera3D camera = { 0 };
static Vector3 playerPosition = { 0 };
static float playerSpeed = 0.2f;
static float mouseSensitivity = 0.003f;
static float playerHeight = 2.0f;
static Texture2D skybox;

// Collision boxes for obstacles
static BoundingBox obstacles[] = {
    // Center red cube
    {
        (Vector3){-1, 0, -1},
        (Vector3){1, 2, 1}
    },
    // Blue cube
    {
        (Vector3){4, 0, 4},
        (Vector3){6, 2, 6}
    },
    // Green cube
    {
        (Vector3){-6, 0, -6},
        (Vector3){-4, 2, -4}
    }
};
static const int numObstacles = sizeof(obstacles)/sizeof(obstacles[0]);

// Check if a point collides with any obstacle
bool CheckCollisionWithObstacles(Vector3 point) {
    BoundingBox playerBox = {
        (Vector3){ point.x - 0.2f, point.y - 0.2f, point.z - 0.2f },
        (Vector3){ point.x + 0.2f, point.y + 0.2f, point.z + 0.2f }
    };
    
    for (int i = 0; i < numObstacles; i++) {
        if (CheckCollisionBoxes(playerBox, obstacles[i])) {
            return true;
        }
    }
    return false;
}

void InitGame(void) {
    // Initialize camera
    camera.position = (Vector3){ 0.0f, playerHeight, 4.0f };
    camera.target = (Vector3){ 0.0f, playerHeight, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Load skybox texture
    Image img = LoadImage("assets/skybox/skybox-day.jpg");
    skybox = LoadTextureFromImage(img);
    UnloadImage(img);

    DisableCursor();
}

void UpdateGame(void) {
    // Get the camera's forward and right vectors
    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    forward.y = 0; // Keep movement horizontal
    forward = Vector3Normalize(forward);
    
    Vector3 right = Vector3CrossProduct(forward, camera.up);
    right = Vector3Normalize(right);

    // Movement
    Vector3 moveVec = {0};
    if (IsKeyDown(KEY_W)) moveVec = Vector3Add(moveVec, forward);
    if (IsKeyDown(KEY_S)) moveVec = Vector3Subtract(moveVec, forward);
    if (IsKeyDown(KEY_D)) moveVec = Vector3Add(moveVec, right);
    if (IsKeyDown(KEY_A)) moveVec = Vector3Subtract(moveVec, right);

    // Normalize and apply movement with collision check
    if (!Vector3Equals(moveVec, (Vector3){0,0,0})) {
        moveVec = Vector3Scale(Vector3Normalize(moveVec), playerSpeed);
        Vector3 newPosition = Vector3Add(camera.position, moveVec);
        
        // Only update position if there's no collision
        if (!CheckCollisionWithObstacles(newPosition)) {
            camera.position = newPosition;
        }
    }

    // Mouse look
    Vector2 mouseMovement = GetMouseDelta();
    
    // Horizontal rotation
    float angleH = -mouseMovement.x * mouseSensitivity;
    Vector3 forward2 = Vector3Subtract(camera.target, camera.position);
    forward2 = Vector3RotateByAxisAngle(forward2, camera.up, angleH);
    camera.target = Vector3Add(camera.position, forward2);

    // Vertical rotation (with limits)
    float angleV = -mouseMovement.y * mouseSensitivity;
    Vector3 right2 = Vector3CrossProduct(forward2, camera.up);
    forward2 = Vector3RotateByAxisAngle(forward2, right2, angleV);
    
    // Limit vertical look angle
    Vector3 up = {0.0f, 1.0f, 0.0f};
    float angle = Vector3Angle(forward2, up);
    if (angle > 0.1f && angle < PI - 0.1f) {
        camera.target = Vector3Add(camera.position, forward2);
    }

    playerPosition = camera.position;
}

void DrawGame(void) {
    BeginMode3D(camera);
        // Draw skybox (centered on camera)
        float size = 900.0f;
        rlDisableBackfaceCulling();
        rlDisableDepthMask();
        DrawCube((Vector3){camera.position.x, camera.position.y, camera.position.z}, 
                size, size, size, Fade(SKYBLUE, 0.5f));
        rlEnableBackfaceCulling();
        rlEnableDepthMask();
        
        // Draw floor
        DrawPlane((Vector3){0, 0, 0}, (Vector2){50, 50}, BLACK);
        
        // Draw obstacles
        DrawCube((Vector3){0, 1, 0}, 2, 2, 2, RED);
        DrawCube((Vector3){5, 1, 5}, 2, 2, 2, BLUE);
        DrawCube((Vector3){-5, 1, -5}, 2, 2, 2, GREEN);

        // Debug: visualize collision boxes
        for (int i = 0; i < numObstacles; i++) {
            DrawBoundingBox(obstacles[i], YELLOW);
        }
    EndMode3D();
}

void UnloadGame(void) {
    UnloadTexture(skybox);
    EnableCursor();
} 