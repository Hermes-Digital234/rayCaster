#include "core/save.h"

#include "core/textParse.h"
#include "core/worldState.h"
#include "party/inventory.h"
#include "party/party.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SAVE_VERSION 1

static void slotPath(char *out, size_t size, int slot) {
    snprintf(out, size, "save%d.sav", slot + 1);
}

bool saveWrite(int slot, const SaveLocation *location) {
    if (slot < 0 || slot >= SAVE_SLOTS) return false;

    char path[64], tempPath[80];
    slotPath(path, sizeof(path), slot);
    snprintf(tempPath, sizeof(tempPath), "%s.tmp", path);

    FILE *file = fopen(tempPath, "w");
    if (!file) return false;

    fprintf(file, "raycasterSave %d\n", SAVE_VERSION);
    fprintf(file, "savedAt %lld\n", (long long)time(NULL));
    fprintf(file, "location %d %d %d %d %d %d\n", location->layer, location->map, location->x, location->y,
            location->dirX, location->dirY);
    for (int i = 0; i < inventoryStackCount(); i++) {
        const InventoryStack *stack = inventoryStackAt(i);
        fprintf(file, "item %s %d\n", stack->id, stack->count);
    }

    const Party *party = partyGet();
    for (int i = 0; i < party->count; i++) partyWriteMember(file, &party->members[i]);

    for (int i = 0; i < worldCount(); i++) {
        const WorldFlag *f = worldAt(i);
        switch (f->type) {
            case WORLD_CHEST_OPENED:    fprintf(file, "flag chest %d %d %d %d\n", f->layer, f->map, f->x, f->y); break;
            case WORLD_SKIP_DISCOVERED: fprintf(file, "flag skip %d %d %d %d\n", f->layer, f->map, f->x, f->y); break;
            case WORLD_EVENT_FIRED:     fprintf(file, "flag event %d %d %d %d %d\n", f->layer, f->map, f->x, f->y, f->index); break;
        }
    }

    bool ok = (fflush(file) == 0) && !ferror(file);
    if (fclose(file) != 0) ok = false;
    if (!ok) { remove(tempPath); return false; }

    if (rename(tempPath, path) != 0) { // some systems refuse to replace an existing file
        remove(path);
        if (rename(tempPath, path) != 0) { remove(tempPath); return false; }
    }
    return true;
}

// --- Reading: everything is parsed into this first, and only applied if the whole file is fine ---

typedef struct {
    const char *path;
    bool failed;
    bool haveHeader, haveLocation;
    long long savedAt;
    SaveLocation location;
    InventoryStack *stacks;
    int stackCount, stackCapacity;
    Party party;
    WorldFlag *flags;
    int flagCount, flagCapacity;
} Parsed;

static void fail(Parsed *p, int lineNumber, const char *message) {
    if (!p->failed) fprintf(stderr, "save: %s:%d: %s\n", p->path, lineNumber, message);
    p->failed = true;
}

static bool nextInt(char **line, int *out) {
    char token[32];
    return textNextToken(line, token, sizeof(token)) && textParseInt(token, out);
}

static void parseSaveLine(char *line, int lineNumber, void *user) {
    Parsed *p = user;
    if (p->failed) return;

    char kind[32];
    textNextToken(&line, kind, sizeof(kind));

    if (!p->haveHeader) {
        int version;
        if (strcmp(kind, "raycasterSave") != 0 || !nextInt(&line, &version) || version != SAVE_VERSION) {
            fail(p, lineNumber, "not a save file this version of the game can read");
            return;
        }
        p->haveHeader = true;
    } else if (!strcmp(kind, "savedAt")) {
        char token[40];
        p->savedAt = (textNextToken(&line, token, sizeof(token))) ? atoll(token) : 0;
    } else if (!strcmp(kind, "location")) {
        SaveLocation *l = &p->location;
        if (!nextInt(&line, &l->layer) || !nextInt(&line, &l->map) || !nextInt(&line, &l->x) || !nextInt(&line, &l->y) ||
            !nextInt(&line, &l->dirX) || !nextInt(&line, &l->dirY) || l->layer < 1 || l->map < 1 ||
            l->dirX < -1 || l->dirX > 1 || l->dirY < -1 || l->dirY > 1 || (l->dirX == 0) == (l->dirY == 0)) {
            fail(p, lineNumber, "bad location line");
            return;
        }
        p->haveLocation = true;
    } else if (!strcmp(kind, "item")) {
        InventoryStack s;
        memset(&s, 0, sizeof(s));
        if (!textNextToken(&line, s.id, sizeof(s.id)) || !nextInt(&line, &s.count) || s.count < 1) {
            fail(p, lineNumber, "bad item line");
            return;
        }
        if (p->stackCount == p->stackCapacity) {
            int capacity = p->stackCapacity ? p->stackCapacity * 2 : 16;
            InventoryStack *grown = realloc(p->stacks, (size_t)capacity * sizeof(InventoryStack));
            if (!grown) { fail(p, lineNumber, "out of memory"); return; }
            p->stacks = grown;
            p->stackCapacity = capacity;
        }
        p->stacks[p->stackCount++] = s;
    } else if (!strcmp(kind, "member")) {
        if (p->party.count >= MAX_PARTY) { fail(p, lineNumber, "too many party members"); return; }
        Character c;
        if (!partyParseMember(line, &c, p->path, lineNumber)) { p->failed = true; return; }
        p->party.members[p->party.count++] = c;
    } else if (!strcmp(kind, "flag")) {
        char type[16];
        WorldFlag f;
        memset(&f, 0, sizeof(f));
        if (!textNextToken(&line, type, sizeof(type)) || !nextInt(&line, &f.layer) || !nextInt(&line, &f.map) ||
            !nextInt(&line, &f.x) || !nextInt(&line, &f.y)) {
            fail(p, lineNumber, "bad flag line");
            return;
        }
        if (!strcmp(type, "chest"))      f.type = WORLD_CHEST_OPENED;
        else if (!strcmp(type, "skip"))  f.type = WORLD_SKIP_DISCOVERED;
        else if (!strcmp(type, "event")) {
            f.type = WORLD_EVENT_FIRED;
            if (!nextInt(&line, &f.index)) { fail(p, lineNumber, "bad event flag"); return; }
        } else {
            fail(p, lineNumber, "unknown flag type");
            return;
        }

        if (p->flagCount == p->flagCapacity) {
            int capacity = p->flagCapacity ? p->flagCapacity * 2 : 32;
            WorldFlag *grown = realloc(p->flags, (size_t)capacity * sizeof(WorldFlag));
            if (!grown) { fail(p, lineNumber, "out of memory"); return; }
            p->flags = grown;
            p->flagCapacity = capacity;
        }
        p->flags[p->flagCount++] = f;
    } else {
        fail(p, lineNumber, "unknown line");
    }
}

// Parses a slot completely. True only if the file is whole and consistent. The caller frees p->flags.
static bool parseSlot(int slot, Parsed *p) {
    memset(p, 0, sizeof(*p));
    char path[64];
    slotPath(path, sizeof(path), slot);
    p->path = path;

    if (!textForEachLine(path, parseSaveLine, p)) return false; // no file: not an error worth printing

    if (!p->failed && (!p->haveHeader || !p->haveLocation || p->party.count == 0)) {
        fprintf(stderr, "save: %s: the file is incomplete\n", path);
        p->failed = true;
    }
    p->path = NULL; // the local buffer goes out of scope
    return !p->failed;
}

SaveInfo saveInfo(int slot) {
    SaveInfo info = {0};
    if (slot < 0 || slot >= SAVE_SLOTS) return info;

    Parsed p;
    if (parseSlot(slot, &p)) {
        info.exists = true;
        info.layer = p.location.layer;
        info.map = p.location.map;
        info.savedAt = p.savedAt;
    }
    free(p.flags);
    free(p.stacks);
    return info;
}

bool saveRead(int slot, SaveLocation *location) {
    if (slot < 0 || slot >= SAVE_SLOTS) return false;

    Parsed p;
    bool ok = parseSlot(slot, &p);
    if (ok) {
        *partyGet() = p.party;
        inventoryReset();
        for (int i = 0; i < p.stackCount; i++) inventoryAdd(p.stacks[i].id, p.stacks[i].count);
        worldReset();
        for (int i = 0; i < p.flagCount; i++)
            worldMark(p.flags[i].type, p.flags[i].layer, p.flags[i].map, p.flags[i].x, p.flags[i].y, p.flags[i].index);
        *location = p.location;
    }
    free(p.flags);
    free(p.stacks);
    return ok;
}
