// saveLoadMenu.h - STATE_SAVE_LOAD: pick one of the save slots to write to or read from.
//
// Usage: open it, and when it closes the state underneath gets its resume() callback. After opening it for
// loading, resume() calls saveLoadMenuTakeLoaded() to find out whether a save was loaded and where to.
#ifndef SAVE_LOAD_MENU_H
#define SAVE_LOAD_MENU_H

#include "core/save.h"
#include <stdbool.h>

void saveLoadMenuRegister(void);

// Save the current run at `here` into a slot the player picks (asks before overwriting).
void saveLoadMenuOpenSave(const SaveLocation *here);

// Load a slot the player picks. The party, items and map changes are already replaced when it closes.
void saveLoadMenuOpenLoad(void);

// True once after a successful load, with the saved location. The caller moves the player there.
bool saveLoadMenuTakeLoaded(SaveLocation *out);

#endif
