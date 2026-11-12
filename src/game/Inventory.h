#ifndef INVENTORY_H
#define INVENTORY_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include "Item.h"

class Inventory {
public:
    static constexpr int SLOT_COUNT = 9;

    Inventory();

    int getActiveSlot() const { return m_activeSlot; }
    void setActiveSlot(int slot);
    void selectNextSlot();
    void selectPrevSlot();

    Item& getActiveItem() { return m_slots[m_activeSlot]; }
    const Item& getActiveItem() const { return m_slots[m_activeSlot]; }

    Item& getItem(int index) { return m_slots[std::clamp(index, 0, SLOT_COUNT - 1)]; }
    const Item& getItem(int index) const { return m_slots[std::clamp(index, 0, SLOT_COUNT - 1)]; }

    // Add item to inventory (auto-stacks if stackable, otherwise finds empty slot)
    bool addItem(Item item);

    // Remove or drop items
    bool removeItem(int slot, int count = 1);
    Item dropActiveItem();

    // Ammo utilities
    int getTotalReserveAmmo(ItemType ammoType) const;
    int consumeReserveAmmo(ItemType ammoType, int amountNeeded);

    // Keycard tracking
    bool hasKey(int vaultNumber) const;
    int getKeyCount() const;

private:
    std::array<Item, SLOT_COUNT> m_slots;
    int m_activeSlot = 0;
};

#endif // INVENTORY_H

