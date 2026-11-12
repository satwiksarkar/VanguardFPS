#include "Inventory.h"

Inventory::Inventory() {
    // Start with empty slots
    for (auto& slot : m_slots) {
        slot = Item();
    }
}

void Inventory::setActiveSlot(int slot) {
    if (slot >= 0 && slot < SLOT_COUNT) {
        m_activeSlot = slot;
    }
}

void Inventory::selectNextSlot() {
    m_activeSlot = (m_activeSlot + 1) % SLOT_COUNT;
}

void Inventory::selectPrevSlot() {
    m_activeSlot = (m_activeSlot - 1 + SLOT_COUNT) % SLOT_COUNT;
}

bool Inventory::addItem(Item item) {
    if (item.isEmpty()) return false;

    // 1. If item is stackable, attempt to merge with existing stacks
    if (item.isStackable()) {
        for (int i = 0; i < SLOT_COUNT; ++i) {
            if (m_slots[i].type == item.type) {
                int space = m_slots[i].getMaxStack() - m_slots[i].count;
                if (space > 0) {
                    int toAdd = std::min(space, item.count);
                    m_slots[i].count += toAdd;
                    item.count -= toAdd;
                    if (item.count <= 0) {
                        return true;
                    }
                }
            }
        }
    }

    // 2. Put remaining item in first empty slot
    if (item.count > 0 || item.isWeapon()) {
        for (int i = 0; i < SLOT_COUNT; ++i) {
            if (m_slots[i].isEmpty()) {
                m_slots[i] = item;
                return true;
            }
        }
    }

    // Inventory full
    return false;
}

bool Inventory::removeItem(int slot, int count) {
    if (slot < 0 || slot >= SLOT_COUNT) return false;
    if (m_slots[slot].isEmpty()) return false;

    if (m_slots[slot].isStackable()) {
        m_slots[slot].count -= count;
        if (m_slots[slot].count <= 0) {
            m_slots[slot] = Item();
        }
        return true;
    } else {
        m_slots[slot] = Item();
        return true;
    }
}

Item Inventory::dropActiveItem() {
    Item item = m_slots[m_activeSlot];
    m_slots[m_activeSlot] = Item();
    return item;
}

int Inventory::getTotalReserveAmmo(ItemType ammoType) const {
    int total = 0;
    for (const auto& slot : m_slots) {
        if (slot.type == ammoType) {
            total += slot.count;
        }
    }
    return total;
}

int Inventory::consumeReserveAmmo(ItemType ammoType, int amountNeeded) {
    int remainingToConsume = amountNeeded;
    int totalConsumed = 0;

    for (auto& slot : m_slots) {
        if (slot.type == ammoType && slot.count > 0) {
            int toTake = std::min(slot.count, remainingToConsume);
            slot.count -= toTake;
            remainingToConsume -= toTake;
            totalConsumed += toTake;

            if (slot.count <= 0) {
                slot = Item();
            }

            if (remainingToConsume <= 0) {
                break;
            }
        }
    }

    return totalConsumed;
}

bool Inventory::hasKey(int vaultNumber) const {
    ItemType target = ItemType::NONE;
    switch (vaultNumber) {
        case 1: target = ItemType::VAULT_KEY_1; break;
        case 2: target = ItemType::VAULT_KEY_2; break;
        case 3: target = ItemType::VAULT_KEY_3; break;
        case 4: target = ItemType::VAULT_KEY_4; break;
        default: return false;
    }

    for (const auto& slot : m_slots) {
        if (slot.type == target) return true;
    }
    return false;
}

int Inventory::getKeyCount() const {
    int count = 0;
    for (int k = 1; k <= 4; ++k) {
        if (hasKey(k)) count++;
    }
    return count;
}

