#ifndef WORLD_ITEM_H
#define WORLD_ITEM_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include "game/Item.h"

struct WorldItem {
    float x = 0.0f;
    float y = 0.0f;
    Item item;
    float bobPhase = 0.0f;

    WorldItem() = default;
    WorldItem(float posX, float posY, const Item& it, float phase = 0.0f)
        : x(posX), y(posY), item(it), bobPhase(phase) {}

    float distanceTo(float px, float py) const {
        float dx = x - px;
        float dy = y - py;
        return std::sqrt(dx * dx + dy * dy);
    }
};

class WorldItemManager {
public:
    static void populateLevel1(std::vector<WorldItem>& items);
};

#endif // WORLD_ITEM_H

