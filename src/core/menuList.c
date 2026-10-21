#include "core/menuList.h"
#include "raylib.h"

#define TITLE_SIZE 48
#define ITEM_SIZE 28
#define ROW_HEIGHT 40
#define LIST_TOP 160
#define LIST_BOTTOM_MARGIN 70

static const Color backgroundColor = {18, 18, 28, 255};
static const Color titleColor = {235, 235, 245, 255};
static const Color itemColor = {170, 170, 185, 255};
static const Color selectedColor = {255, 215, 0, 255};
static const Color dimColor = {110, 110, 125, 255};

static void drawCentered(const char *text, int y, int size, Color color) {
    DrawText(text, (GetScreenWidth() - MeasureText(text, size)) / 2, y, size, color);
}

int menuListMove(int selected, int count, MenuAction action) {
    if (count <= 0) return 0;
    if (action == MENU_UP)   return (selected - 1 + count) % count;
    if (action == MENU_DOWN) return (selected + 1) % count;
    return selected;
}

void menuListDraw(const char *title, const char *const *labels, int count, int selected,
                  const char *emptyMessage, const char *hint) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, backgroundColor);
    drawCentered(title, 50, TITLE_SIZE, titleColor);

    int rows = (h - LIST_TOP - LIST_BOTTOM_MARGIN) / ROW_HEIGHT;
    if (rows < 1) rows = 1;

    if (count <= 0) {
        if (emptyMessage) drawCentered(emptyMessage, LIST_TOP, ITEM_SIZE, dimColor);
    } else {
        // Scroll window: keep the selection roughly centered once the list is long enough
        int first = 0;
        if (count > rows) {
            first = selected - rows / 2;
            if (first < 0) first = 0;
            if (first > count - rows) first = count - rows;
        }

        for (int i = 0; i < rows && first + i < count; i++) {
            int index = first + i;
            bool isSelected = (index == selected);
            const char *text = isSelected ? TextFormat("> %s <", labels[index]) : labels[index];
            drawCentered(text, LIST_TOP + i * ROW_HEIGHT, ITEM_SIZE, isSelected ? selectedColor : itemColor);
        }

        if (first > 0)             drawCentered("^", LIST_TOP - 28, ITEM_SIZE, dimColor);
        if (first + rows < count)  drawCentered("v", LIST_TOP + rows * ROW_HEIGHT, ITEM_SIZE, dimColor);
    }

    if (hint) drawCentered(hint, h - 40, 20, dimColor);
}
