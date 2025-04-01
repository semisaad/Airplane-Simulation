#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

// Define RayGUI implementation before including its header
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#ifndef MAP_DIFFUSE
    #define MAP_DIFFUSE MATERIAL_MAP_ALBEDO
#endif

int main(void) 
{
    // Initialization
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    InitWindow(screenWidth, screenHeight, "Airplane Simulation");

    // ------------------------------
    // Load Terrain
    // ------------------------------
    Image heightmap = LoadImage("Great Lakes/Height-Map.png");
    if (heightmap.data == NULL)
    {
        printf("Error: Failed to load heightmap\n");
        CloseWindow();
        return 1;
    }
    int newWidth = heightmap.width / 3.0;
    int newHeight = heightmap.height / 3.0;
    ImageResize(&heightmap, newWidth, newHeight);

    Mesh terrainMesh = GenMeshHeightmap(heightmap, (Vector3){1000, 350, 1000});
    Model terrain = LoadModelFromMesh(terrainMesh);
    UnloadImage(heightmap);

    Texture2D terrainTexture = LoadTexture("Great Lakes/Diffuse-Map.png");
    if (terrainTexture.id == 0)
    {
        printf("Error: Failed to load terrain texture\n");
        UnloadModel(terrain);
        CloseWindow();
        return 1;
    }
    terrain.materials[0].maps[MAP_DIFFUSE].texture = terrainTexture;

    // ------------------------------
    // Load Terrain Saturation Shader (Optional)
    // ------------------------------
    Shader saturationShader = LoadShader(0, "Assets/saturation.fs");
    float saturationValue = 3.0f;  // Increase as desired
    int saturationLoc = GetShaderLocation(saturationShader, "saturation");
    SetShaderValue(saturationShader, saturationLoc, &saturationValue, SHADER_UNIFORM_FLOAT);
    terrain.materials[0].shader = saturationShader;

    // ------------------------------
    // Load Plane Model
    // ------------------------------
    Model plane = LoadModel("Assets/plane.obj");
    Texture2D planeTexture = LoadTexture("Assets/An2_aeroflot.png");
    plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = planeTexture;

    // ------------------------------
    // Load Runway Texture and Create Runway Model
    // ------------------------------
    Texture2D runwayTexture = LoadTexture("Assets/runway_texture.png");
    if (runwayTexture.id == 0)
    {
        printf("Error: Failed to load runway texture\n");
        CloseWindow();
        return 1;
    }
    float runwayWidth = 6.0f;
    float runwayLength = 22.0f;
    Mesh runwayMesh = GenMeshPlane(runwayWidth, runwayLength, 1, 1);
    Model runway = LoadModelFromMesh(runwayMesh);
    runway.materials[0].maps[MAP_DIFFUSE].texture = runwayTexture;

    // ------------------------------
    // Initial plane transform
    // ------------------------------
    Matrix scaleMatrix = MatrixScale(0.005f, 0.005f, 0.005f);
    Matrix correction = MatrixRotateY(PI/2);  // Rotation fix for some models

    // Spawn position for plane and runway
    const float spawnX = -1000.0f, spawnY = 5500.0f, spawnZ = 19000.0f;
    float transX = spawnX, transY = spawnY, transZ = spawnZ;
    float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;

    // ------------------------------
    // Setup Camera
    // ------------------------------
    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 60.0f, 120.0f };
    camera.target   = (Vector3){ 0.0f, 10.0f, 0.0f };
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy     = 8.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    UpdateCamera(&camera, CAMERA_FREE);

    //-------------------------------
    // Setup Music
    //-------------------------------
    InitAudioDevice();
    Music music_engine = LoadMusicStream("Assets/airplane-sound.mp3");
    PlayMusicStream(music_engine);

    // ------------------------------
    // First Person and Free Camera Toggles
    // ------------------------------
    bool firstPerson = false;
    bool freeCamera = false;

    // Define the landing spot (the runway's position) and red spot collision trigger
    Vector3 landingSpot = {100.0f, 27.458f, 1.0f};
    const float collisionThreshold = 5.0f;

    // Game Over flag
    bool gameOver = false;

    // --- Menu State ---
    bool inMenu = true;

    SetTargetFPS(60);
    
    // Main loop
    while (!WindowShouldClose()) 
    {
        // --- Starting Menu with RayGUI ---
        if (inMenu)
        {
            BeginDrawing();
                ClearBackground(RAYWHITE);
                DrawText("Airplane Simulation", screenWidth/2 - MeasureText("Airplane Simulation", 40)/2, screenHeight/2 - 120, 40, DARKBLUE);
                if (GuiButton((Rectangle){ screenWidth/2 - 100, screenHeight/2 - 40, 200, 50 }, "Start"))
                {
                    inMenu = false;
                    // Optionally, reset simulation variables in case of re-entry
                    transX = spawnX;
                    transY = spawnY;
                    transZ = spawnZ;
                    pitch = roll = yaw = 0.0f;
                    gameOver = false;
                }
                if (GuiButton((Rectangle){ screenWidth/2 - 100, screenHeight/2 + 20, 200, 50 }, "Exit"))
                {
                    CloseWindow();
                    return 0;
                }
            EndDrawing();
            continue;   // Skip simulation update until menu is dismissed
        }

        // Update music stream
        UpdateMusicStream(music_engine);

        // Toggle first person view with F and free camera with R (if not in game over state)
        if (!gameOver)
        {
            if (IsKeyPressed(KEY_F)) firstPerson = !firstPerson;
            if (IsKeyPressed(KEY_R)) freeCamera = !freeCamera;
        }

        // ------------------------------
        // Update Plane Movement with Gravity & Ground Effect (only if not game over)
        // ------------------------------
        if (!gameOver)
        {
            float groundAltitude = 6000.0f;      // Altitude threshold for ground effect
            float constantGravity = 0.5f;        // Constant downward pull when near ground
            float groundSpeedFactor = 0.5f;      // Slowdown factor for horizontal movement near ground

            Vector3 forward = { sinf(DEG2RAD * yaw), 0.0f, cosf(DEG2RAD * yaw) };
            float speedFactor = 1.0f;

            if (transY <= groundAltitude)
            {
                transY -= constantGravity;
                speedFactor = groundSpeedFactor;
            }

            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                transX += forward.x * 40.5f * speedFactor;
                transZ += forward.z * 40.5f * speedFactor;
                transY += sinf(DEG2RAD * pitch) * 20.5f * speedFactor;
            }
            if (IsKeyDown(KEY_SPACE)) {
                transX += forward.x * 50.0f * speedFactor;
                transZ += forward.z * 50.0f * speedFactor;
            }
            if (IsKeyDown(KEY_W)) transY += 12.0f * speedFactor;
            else if (IsKeyDown(KEY_S)) transY -= 12.0f * speedFactor;

            if (IsKeyDown(KEY_DOWN)) pitch += 0.2f;
            else if (IsKeyDown(KEY_UP)) pitch -= 0.2f;
            else {
                if (pitch > 0.2f) pitch -= 0.2f;
                else if (pitch < -0.2f) pitch += 0.2f;
            }
            if (IsKeyDown(KEY_D)) yaw -= 0.4f;
            if (IsKeyDown(KEY_A)) yaw += 0.4f;
            if (IsKeyDown(KEY_LEFT)) roll -= 0.7f;
            else if (IsKeyDown(KEY_RIGHT)) roll += 0.7f;
            else {
                if (roll > 0.0f) roll -= 0.3f;
                else if (roll < 0.0f) roll += 0.3f;
            }
            
            transX = Clamp(transX, -189900.0f, 9900.0f);
            transZ = Clamp(transZ, -9900.0f, 189900.0f);
            transY = Clamp(transY, 5500.0f, 200000.0f);

            Matrix rotationMat = MatrixRotateXYZ((Vector3){ DEG2RAD * pitch, DEG2RAD * yaw, DEG2RAD * roll });
            Matrix translationMat = MatrixTranslate(transX, transY, transZ);
            plane.transform = MatrixMultiply(rotationMat, MatrixMultiply(translationMat, MatrixMultiply(correction, scaleMatrix)));

            // Check for collision with the red landing spot
            Vector3 planePos = { plane.transform.m12, plane.transform.m13, plane.transform.m14 };
            if (Vector3Distance(planePos, landingSpot) < collisionThreshold)
            {
                gameOver = true;
            }
        }
        
        // ------------------------------
        // Update Camera
        // ------------------------------
        if (freeCamera)
        {
            UpdateCamera(&camera, CAMERA_FREE);
        }
        else
        {
            Vector3 planePos = (Vector3){ plane.transform.m12, plane.transform.m13, plane.transform.m14 };
            if (firstPerson)
            {
                camera.position = Vector3Add(planePos, (Vector3){0.0f, 0.5f, 0.0f});
                float pitchRad = DEG2RAD * pitch;
                float yawRad   = DEG2RAD * yaw;
                float rollRad  = DEG2RAD * roll;
                Matrix planeRotation = MatrixRotateXYZ((Vector3){ pitchRad, yawRad, rollRad });
                Matrix correctionMatrix = MatrixRotateY(PI/2);
                Matrix rot = MatrixMultiply(planeRotation, correctionMatrix);
                Vector3 forwardVec = Vector3Transform((Vector3){ 0.0f, 0.0f, 1.0f }, rot);
                Vector3 upVec = Vector3Transform((Vector3){ 0.0f, 1.0f, 0.0f }, rot);
                camera.target = Vector3Add(camera.position, forwardVec);
                camera.up = upVec;
            }
            else
            {
                Matrix yawPitchRotation = MatrixRotateXYZ((Vector3){ DEG2RAD * pitch, DEG2RAD * yaw, 0.0f });
                Vector3 cameraOffset = (Vector3){ -15.0f, 2.5f, 0.0f };
                cameraOffset = Vector3Transform(cameraOffset, yawPitchRotation);
                Vector3 desiredCamPos = Vector3Add((Vector3){ plane.transform.m12, plane.transform.m13, plane.transform.m14 }, cameraOffset);
                camera.position = Vector3Lerp(camera.position, desiredCamPos, 0.1f);
                camera.target = (Vector3){ plane.transform.m12, plane.transform.m13, plane.transform.m14 };
            }
        }

        // ------------------------------
        // Drawing
        // ------------------------------
        BeginDrawing();
            ClearBackground(SKYBLUE);
            
            BeginMode3D(camera);
                DrawModel(terrain, (Vector3){ -50, 0, -50 }, 1.0f, WHITE);
                DrawGrid(500, 1.0f);
                DrawModel(runway, (Vector3){ 95.0f, 27.458f, 15.0f }, 1.0f, WHITE);
                DrawModel(plane, (Vector3){ 0, 0, 0 }, 1.0f, WHITE);

                // Draw red landing spot (as a sphere) at the landing spot position
                DrawSphere(landingSpot, 1.0f, RED);
            EndMode3D();
            
            DrawFPS(10, 10);
            char positionText[50];
            snprintf(positionText, 50, "X: %.2f, Y: %.2f, Z: %.2f", transX, transY, transZ);
            DrawText(positionText, 10, 30, 20, WHITE);

            // --------------------------
            // Radar HUD
            // --------------------------
            Vector3 diff = Vector3Subtract(landingSpot, (Vector3){ plane.transform.m12, plane.transform.m13, plane.transform.m14 });
            float radarScale = 0.05f;
            Vector2 radarOffset = { diff.x * radarScale, diff.z * radarScale };

            Vector2 radarCenter = { 150, screenHeight - 150 };
            float radarRadius = 70.0f;

            DrawCircleV(radarCenter, radarRadius, DARKGRAY);
            DrawCircleLines(radarCenter.x, radarCenter.y, radarRadius, BLACK);
            DrawLine(radarCenter.x - radarRadius, radarCenter.y, radarCenter.x + radarRadius, radarCenter.y, GREEN);
            DrawLine(radarCenter.x, radarCenter.y - radarRadius, radarCenter.x, radarCenter.y + radarRadius, GREEN);
            Vector2 landingMarker = { radarCenter.x + radarOffset.x, radarCenter.y + radarOffset.y };
            DrawCircleV(landingMarker, 5.0, RED);

            // --------------------------
            // Game Over Overlay
            // --------------------------
            if (gameOver)
            {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
                DrawText("LANDED!", screenWidth/2 - MeasureText("LANDED!", 40)/2, screenHeight/2 - 50, 40, WHITE);
                DrawText("Press Y to Play Again or N to Exit", screenWidth/2 - MeasureText("Press Y to Play Again or N to Exit", 20)/2, screenHeight/2 + 10, 20, WHITE);
                
                if (IsKeyPressed(KEY_Y))
                {
                    transX = spawnX;
                    transY = spawnY;
                    transZ = spawnZ;
                    pitch = 0.0f;
                    roll = 0.0f;
                    yaw = 0.0f;
                    gameOver = false;
                }
                if (IsKeyPressed(KEY_N))
                {
                    break;
                }
            }
            
        EndDrawing();
    }

    // ------------------------------
    // De-initialization
    // ------------------------------
    CloseAudioDevice();
    UnloadMusicStream(music_engine);
    UnloadModel(terrain);
    UnloadTexture(terrainTexture);
    UnloadShader(saturationShader);
    UnloadModel(plane);
    UnloadTexture(planeTexture);
    UnloadModel(runway);
    UnloadTexture(runwayTexture);

    CloseWindow();
    return 0;
}
