#include "dungeon/interaction.h"

InteractKind interactionKind(const Map *map, int tileX, int tileY, int dirX, int dirY) {
    int tile = mapTile(map, tileX, tileY);

    switch (tile) {
        case T_DOOR:
        case T_SKIP: {
            // Only offer to go through if there is somewhere to stand on the far side
            bool somewhereToGo = mapWalkable(mapTile(map, tileX + dirX, tileY + dirY));
            if (!somewhereToGo) return INTERACT_NONE;
            return tile == T_DOOR ? INTERACT_DOOR : INTERACT_SKIP;
        }
        case T_STAIRS_UP:   return INTERACT_STAIRS_UP;
        case T_STAIRS_DOWN: return INTERACT_STAIRS_DOWN;
        case T_ITEM:        return INTERACT_CHEST; // T_CHEST_OPEN is deliberately not here
        default:            return INTERACT_NONE;
    }
}

const char *interactionPrompt(InteractKind kind) {
    switch (kind) {
        case INTERACT_DOOR:        return "Open the door?";
        case INTERACT_STAIRS_UP:   return "Go up the stairs?";
        case INTERACT_STAIRS_DOWN: return "Go down the stairs?";
        case INTERACT_SKIP:        return "Push through the wall?";
        case INTERACT_CHEST:       return "Open the treasure chest?";
        case INTERACT_NONE:        break;
    }
    return "";
}

const char *interactionLabel(InteractKind kind) {
    switch (kind) {
        case INTERACT_DOOR:        return "Open door";
        case INTERACT_STAIRS_UP:   return "Go up";
        case INTERACT_STAIRS_DOWN: return "Go down";
        case INTERACT_SKIP:        return "Push wall";
        case INTERACT_CHEST:       return "Open chest";
        case INTERACT_NONE:        break;
    }
    return "";
}