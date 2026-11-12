#include "Item.h"

bool Item::isWeapon() const {
    return type == ItemType::WEAPON_PISTOL ||
           type == ItemType::WEAPON_RIFLE ||
           type == ItemType::WEAPON_SNIPER;
}

bool Item::isConsumable() const {
    return type == ItemType::MEDKIT || type == ItemType::STIMPACK;
}

bool Item::isAmmo() const {
    return type == ItemType::AMMO_PISTOL ||
           type == ItemType::AMMO_RIFLE ||
           type == ItemType::AMMO_SNIPER;
}

bool Item::isKey() const {
    return type == ItemType::VAULT_KEY_1 ||
           type == ItemType::VAULT_KEY_2 ||
           type == ItemType::VAULT_KEY_3 ||
           type == ItemType::VAULT_KEY_4;
}

bool Item::isStackable() const {
    return isAmmo() || isConsumable();
}

int Item::getMaxStack() const {
    if (type == ItemType::AMMO_PISTOL) return 120;
    if (type == ItemType::AMMO_RIFLE) return 180;
    if (type == ItemType::AMMO_SNIPER) return 30;
    if (type == ItemType::MEDKIT) return 5;
    if (type == ItemType::STIMPACK) return 10;
    return 1;
}

ItemType Item::getAmmoTypeNeeded() const {
    switch (type) {
        case ItemType::WEAPON_PISTOL: return ItemType::AMMO_PISTOL;
        case ItemType::WEAPON_RIFLE:  return ItemType::AMMO_RIFLE;
        case ItemType::WEAPON_SNIPER: return ItemType::AMMO_SNIPER;
        default: return ItemType::NONE;
    }
}

std::string Item::getName() const {
    switch (type) {
        case ItemType::WEAPON_PISTOL: return "V-9 Tactical Pistol";
        case ItemType::WEAPON_RIFLE:  return "AR-77 Assault Rifle";
        case ItemType::WEAPON_SNIPER: return "Longshot Sniper Rifle";
        case ItemType::AMMO_PISTOL:   return "9mm Pistol Ammo";
        case ItemType::AMMO_RIFLE:    return "5.56mm Rifle Ammo";
        case ItemType::AMMO_SNIPER:   return "High-Caliber Sniper Ammo";
        case ItemType::MEDKIT:        return "Trauma Medkit (+40 HP)";
        case ItemType::STIMPACK:      return "Adrenaline Stimpack (+20 HP)";
        case ItemType::VAULT_KEY_1:   return "Vault 1 Security Key [Cyan]";
        case ItemType::VAULT_KEY_2:   return "Vault 2 Security Key [Orange]";
        case ItemType::VAULT_KEY_3:   return "Vault 3 Security Key [Green]";
        case ItemType::VAULT_KEY_4:   return "Vault 4 Security Key [Purple]";
        default: return "Empty";
    }
}

std::string Item::getShortName() const {
    switch (type) {
        case ItemType::WEAPON_PISTOL: return "PISTOL";
        case ItemType::WEAPON_RIFLE:  return "AR-77";
        case ItemType::WEAPON_SNIPER: return "SNIPER";
        case ItemType::AMMO_PISTOL:   return "9mm";
        case ItemType::AMMO_RIFLE:    return "5.56";
        case ItemType::AMMO_SNIPER:   return ".50 CAL";
        case ItemType::MEDKIT:        return "MEDKIT";
        case ItemType::STIMPACK:      return "STIM";
        case ItemType::VAULT_KEY_1:   return "KEY 1";
        case ItemType::VAULT_KEY_2:   return "KEY 2";
        case ItemType::VAULT_KEY_3:   return "KEY 3";
        case ItemType::VAULT_KEY_4:   return "KEY 4";
        default: return "---";
    }
}

float Item::getFireRate() const {
    switch (type) {
        case ItemType::WEAPON_PISTOL: return 0.22f; // ~4.5 shots/sec
        case ItemType::WEAPON_RIFLE:  return 0.11f; // ~9 shots/sec
        case ItemType::WEAPON_SNIPER: return 0.85f; // Heavy bolt action
        default: return 0.5f;
    }
}

float Item::getReloadTime() const {
    switch (type) {
        case ItemType::WEAPON_PISTOL: return 1.2f;
        case ItemType::WEAPON_RIFLE:  return 1.8f;
        case ItemType::WEAPON_SNIPER: return 2.4f;
        default: return 1.0f;
    }
}

int Item::getDamage() const {
    switch (type) {
        case ItemType::WEAPON_PISTOL: return 32;
        case ItemType::WEAPON_RIFLE:  return 24;
        case ItemType::WEAPON_SNIPER: return 110;
        default: return 0;
    }
}

int Item::getSpriteTextureId() const {
    switch (type) {
        case ItemType::WEAPON_PISTOL: return 102;
        case ItemType::WEAPON_RIFLE:  return 103;
        case ItemType::WEAPON_SNIPER: return 104;
        case ItemType::AMMO_PISTOL:   return 105;
        case ItemType::AMMO_RIFLE:    return 106;
        case ItemType::AMMO_SNIPER:   return 107;
        case ItemType::MEDKIT:        return 100;
        case ItemType::STIMPACK:      return 101;
        case ItemType::VAULT_KEY_1:   return 108;
        case ItemType::VAULT_KEY_2:   return 109;
        case ItemType::VAULT_KEY_3:   return 110;
        case ItemType::VAULT_KEY_4:   return 111;
        default: return 100;
    }
}

Item Item::createPistol(int ammo) {
    Item item;
    item.type = ItemType::WEAPON_PISTOL;
    item.count = 1;
    item.maxClip = 12;
    item.ammoInClip = std::min(ammo, item.maxClip);
    return item;
}

Item Item::createRifle(int ammo) {
    Item item;
    item.type = ItemType::WEAPON_RIFLE;
    item.count = 1;
    item.maxClip = 30;
    item.ammoInClip = std::min(ammo, item.maxClip);
    return item;
}

Item Item::createSniper(int ammo) {
    Item item;
    item.type = ItemType::WEAPON_SNIPER;
    item.count = 1;
    item.maxClip = 5;
    item.ammoInClip = std::min(ammo, item.maxClip);
    return item;
}

Item Item::createPistolAmmo(int count) {
    Item item;
    item.type = ItemType::AMMO_PISTOL;
    item.count = count;
    return item;
}

Item Item::createRifleAmmo(int count) {
    Item item;
    item.type = ItemType::AMMO_RIFLE;
    item.count = count;
    return item;
}

Item Item::createSniperAmmo(int count) {
    Item item;
    item.type = ItemType::AMMO_SNIPER;
    item.count = count;
    return item;
}

Item Item::createMedkit(int count) {
    Item item;
    item.type = ItemType::MEDKIT;
    item.count = count;
    return item;
}

Item Item::createStimpack(int count) {
    Item item;
    item.type = ItemType::STIMPACK;
    item.count = count;
    return item;
}

Item Item::createKey(int vaultNumber) {
    Item item;
    switch (vaultNumber) {
        case 1: item.type = ItemType::VAULT_KEY_1; break;
        case 2: item.type = ItemType::VAULT_KEY_2; break;
        case 3: item.type = ItemType::VAULT_KEY_3; break;
        case 4: item.type = ItemType::VAULT_KEY_4; break;
        default: item.type = ItemType::VAULT_KEY_1; break;
    }
    item.count = 1;
    return item;
}

