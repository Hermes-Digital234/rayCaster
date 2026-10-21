// party.h - the player's party: up to MAX_PARTY characters, shared by the whole game.
//
// partyInitDefault() builds a hardcoded test party. A future party-creation screen only has to fill the same
// Party struct (names, classes, equipment); nothing that reads the party needs to change.
#ifndef PARTY_H
#define PARTY_H

#include "party/character.h"

#define MAX_PARTY 5

typedef struct {
    Character members[MAX_PARTY];
    int count;
} Party;

// The one party of the running game.
Party *partyGet(void);

// Replaces the party with the hardcoded test party: 4 characters, 100 health, a weapon with 20 damage and
// a main armor with 300 armor each.
void partyInitDefault(void);

// Full health for everyone, including the fallen.
void partyRestore(void);

int partyLivingCount(const Party *p);

// Index of the first living member at or after `from`, or -1.
int partyFirstLiving(const Party *p, int from);

#endif
