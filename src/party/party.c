#include "party/party.h"

static Party party;

Party *partyGet(void) {
    return &party;
}

void partyInitDefault(void) {
    static const struct { const char *name; CharacterClass cls; } roster[] = {
        {"Warrior", CLASS_WARRIOR},
        {"Rogue", CLASS_ROGUE},
        {"Mage", CLASS_MAGE},
        {"Healer",  CLASS_HEALER},
    };

    party.count = (int)(sizeof(roster) / sizeof(roster[0]));
    for (int i = 0; i < party.count; i++) {
        Character *c = &party.members[i];
        characterInit(c, roster[i].name, roster[i].cls, 100);
        characterEquip(c, SLOT_WEAPON, itemDefFind("trainingSword"));    // 20 base damage
        characterEquip(c, SLOT_MAIN_ARMOR, itemDefFind("trainingArmor")); // 300 armor
    }
}

void partyRestore(void) {
    for (int i = 0; i < party.count; i++) party.members[i].health = party.members[i].maxHealth;
}

int partyLivingCount(const Party *p) {
    int n = 0;
    for (int i = 0; i < p->count; i++)
        if (characterIsAlive(&p->members[i])) n++;
    return n;
}

int partyFirstLiving(const Party *p, int from) {
    for (int i = from < 0 ? 0 : from; i < p->count; i++)
        if (characterIsAlive(&p->members[i])) return i;
    return -1;
}
