# Data files and saves

Everything game-specific lives in plain text files under `assets/data/`. Lines starting with `#` and blank lines are ignored. A bad line prints a warning (file and line number) on the terminal and is skipped.

## items.txt
`item <id> <weapon|armor|charm> [damage=N] [armor=N] <display name>`

    item trainingSword weapon damage=20 Training Sword
    item trainingArmor armor armor=300 Training Armor

Weapons fit the weapon slot, armor the main-armor slot, charms the three misc slots.

## enemies.txt
`enemy <id> health=N damage=N armor=N texture=<path> <display name>`
`attack <enemyId> [multiplier=1.0] [weight=1] <attack name>`

An enemy without `attack` lines gets a plain "Attack". Map files start fights with `battle <enemyId>`.
Armor: damage reduction = armor / (armor + 300), minimum 1 damage per hit.

## party.txt
`member [class=<label>] [health=N] [hp=N] [weapon=<itemId>] [armor=<itemId>] [misc1=<itemId>] [misc2=..] [misc3=..] <name>`

Up to 5 members. `class` is just a label shown in the menus. Empty party = battles refuse to start.

## Saves
Three slots, written as `save1.sav`..`save3.sav` next to the executable (text files, written atomically; a damaged file is refused whole). A save holds the location, party, item count and every map change (opened chests, discovered skips, once-only events fired).

## Game flow
Esc in the dungeon opens the pause menu (Resume, Party, Save Game, Load Game, Leave Dungeon). If every party member falls, the game-over screen offers Load Game or Return to Title.
