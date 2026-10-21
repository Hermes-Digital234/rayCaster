// battleMain.h - STATE_BATTLE_MAIN: a turn-based fight against one enemy.
//
// Round structure: every living party member picks a command (Attack or Run) one after the other, each
// action resolves right away, and then the enemy takes its turn against a random living member.
#ifndef BATTLE_MAIN_H
#define BATTLE_MAIN_H

#include "enemies/enemy.h"

// Registers STATE_BATTLE_MAIN with the state manager.
void battleMainRegister(void);

// Starts a battle against the given enemy: pushes the battle state on top of the current one. The state
// underneath resumes when the battle ends (victory, escape, or the party being wiped out).
void battleMainStart(const EnemyDef *enemy);

#endif
