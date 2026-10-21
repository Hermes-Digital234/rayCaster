#include "combat/damage.h"
#include <math.h>

float armorDamageReduction(float armor) {
    if (armor <= 0.0f) return 0.0f;
    return armor / (armor + ARMOR_CONSTANT);
}

float armorEffectiveHealth(float health, float armor) {
    if (armor <= 0.0f) return health;
    return health * (armor + ARMOR_CONSTANT) / ARMOR_CONSTANT;
}

int damageAfterArmor(float damage, float armor) {
    if (damage <= 0.0f) return 0;
    int dealt = (int)lroundf(damage * (1.0f - armorDamageReduction(armor)));
    return dealt < 1 ? 1 : dealt;
}
