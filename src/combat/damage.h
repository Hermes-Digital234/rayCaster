// damage.h - the damage rules shared by everyone who fights. Pure math: no drawing, no input, no state.
//
// ARMOR works like "effective health", not like a flat cut that scales up forever:
//
//     damage reduction = armor / (armor + 300)
//     effective health = health * (armor + 300) / 300        (the same thing, seen from the health side)
//
//      armor |  300 |  600 |  900 | 1200
//  reduction |  50% |  67% |  75% |  80%
//
// Every point of armor adds the same amount of effective health (health/300 per point), so armor stays
// worth buying without ever reaching 100%. Players and enemies use the same formula.
// Armor below 0 counts as 0. A hit that does any damage at all always does at least 1.
#ifndef DAMAGE_H
#define DAMAGE_H

#define ARMOR_CONSTANT 300.0f

// Fraction of damage blocked by armor, from 0.0 up to (but never including) 1.0.
float armorDamageReduction(float armor);

// How much damage `health` can really absorb with this armor.
float armorEffectiveHealth(float health, float armor);

// Damage actually dealt after armor, rounded to the nearest whole point, minimum 1 (0 stays 0).
int damageAfterArmor(float damage, float armor);

#endif
