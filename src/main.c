#include "input.h"
#include "map.h"
#include "player.h"
#include "raycaster.h"
#include "raylib.h"
#include <stdio.h>

#define WIN_W 960
#define WIN_H 600

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "map.png";

    Map map;
    if (!map_load(&map, path)) {
        fprintf(stderr, "Could not load map image '%s'\n", path);
        return 1;
    }

    int startX, startY;
    if (!map_find_start(&map, &startX, &startY)) {
        fprintf(stderr, "Map has no white floor tile to start on\n");
        map_free(&map);
        return 1;
    }

    InitWindow(WIN_W, WIN_H, "raycaster - image map");
    SetTargetFPS(60);

    Raycaster rc;
    rc_init(&rc);

    Player player;
    player_init(&player, &map, startX, startY);

    while (!WindowShouldClose()) {
        player_update(&player, &map, input_poll(), GetFrameTime());

        rc_render(&rc, &map, player.posX, player.posY, player.angle);

        BeginDrawing();
        ClearBackground(BLACK);
        rc_draw(&rc, WIN_W, WIN_H);
        if (player.fade > 0.0f)
            DrawRectangle(0, 0, WIN_W, WIN_H, (Color){0, 0, 0, (unsigned char)(player.fade * 255)});
        DrawText(TextFormat("Items: %d", player.itemsCollected), 12, 12, 24, WHITE);
        DrawFPS(WIN_W - 90, 8);
        EndDrawing();
    }

    rc_free(&rc);
    map_free(&map);
    CloseWindow();
    return 0;
}
