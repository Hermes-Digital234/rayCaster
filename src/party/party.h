// party.h - the player's party: up to MAX_PARTY characters, shared by the whole game.
//
// The starting party comes from assets/data/party.txt. A future party-creation screen only has to fill the same
// Party struct (names, classes, equipment); nothing that reads the party needs to change.
//
// File format (# for comments), one line per character in party order:
//
//   member [class=<word>] [health=<max hp>] [hp=<current hp>] [weapon=<item id>] [armor=<item id>]
//          [misc1=<item id>] [misc2=<item id>] [misc3=<item id>] <name>
//
//   class    a one-word label such as Warrior (default Adventurer)
//   health   maximum health (default 100)
//   hp       current health (default: full; saves use this)
//   weapon/armor/misc1-3   ids from items.txt, or - for nothing. A bad id is reported and left empty.
//   The name is the rest of the line and may contain spaces. At most MAX_PARTY members are used.
//
//   member class=Warrior health=100 weapon=trainingSword armor=trainingArmor Warrior
#ifndef PARTY_H
#define PARTY_H

#include "party/character.h"
#include <stdio.h>

#define PARTY_FILE "assets/data/party.txt"
#define MAX_PARTY 5

typedef struct {
    Character members[MAX_PARTY];
    int count;
} Party;

// The one party of the running game.
Party *partyGet(void);

// Replaces the party with the one in PARTY_FILE. If that file cannot be read or has no members, the party is
// empty and an error is printed on stderr.
void partyInitDefault(void);

// Same, from any file. Returns the number of members loaded, or -1 if the file could not be read.
int partyLoadFile(const char *path);

// Parses the text after "member" into a character. False (with a message on stderr) if it is unusable.
bool partyParseMember(char *line, Character *out, const char *path, int lineNumber);

// Writes one "member ..." line for a character (used by saves; the output can be read by partyParseMember).
void partyWriteMember(FILE *file, const Character *c);

// Full health for everyone, including the fallen.
void partyRestore(void);

int partyLivingCount(const Party *p);

// Index of the first living member at or after `from`, or -1.
int partyFirstLiving(const Party *p, int from);

#endif
