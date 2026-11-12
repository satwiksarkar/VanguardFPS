#ifndef ITEM_H
#define ITEM_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <string>

enum class ItemType : uint8_t {
    NONE = 0,
    WEAPON_PISTOL = 1,
    WEAPON_RIFLE = 2,
    WEAPON_SNIPER = 3,
    AMMO_PISTOL = 10,
    AMMO_RIFLE = 11,
    AMMO_SNIPER = 12,
    MEDKIT = 20,
    STIMPACK = 21,
    VAULT_KEY_1 = 30,
    VAULT_KEY_2 = 31,
    VAULT_KEY_3 = 32,
    VAULT_KEY_4 = 33
};

struct Item {
    ItemType type = ItemType::NONE;
    int count = 0;       // Stacking count for ammo & consumables
    int ammoInClip = 0;  // Rounds currently in the magazine
    int maxClip = 0;     // Magazine capacity for weapons

    Item() = default;
    Item(ItemType t, int c = 1, int ammo = 0, int clip = 0)
        : type(t), count(c), ammoInClip(ammo), maxClip(clip) {}

    bool isEmpty() const { return type == ItemType::NONE || (count <= 0 && !isWeapon()); }
    bool isWeapon() const;
    bool isConsumable() const;
    bool isAmmo() const;
    bool isKey() const;
    bool isStackable() const;

    int getMaxStack() const;
    ItemType getAmmoTypeNeeded() const;
    std::string getName() const;
    std::string getShortName() const;

    float getFireRate() const;   // Cooldown in seconds between shots
    float getReloadTime() const; // Reload duration in seconds
    int getDamage() const;
    int getSpriteTextureId() const;

    // Factory helpers
    static Item createPistol(int ammo = 12);
    static Item createRifle(int ammo = 30);
    static Item createSniper(int ammo = 5);
    static Item createPistolAmmo(int count = 24);
    static Item createRifleAmmo(int count = 30);
    static Item createSniperAmmo(int count = 5);
    static Item createMedkit(int count = 1);
    static Item createStimpack(int count = 1);
    static Item createKey(int vaultNumber);
};

#endif // ITEM_H

