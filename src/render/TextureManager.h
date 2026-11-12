#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <unordered_map>

class TextureManager {
public:
    static constexpr int TEX_SIZE = 64;
    static constexpr int TEX_MASK = 63;

    // Special IDs for floor, ceiling, and viewmodels
    static constexpr int TEX_FLOOR = 1000;
    static constexpr int TEX_CEILING = 1001;

    // Item Sprite IDs
    static constexpr int SPRITE_MEDKIT = 100;
    static constexpr int SPRITE_STIMPACK = 101;
    static constexpr int SPRITE_PISTOL = 102;
    static constexpr int SPRITE_RIFLE = 103;
    static constexpr int SPRITE_SNIPER = 104;
    static constexpr int SPRITE_AMMO_PISTOL = 105;
    static constexpr int SPRITE_AMMO_RIFLE = 106;
    static constexpr int SPRITE_AMMO_SNIPER = 107;
    static constexpr int SPRITE_VAULT_KEY_1 = 108;
    static constexpr int SPRITE_VAULT_KEY_2 = 109;
    static constexpr int SPRITE_VAULT_KEY_3 = 110;
    static constexpr int SPRITE_VAULT_KEY_4 = 111;

    // Enemy Sprite IDs
    static constexpr int SPRITE_TASER_DRONE = 120;
    static constexpr int SPRITE_GUN_DRONE = 121;
    static constexpr int SPRITE_TERMINATOR_UNARMED = 122;
    static constexpr int SPRITE_TERMINATOR_ARMED = 123;
    static constexpr int SPRITE_ENEMY_DEAD = 124;

    // Co-op Teammate Survivor Marine Sprite IDs
    static constexpr int SPRITE_SURVIVOR = 130;
    static constexpr int SPRITE_SURVIVOR_RIFLE = 130;
    static constexpr int SPRITE_SURVIVOR_RIFLE_FIRE = 131;
    static constexpr int SPRITE_SURVIVOR_PISTOL = 132;
    static constexpr int SPRITE_SURVIVOR_PISTOL_FIRE = 133;
    static constexpr int SPRITE_SURVIVOR_SNIPER = 134;
    static constexpr int SPRITE_SURVIVOR_DOWNED = 135;
    static constexpr int SPRITE_SURVIVOR_DEAD = 136;

    TextureManager();

    inline uint32_t getPixel(int texId, int u, int v) const {
        auto it = m_textures.find(texId);
        if (it != m_textures.end()) {
            return it->second[(v & TEX_MASK) * TEX_SIZE + (u & TEX_MASK)];
        }
        return m_defaultTexture[(v & TEX_MASK) * TEX_SIZE + (u & TEX_MASK)];
    }

    const std::vector<uint32_t>& getTextureBuffer(int texId) const;

private:
    void generateTextures();
    void generateLabWall(int id);
    void generateHazardWall(int id);
    void generateServerRack(int id);
    void generateConcreteWall(int id);
    void generateVaultDoor(int id, uint32_t accentColor, int vaultNum);
    void generateElectricalBox(int id, int boxNum);
    void generateControlPanel(int id, uint32_t accentColor, int panelNum);
    void generateServerExitDoor(int id);
    void generateFloorTexture();
    void generateCeilingTexture();

    // Item Sprite generation
    void generateMedkitSprite(int id);
    void generateStimpackSprite(int id);
    void generatePistolSprite(int id);
    void generateRifleSprite(int id);
    void generateSniperSprite(int id);
    void generateAmmoBoxSprite(int id, uint32_t boxColor, const char* label);
    void generateKeycardSprite(int id, uint32_t glowColor);

    // Enemy Sprite generation (Alpha 254 = Emissive / Self-illuminated)
    void generateTaserDroneSprite(int id);
    void generateGunDroneSprite(int id);
    void generateTerminatorSprite(int id, bool armed);
    void generateEnemyDeadSprite(int id);

    // Co-op Survivor Marine Sprite generation
    void generateSurvivorSprite(int id);
    void generateSurvivorRifleSprite(int id, bool firing);
    void generateSurvivorPistolSprite(int id, bool firing);
    void generateSurvivorSniperSprite(int id);
    void generateSurvivorDownedSprite(int id);
    void generateSurvivorDeadSprite(int id);

    static inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               static_cast<uint32_t>(b);
    }

    // Emissive helper (Alpha = 254)
    static inline uint32_t emissive(uint8_t r, uint8_t g, uint8_t b) {
        return rgba(r, g, b, 254);
    }

    std::unordered_map<int, std::vector<uint32_t>> m_textures;
    std::vector<uint32_t> m_defaultTexture;
};

#endif // TEXTURE_MANAGER_H
