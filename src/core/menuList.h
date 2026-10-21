// menuList.h - the vertical list every menu screen shares (cursor movement + drawing)
#ifndef MENU_LIST_H
#define MENU_LIST_H

#include "core/input.h"

// Returns the new selection after applying an up/down action. Wraps around.
int menuListMove(int selected, int count, MenuAction action);

// Draws a full-screen menu. Long lists scroll to keep the selection visible.
//   emptyMessage: shown instead of the list when count == 0 (may be NULL)
//   hint:         small line at the bottom (may be NULL)
void menuListDraw(const char *title, const char *const *labels, int count, int selected,
                  const char *emptyMessage, const char *hint);

#endif
