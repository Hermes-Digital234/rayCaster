#include "raylib.h"
#include <math.h>

#define SCREEN_W 960
#define SCREEN_H 600
#define MAP_W 16
#define MAP_H 16
#define STEP_TIME 0.18f // seconds per move/turn

static const int map[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,2,0,0,0,0,0,3,3,3,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,3,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,3,0,0,1},
    {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,3,0,0,0,0,0,0,0,2,0,0,0,1},
    {1,0,0,3,0,0,0,0,0,0,0,2,0,0,0,1},
    {1,0,0,3,3,0,0,0,0,0,2,2,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static Color wallColor(int id, bool sideHit) {
    Color c = (id == 1) ? (Color){200, 200, 200, 255}
            : (id == 2) ? (Color){200, 60, 60, 255}
                        : (Color){60, 90, 200, 255};
    if (sideHit) { c.r /= 2; c.g /= 2; c.b /= 2; }
    return c;
}

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "grid raycaster");
    SetTargetFPS(60);

    int gx = 8, gy = 8;
    float targetAngle = PI; // facing -x

    float posX = gx + 0.5f, posY = gy + 0.5f;
    float angle = targetAngle;

    float startX = posX, startY = posY, startAngle = angle;
    float t = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (t >= 1.0f) {
            int fx = (int)lroundf(cosf(targetAngle)), fy = (int)lroundf(sinf(targetAngle));
            int rx = (int)lroundf(sinf(targetAngle)), ry = (int)lroundf(-cosf(targetAngle));

            int mx = 0, my = 0; // requested cell offset
            float turn = 0;

            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    { mx =  fx; my =  fy; }
            else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { mx = -fx; my = -fy; }
            else if (IsKeyDown(KEY_E)) { mx =  rx; my =  ry; }
            else if (IsKeyDown(KEY_Q)) { mx = -rx; my = -ry; }
            else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  turn =  PI / 2;
            else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) turn = -PI / 2;

            bool start = false;
            if ((mx || my) && map[gy + my][gx + mx] == 0) { gx += mx; gy += my; start = true; }
            if (turn != 0) { targetAngle += turn; start = true; }

            if (start) {
                startX = posX; startY = posY; startAngle = angle;
                t = 0.0f;
            }
        }

        if (t < 1.0f) {
            t += dt / STEP_TIME;
            if (t > 1.0f) t = 1.0f;
            float s = t * t * (3.0f - 2.0f * t);
            posX  = lerpf(startX, gx + 0.5f, s);
            posY  = lerpf(startY, gy + 0.5f, s);
            angle = lerpf(startAngle, targetAngle, s);
        }

        float dirX = cosf(angle), dirY = sinf(angle);
        float planeX = sinf(angle) * 0.66f, planeY = -cosf(angle) * 0.66f;

        BeginDrawing();
        ClearBackground(BLACK);
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H / 2, (Color){40, 40, 60, 255});
        DrawRectangle(0, SCREEN_H / 2, SCREEN_W, SCREEN_H / 2, (Color){70, 60, 50, 255});

        for (int x = 0; x < SCREEN_W; x++) {
            float camX = 2.0f * x / (float)SCREEN_W - 1.0f;
            float rayX = dirX + planeX * camX;
            float rayY = dirY + planeY * camX;

            int mapX = (int)posX, mapY = (int)posY;
            float deltaX = (rayX == 0) ? 1e30f : fabsf(1.0f / rayX);
            float deltaY = (rayY == 0) ? 1e30f : fabsf(1.0f / rayY);

            int stepX, stepY;
            float sideX, sideY;
            if (rayX < 0) { stepX = -1; sideX = (posX - mapX) * deltaX; }
            else          { stepX =  1; sideX = (mapX + 1.0f - posX) * deltaX; }
            if (rayY < 0) { stepY = -1; sideY = (posY - mapY) * deltaY; }
            else          { stepY =  1; sideY = (mapY + 1.0f - posY) * deltaY; }

            bool sideHit = false;
            int hit = 0;
            while (!hit) {
                if (sideX < sideY) { sideX += deltaX; mapX += stepX; sideHit = false; }
                else               { sideY += deltaY; mapY += stepY; sideHit = true;  }
                hit = map[mapY][mapX];
            }

            float dist = sideHit ? (sideY - deltaY) : (sideX - deltaX);
            int lineH = (int)(SCREEN_H / dist);
            int top = SCREEN_H / 2 - lineH / 2;
            DrawLine(x, top, x, top + lineH, wallColor(hit, sideHit));
        }

        DrawFPS(8, 8);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}