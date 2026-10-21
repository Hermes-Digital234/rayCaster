# Map making guide

Maps are ordinary PNG images. **One pixel is one tile.** A 21x14 image is a 21x14 map. You draw the dungeon in any pixel-art editor, save it as PNG, and the game reads the colors.

This guide covers the file layout, the color palette, what each tile does, the data files for skips and events, which editors work well, and a checker script that catches mistakes.

---

## 1. Files and naming

| What | Where | Name |
|------|-------|------|
| Map image | `assets/maps/` | `l<layer>m<map>.png`, e.g. `l1m1.png` |
| Map data (optional) | `assets/data/` | same name with `.txt`, e.g. `l1m1.txt` |
| Chest textures | `assets/textures/` | `chestClosed.png`, `chestOpen.png` |

- **Layer** is a set of floors (a tower, a dungeon, a biome). It is also where per-layer textures will go later.
- **Map** is the floor number inside the layer. Stairs go to map `N-1` (up) and `N+1` (down) **of the same layer**.
- Both numbers start at 1. `l0m1.png`, `l1m0.png` and names like `l1m1x.png` are ignored.
- Numbers do not need to be contiguous for the Development menu, but stairs only work between consecutive maps (`l1m2` to `l1m3`). If there is no floor above or below, the player gets "There is no floor above/below."
- The Development menu lists every valid map in numeric order, so a new map shows up by itself.
- Maps are copied next to the executable on every build. After adding or changing a map, run `cmake --build build` (no code change needed).

---

## 2. The palette

The game matches each pixel to the **nearest** palette color, so a slightly-off shade still works, but be exact anyway (see the pitfalls below).

| Color | RGB | Hex | Tile | In game |
|-------|-----|-----|------|---------|
| White | 255, 255, 255 | `#FFFFFF` | Floor | Walkable. The player starts on the first one (see 4). |
| Black | 0, 0, 0 | `#000000` | Wall | Solid grey wall. |
| Blue | 0, 0, 255 | `#0000FF` | Water | Blue floor you cannot step on but can see past. |
| Yellow | 255, 255, 0 | `#FFFF00` | Chest | Chest sprite. Blocks the player. Interact to open it. |
| Red | 255, 0, 0 | `#FF0000` | Door | Red wall segment. Interact to fade through to the other side. |
| Purple | 128, 0, 255 | `#8000FF` | Stairs up | Purple wall segment. Goes to map `N-1`. |
| Orange | 255, 165, 0 | `#FFA500` | Stairs down | Orange wall segment. Goes to map `N+1`. |
| Magenta | 255, 0, 255 | `#FF00FF` | Hidden skip | Looks exactly like a normal wall. Passable by interacting (see 5). |
| Green | 0, 255, 0 | `#00FF00` | Healing spot | Green floor. Walkable. No effect yet. |
| Pink | 255, 128, 192 | `#FF80C0` | Event | Pink floor. Walkable. Press interact on it to start its event (see 6). |

Anything else is matched to the closest of these. A dark grey pixel becomes black, a light grey one becomes white, a red-ish orange could become either red or orange. Do not rely on this.

### Loading the palette into your editor

Save this as `dungeon.gpl`. Aseprite, GIMP, Krita and LibreSprite can load GIMP palette files, and Pixelorama can import them too.

```
GIMP Palette
Name: Dungeon
Columns: 5
#
255 255 255 floor
  0   0   0 wall
  0   0 255 water
255 255   0 chest
255   0   0 door
128   0 255 stairs up
255 165   0 stairs down
255   0 255 hidden skip
  0 255   0 healing spot
255 128 192 event
```

Having only these ten colors in your palette makes it nearly impossible to paint a wrong one.

---

## 3. How tiles behave

**The player** moves one cell at a time (W/S/Q/E), turns in 90 degree steps (A/D), and can only act while standing still.

**The interact button.** When there is something to use, a `[SPACE] <action>` button appears at the bottom of the screen. Space, Enter or Z presses it. It appears when you are:

- facing a door, stairs, hidden skip or **closed** chest in the next cell, or
- standing on a pink event tile that still has an event to run.

Walking into things does nothing. Doors, stairs, skips and chests ask a Yes/No question first. Events start directly.

| Tile | Rule to follow when drawing it |
|------|-------------------------------|
| **Wall** | Everything outside the map counts as wall, but still close off the edge with black so the map looks right in the editor. |
| **Water** | Not walkable, not solid for the view. Good for pools, pits and views across a gap. |
| **Door** | One tile thick, with a walkable tile on **both** sides, in a straight line (left and right, or above and below). Interacting fades out, then you appear on the far side. A door with a wall behind it does not offer the button. |
| **Stairs** | Place in a wall, with at least one walkable neighbor. Going **up** (purple) arrives next to the **orange** stairs of the map above; going **down** (orange) arrives next to the **purple** stairs of the map below. You arrive facing away from the stairs. So every map you can reach by stairs needs the matching staircase. If a map has several staircases of the same color, the first one (top-left first) is the arrival point. |
| **Chest** | Solid. Becomes an open chest (different texture) when opened and stays solid. **Never put one where it blocks the only path**, such as a 1-wide corridor: the player can never walk past it. Put chests in rooms, alcoves and dead ends. |
| **Healing spot** | Walkable floor, green. Does nothing yet. |
| **Event** | Walkable pink floor. Nothing happens by stepping on it. The player presses interact there. A pink tile with no entry in the data file shows no button. |
| **Hidden skip** | Same rule as doors: walkable cells on both sides in a line. Drawn as a normal grey wall (see 5). |

### Size and limits

- No fixed size. 20x20 to 40x40 is a comfortable range for this style.
- At most 256 chests are drawn per map.
- Draw all four edges as black wall. Walkable tiles on the edge look odd and the checker flags them.

---

## 4. Where the player starts

The player starts on the **first white floor tile when reading the image left to right, top to bottom** (the top-most row first, and inside a row the left-most pixel first).

- To start somewhere specific, make sure no white tile comes before it in reading order. Use green (heal) or pink tiles for floor higher up or further left if needed.
- Using stairs does not use this rule. You arrive at the matching staircase.
- If a map has no white tile at all, it cannot be loaded.

---

## 5. Hidden skips (magenta) and the data file

A magenta pixel is a wall you can push through, like the hidden skips in Etrian Odyssey. It is drawn **exactly** like a normal wall, but when you face it from either side, the interact button appears (`Push wall`).

By default it works from both sides straight away. Use the data file to restrict the **first** crossing:

```
skip <x> <y> [from=<north|east|south|west|any>]
```

- `x` and `y` are the pixel coordinates in the image, counted from the top-left pixel as `0,0`.
- `from=west` means the player has to be standing **west** of the wall, walking east. From the wrong side the popup still appears but says "The wall doesn't budge."
- After one successful crossing from the right side, the skip is *discovered* and works from both sides.
- Discovery is kept in memory only while the map is loaded. It is lost when you leave the map or the game. (Saving comes later.)

Note that the interact button appears whenever you face a skip, so a skip is easy to find by looking at walls. If you want skips that stay secret, say so and we can add an option for it.

---

## 6. Events (pink) and questions

All of the following goes into `assets/data/l<layer>m<map>.txt`. Lines starting with `#` are comments. Blank lines are ignored. Mistakes are reported on the terminal and the bad line is skipped; the game still runs.

### Plain events

```
event <x> <y> [chance=0-100] [once=0|1] <message|battle|ask> <text>
```

| Part | Meaning |
|------|---------|
| `x y` | Pixel position of a pink tile. |
| `chance` | Percent chance this event happens when the player interacts (default 100). |
| `once=1` | It can only happen once per map load. Afterwards the button disappears if nothing else is left on that tile. |
| `message <text>` | Shows the text in a popup. |
| `battle <group>` | Starts a battle. Battles do not exist yet, so a placeholder popup shows the group name. |
| `ask <question>` | Shows the question and a menu of answers (below). |

Several `event` lines on the same tile are tried **in file order**. The first one whose chance roll succeeds happens. If none succeed, the player sees "You look around but find nothing."

```
# 40% a battle, otherwise you hear something
event 12 9 chance=40 battle slimes
event 12 9 message You hear a faint whisper somewhere in the dark.
```

### Questions with answers

Put `option` lines directly under an `ask` event:

```
option <label> => [chance=0-100] <message|battle|item|nothing> [text]
```

```
event 4 11 once=1 ask You see a bag left behind by another adventurer. Pick it up?
option Pick it up => chance=25 battle goblins
option Pick it up => item 1 Inside the bag you find a healing potion.
option Leave it => message You leave the bag where it lies.
option Check for traps first => message You find nothing suspicious.
```

- Up to **4 different labels** per question. Each label is a menu entry, in the order they first appear.
- The **same label repeated** adds more possible outcomes for that answer. They are tried in order, and the first whose `chance` succeeds happens. So give the last one no `chance` (always succeeds). In the example, "Pick it up" is a 25% ambush, otherwise you get the potion.
- Outcomes:

| Outcome | Result |
|---------|--------|
| `message <text>` | Popup with the text. |
| `battle <group>` | Battle (placeholder for now). |
| `item <count> [text]` | Adds items to the player and shows the text, or "You received N items." if you leave it out. |
| `nothing` | Closes the menu, nothing happens. |

- The player must pick an answer. Esc does not close a question, so always include a "Leave it" style answer.
- An `ask` with no valid `option` lines is shown as a plain message.
- An `option` line only works directly below an `ask` line (other `event` or `skip` lines end the list).
- Keep text short. Long text wraps in the popup, but very long text gets cut at about 160 characters.

---

## 7. Editors: what to use

What matters for this game is that the editor can:

1. Draw with **no anti-aliasing and no smoothing** (hard single pixels).
2. Work at **1 pixel = 1 tile**, zoomed in, with a visible grid.
3. Show the **pixel coordinate** under the cursor, so you can write data-file lines.
4. Load a fixed **palette** and export a plain **RGB PNG** (no transparency, no color profile).

| Editor | Cost | Verdict |
|--------|------|---------|
| **Aseprite** | Small one-time price (or build from source for free) | Best option. Pixel-first, palette files, grid, coordinates in the status bar, layers for a reference overlay. |
| **LibreSprite** | Free | Fork of the older free Aseprite. Nearly the same workflow. A good free pick. |
| **Pixelorama** | Free | Modern free pixel-art editor with palettes, grids and layers. Good if you want something maintained and free. |
| **GIMP** | Free | Works, but you must be careful (see below). Fine if you already have it. |
| **Krita** | Free | Works with a pixel brush and anti-aliasing off. More tool than needed. |
| **Piskel** | Free, in the browser | Quick and simple for small maps. Fewer features. |

Check your package manager, the AUR or Flatpak for the exact package names on your system.

### Aseprite / LibreSprite / Pixelorama

1. New file, size = your map size (e.g. 21x14), **RGB** color mode, no transparent background (fill the background with black so you start from solid wall).
2. Load `dungeon.gpl` as the palette.
3. Turn on the pixel grid and zoom in until each tile is large.
4. Use the pencil, rectangle and fill tools. Avoid anything that blends.
5. Hide reference or sketch layers, then export as PNG at **100% / 1x scale**. Do not export a sprite sheet and do not scale up.

### GIMP

1. **File > New**, set width and height in pixels to the map size. Under Advanced options set the fill to a solid color and turn on **no color management** if offered.
2. **Image > Mode > RGB**. Do not use Indexed mode while editing.
3. Use the **Pencil** tool, not the Paintbrush. The Pencil has no anti-aliasing. For the **Bucket Fill**, set the threshold to 0 and turn off "Antialiasing" and "Feather edges".
4. **View > Show Grid**, and set the grid under **Image > Configure Grid** to 1x1 pixels. Zoom in with `+`.
5. The cursor coordinates are in the bottom-left of the window.
6. Export with **File > Export As**, `.png`. In the PNG dialog, untick "Save color values from transparent pixels" and "Save background color", and do not add gamma or color profile data.

### Common pitfalls with any editor

- **Anti-aliasing / soft brushes** create in-between colors on every edge. They get matched to the nearest palette color, and the result is usually wrong. Always use hard, 1-pixel tools.
- **Scaling the canvas** (Image > Scale) blends pixels. If you must resize, use "no interpolation / nearest neighbour", or better, just change the canvas size.
- **Color profiles / color management** can shift colors slightly on export. Strip profiles on export when the editor allows it.
- **Transparency:** a transparent pixel still has an RGB value (often black or white). The game ignores alpha, so a "transparent" area becomes a wall or a floor. Fill everything with solid color.
- **Indexed PNG** usually works, but RGB is the safe choice.
- **Jpeg is not supported.** Lossy compression ruins the colors. Save as PNG.

### Other ways to make maps

- **Text first:** many people sketch a dungeon in a text file (`#` wall, `.` floor, `D` door, ...) and convert it to PNG with a small script. This is fast for iterating on layout and very easy to keep in git. If you want that workflow, ask and I will write the converter.
- **Tiled (map editor):** it can be used with a 1x1-pixel tileset of the ten colors and then exported as an image, but this is not tested with this game, and it is more setup than a pixel editor needs.

---

## 8. Check your map before running the game

`tools/checkMap.py` catches mistakes the game would accept silently. It needs Pillow:

```
sudo pacman -S python-pillow
python3 tools/checkMap.py assets/maps/l1m1.png
python3 tools/checkMap.py assets/maps/*.png
```

It reports:

- pixels that are not an **exact** palette color, and pixels that are not fully opaque,
- walkable tiles, water or chests on the **map edge**,
- doors and skips without a walkable tile on both sides, stairs and chests without a walkable neighbor,
- walkable areas the player **cannot reach** from the start (for example, a chest blocking a corridor),
- data-file lines (`skip`, `event`) that point at the wrong kind of tile,
- where the player will start.

It exits with a non-zero status if there are problems, so it can be used in a script or a git hook. It does not check the stairs between maps (that a matching staircase exists on the next map), so check those by hand.

---

## 9. Checklist for a new map

1. Create `assets/maps/l<layer>m<map>.png` with only palette colors, a black edge, and a white tile where the player should start (first in reading order).
2. Doors and skips: one tile thick, walkable on both sides.
3. Stairs: one orange for the way down and one purple for the way up. The map you connect to needs the matching color.
4. Chests in rooms and dead ends, never in the only path.
5. Run `python3 tools/checkMap.py` on it.
6. Optional: create `assets/data/l<layer>m<map>.txt` for skips and events.
7. Build (`cmake --build build`) and open it from **Development** in the main menu.
8. Walk it: try every door, skip (from both sides), staircase, chest and event.
