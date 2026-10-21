#!/usr/bin/env python3
"""checkMap.py - sanity-checks map images for the dungeon crawler.

Usage:  python3 tools/checkMap.py assets/maps/l1m1.png [more maps...]
        python3 tools/checkMap.py assets/maps/*.png

Needs Pillow (Arch: sudo pacman -S python-pillow).
Reports problems the game itself would silently accept.
"""
import os
import re
import sys
from PIL import Image

PALETTE = {
    "floor": (255, 255, 255), "wall": (0, 0, 0), "water": (0, 0, 255), "chest": (255, 255, 0),
    "door": (255, 0, 0), "heal": (0, 255, 0), "event": (255, 128, 192),
    "stairsUp": (128, 0, 255), "stairsDown": (255, 165, 0), "skip": (255, 0, 255),
}
WALKABLE = {"floor", "heal", "event"}
DIRS = ((1, 0), (0, 1), (-1, 0), (0, -1))


def nearest(rgb):
    return min(PALETTE, key=lambda n: sum((a - b) ** 2 for a, b in zip(rgb, PALETTE[n])))


def check(path):
    problems, notes = [], []
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    grid = [[None] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            r, g, b, a = img.getpixel((x, y))
            name = nearest((r, g, b))
            grid[y][x] = name
            if (r, g, b) != PALETTE[name]:
                problems.append(f"({x},{y}) is ({r},{g},{b}), not an exact palette color; the game reads it as {name}")
            if a != 255:
                problems.append(f"({x},{y}) is not fully opaque (alpha {a}); the game ignores alpha")

    def at(x, y):
        return grid[y][x] if 0 <= x < w and 0 <= y < h else "wall"

    walk = [(x, y) for y in range(h) for x in range(w) if grid[y][x] == "floor"]
    if not walk:
        problems.append("no white floor tile, so the player has no start position")
    else:
        notes.append(f"player starts at {walk[0]} (first white tile, reading left to right, top to bottom)")

    for y in range(h):
        for x in range(w):
            t = grid[y][x]
            if (x in (0, w - 1) or y in (0, h - 1)) and t in WALKABLE | {"water", "chest"}:
                problems.append(f"({x},{y}) {t} on the map edge: the edge should be wall")
            if t in ("door", "skip"):
                ok = any(at(x - dx, y - dy) in WALKABLE and at(x + dx, y + dy) in WALKABLE for dx, dy in DIRS[:2])
                if not ok:
                    problems.append(f"({x},{y}) {t} needs a walkable tile on both opposite sides, or it can never be used")
            if t in ("stairsUp", "stairsDown"):
                if not any(at(x + dx, y + dy) in WALKABLE for dx, dy in DIRS):
                    problems.append(f"({x},{y}) {t} has no walkable neighbour")
            if t == "chest":
                if not any(at(x + dx, y + dy) in WALKABLE for dx, dy in DIRS):
                    problems.append(f"({x},{y}) chest has no walkable neighbour, it can never be opened")

    for kind in ("stairsUp", "stairsDown"):
        n = sum(row.count(kind) for row in grid)
        if n > 1:
            notes.append(f"{n} {kind} tiles: the game uses the first one (top-left first) as the arrival point")

    # Reachability from the start. Doors and skips connect the cells on either side of them.
    # Chests, water and walls block, so a chest in a 1-wide corridor shows up here.
    if walk:
        seen, todo = {walk[0]}, [walk[0]]
        while todo:
            x, y = todo.pop()
            for dx, dy in DIRS:
                nx, ny = x + dx, y + dy
                t = at(nx, ny)
                if t in WALKABLE:
                    dest = (nx, ny)
                elif t in ("door", "skip") and at(nx + dx, ny + dy) in WALKABLE:
                    dest = (nx + dx, ny + dy)
                else:
                    continue
                if dest not in seen:
                    seen.add(dest)
                    todo.append(dest)
        lost = [(x, y) for y in range(h) for x in range(w) if grid[y][x] in WALKABLE and (x, y) not in seen]
        if lost:
            problems.append(f"{len(lost)} walkable tile(s) cannot be reached from the start, first at {lost[0]} "
                            "(a chest, water or a stair-only connection in the way?)")

    # Data file: positions must point at the right kind of tile
    m = re.match(r"l(\d+)m(\d+)\.png$", os.path.basename(path))
    if m:
        dataPath = os.path.join(os.path.dirname(path), "..", "data", f"l{m.group(1)}m{m.group(2)}.txt")
        if os.path.exists(dataPath):
            for number, line in enumerate(open(dataPath), 1):
                parts = line.split()
                if len(parts) >= 3 and parts[0] in ("skip", "event"):
                    try:
                        x, y = int(parts[1]), int(parts[2])
                    except ValueError:
                        continue
                    want = "skip" if parts[0] == "skip" else "event"
                    if at(x, y) != want:
                        problems.append(f"data file line {number}: {parts[0]} {x} {y} points at {at(x, y)}, not {want}")
    return problems, notes


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    bad = 0
    for path in sys.argv[1:]:
        problems, notes = check(path)
        print(f"{path}: {'OK' if not problems else str(len(problems)) + ' problem(s)'}")
        for n in notes:
            print(f"  note: {n}")
        for p in problems[:30]:
            print(f"  PROBLEM: {p}")
        if len(problems) > 30:
            print(f"  ... and {len(problems) - 30} more")
        bad += bool(problems)
    return 1 if bad else 0


sys.exit(main())
