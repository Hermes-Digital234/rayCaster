// save.h - saving and loading the run: party, item count, changes made to maps, and where the player stands.
//
// A save is a small text file per slot, written next to the executable as save1.sav, save2.sav, ... It is
// written to a temporary file first and then renamed, so a crash while saving never destroys the old save.
//
// File format:
//   raycasterSave 1
//   savedAt <unix time>
//   location <layer> <map> <x> <y> <dirX> <dirY>
//   items <count>
//   member class=Warrior health=100 hp=85 weapon=trainingSword armor=trainingArmor ... Warrior   (party.h)
//   flag chest <layer> <map> <x> <y>
//   flag skip  <layer> <map> <x> <y>
//   flag event <layer> <map> <x> <y> <index>
#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>

#define SAVE_SLOTS 3

// Where the player is. (dirX, dirY) is the direction they face, e.g. (1,0) = east.
typedef struct {
    int layer, map;
    int x, y;
    int dirX, dirY;
} SaveLocation;

typedef struct {
    bool exists;           // a readable save is in this slot
    int layer, map;        // where it was saved
    long long savedAt;     // unix time
} SaveInfo;

// Slots are numbered 0 to SAVE_SLOTS - 1.
SaveInfo saveInfo(int slot);

// Writes the current party, items and map changes together with the location. False if the file cannot be written.
bool saveWrite(int slot, const SaveLocation *location);

// Reads a slot. On success the party, items and map changes are replaced by the saved ones, *location is filled in
// and true is returned. If the file is missing or damaged, nothing is changed and false is returned.
bool saveRead(int slot, SaveLocation *location);

#endif
