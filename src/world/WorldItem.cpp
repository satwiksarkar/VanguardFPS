#include "WorldItem.h"

void WorldItemManager::populateLevel1(std::vector<WorldItem>& items) {
    items.clear();

    // 1. Central Spawn Room (14-18, 14-18)
    items.emplace_back(15.5f, 15.5f, Item::createPistol(12), 0.0f);
    items.emplace_back(17.0f, 15.5f, Item::createPistolAmmo(24), 1.2f);
    items.emplace_back(15.5f, 17.0f, Item::createMedkit(1), 2.4f);
    items.emplace_back(17.0f, 17.0f, Item::createStimpack(2), 3.6f);

    // 2. Sector 1 (NW) - Maintenance & Vault 1
    items.emplace_back(3.5f, 9.5f, Item::createRifle(30), 0.5f);
    items.emplace_back(4.5f, 9.5f, Item::createRifleAmmo(60), 1.8f);
    items.emplace_back(6.0f, 4.0f, Item::createKey(1), 2.9f); // Vault 1 Key
    items.emplace_back(5.0f, 4.0f, Item::createMedkit(1), 4.1f);

    // 3. Sector 2 (NE) - Maintenance & Vault 2
    items.emplace_back(28.5f, 9.5f, Item::createSniper(5), 0.8f);
    items.emplace_back(27.5f, 9.5f, Item::createSniperAmmo(15), 2.1f);
    items.emplace_back(26.0f, 4.0f, Item::createKey(2), 3.4f); // Vault 2 Key
    items.emplace_back(25.0f, 4.0f, Item::createStimpack(2), 4.7f);

    // 4. Sector 3 (SW) - Maintenance & Vault 3
    items.emplace_back(3.5f, 22.5f, Item::createRifleAmmo(60), 1.0f);
    items.emplace_back(4.5f, 22.5f, Item::createMedkit(1), 2.3f);
    items.emplace_back(6.0f, 28.0f, Item::createKey(3), 3.7f); // Vault 3 Key
    items.emplace_back(7.0f, 28.0f, Item::createPistolAmmo(48), 5.0f);

    // 5. Sector 4 (SE) - Maintenance & Vault 4
    items.emplace_back(28.5f, 22.5f, Item::createSniperAmmo(15), 1.3f);
    items.emplace_back(27.5f, 22.5f, Item::createMedkit(1), 2.6f);
    items.emplace_back(26.0f, 28.0f, Item::createKey(4), 4.0f); // Vault 4 Key
    items.emplace_back(25.0f, 28.0f, Item::createStimpack(2), 5.3f);

    // 6. Atmospheric corridor drops
    items.emplace_back(16.0f, 8.0f, Item::createMedkit(1), 0.7f);
    items.emplace_back(16.0f, 20.0f, Item::createStimpack(1), 1.9f);
    items.emplace_back(10.0f, 16.0f, Item::createRifleAmmo(30), 3.1f);
    items.emplace_back(22.0f, 16.0f, Item::createPistolAmmo(24), 4.4f);
}

