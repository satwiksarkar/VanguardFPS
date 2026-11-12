#ifndef RASTER_RENDERER_H
#define RASTER_RENDERER_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <QImage>
#include <QString>
#include "TextureManager.h"
#include "net/NetProtocol.h"

class Player;
class Map;
struct WorldItem;
struct Item;
class Enemy;
class TaskManager;

struct ProjectedPlayerHud {
    quint8 id = 0;
    QString name;
    qint32 health = 100;
    int screenX = 0;
    int headScreenY = 0;
    bool isVisible = false;
    bool isDowned = false;
};

class RasterRenderer {
public:
    static constexpr int SCREEN_WIDTH = 640;
    static constexpr int SCREEN_HEIGHT = 360;

    RasterRenderer();

    // Main render entrypoint: renders 3D world, depth-sorted items, enemies & remote co-op players, first-person viewmodel, and screen FX
    void render(const Player& player,
                const Map& map,
                const std::vector<WorldItem>& items,
                const std::vector<Enemy>& enemies,
                const std::vector<NetPlayerState>& remotePlayers,
                const Item& activeItem,
                float timeSeconds,
                float fireCooldownTimer,
                float muzzleFlashTimer,
                float reloadTimer,
                float damageFlashTimer = 0.0f,
                float taserShockTimer = 0.0f,
                float healPulseTimer = 0.0f,
                const TaskManager* taskManager = nullptr);

    // Get rendered frame buffer
    const QImage& getFrameBuffer() const { return m_frameBuffer; }

    // 1D Depth buffer (perpendicular wall distances)
    const float* getZBuffer() const { return m_zBuffer.data(); }

    const TextureManager& getTextureManager() const { return m_textureManager; }

    // Remote players projected onto screen for HUD nameplates and HP bars
    const std::vector<ProjectedPlayerHud>& getProjectedPlayers() const { return m_projectedPlayers; }

    // Full-screen raster status effects (Red damage vignette, Cyan taser jitter, Green healing pulse)
    void applyScreenEffects(float damageFlashTimer, float taserShockTimer, float healPulseTimer, float timeSeconds);

private:
    void renderFloorAndCeiling(const Player& player, const Map& map, const std::vector<NetPlayerState>& remotePlayers, float timeSeconds);
    void renderWalls(const Player& player, const Map& map, const std::vector<NetPlayerState>& remotePlayers, const TaskManager* taskManager, float timeSeconds);
    void renderEntities(const Player& player,
                        const Map& map,
                        const std::vector<WorldItem>& items,
                        const std::vector<Enemy>& enemies,
                        const std::vector<NetPlayerState>& remotePlayers,
                        float timeSeconds);

    void renderWeaponViewModel(const Item& activeItem,
                               float fireCooldownTimer,
                               float muzzleFlashTimer,
                               float reloadTimer,
                               bool isScoped,
                               float timeSeconds);
    void renderSniperScopeOverlay(const Player& player, float timeSeconds);

    static inline uint32_t applyLighting(uint32_t color, float light, bool isRestoredSector = false) {
        uint32_t r = (color >> 16) & 0xFF;
        uint32_t g = (color >> 8)  & 0xFF;
        uint32_t b = color         & 0xFF;

        if (isRestoredSector) {
            // Warm golden amber glow from restored electrical power (as seen in image_6.png)
            r = static_cast<uint32_t>(std::min(255.0f, r * light * 1.25f + 16.0f * light));
            g = static_cast<uint32_t>(std::min(255.0f, g * light * 1.08f + 8.0f * light));
            b = static_cast<uint32_t>(std::min(255.0f, b * light * 0.72f));
        } else {
            r = static_cast<uint32_t>(std::min(255.0f, r * light));
            g = static_cast<uint32_t>(std::min(255.0f, g * light));
            b = static_cast<uint32_t>(std::min(255.0f, b * light));
        }

        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    QImage m_frameBuffer;
    std::vector<float> m_zBuffer;
    TextureManager m_textureManager;
    std::vector<ProjectedPlayerHud> m_projectedPlayers;
};

#endif // RASTER_RENDERER_H
