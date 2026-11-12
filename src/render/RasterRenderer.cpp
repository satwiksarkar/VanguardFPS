#include "RasterRenderer.h"
#include "game/Player.h"
#include "game/Item.h"
#include "game/Enemy.h"
#include "world/Map.h"
#include "world/WorldItem.h"
#include "world/TaskManager.h"
#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <qrgb.h>

RasterRenderer::RasterRenderer()
    : m_frameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, QImage::Format_ARGB32_Premultiplied)
    , m_zBuffer(SCREEN_WIDTH, 1000.0f)
{
}

void RasterRenderer::render(const Player& player,
                            const Map& map,
                            const std::vector<WorldItem>& items,
                            const std::vector<Enemy>& enemies,
                            const std::vector<NetPlayerState>& remotePlayers,
                            const Item& activeItem,
                            float timeSeconds,
                            float fireCooldownTimer,
                            float muzzleFlashTimer,
                            float reloadTimer,
                            float damageFlashTimer,
                            float taserShockTimer,
                            float healPulseTimer,
                            const TaskManager* taskManager)
{
    // 1. Render floor and ceiling
    renderFloorAndCeiling(player, map, remotePlayers, timeSeconds);

    // 2. Render walls using DDA raycasting across all columns (fills m_zBuffer)
    renderWalls(player, map, remotePlayers, taskManager, timeSeconds);

    // 3. Render depth-sorted entities (world items, enemies, and remote co-op teammates)
    renderEntities(player, map, items, enemies, remotePlayers, timeSeconds);

    // 4. Render first-person viewmodel & muzzle flash or sniper scope (hidden when downed)
    if (!player.isDowned()) {
        if (player.isScoped() && activeItem.type == ItemType::WEAPON_SNIPER) {
            renderSniperScopeOverlay(player, timeSeconds);
        } else {
            renderWeaponViewModel(activeItem, fireCooldownTimer, muzzleFlashTimer, reloadTimer, player.isScoped(), timeSeconds);
        }
    }

    // 5. Full-screen raster status effects (Red damage vignette, Cyan taser jitter, Green healing pulse)
    applyScreenEffects(damageFlashTimer, taserShockTimer, healPulseTimer, timeSeconds);
}

void RasterRenderer::renderFloorAndCeiling(const Player& player, const Map& map, const std::vector<NetPlayerState>& remotePlayers, float timeSeconds) {
    uint32_t* scanlines = reinterpret_cast<uint32_t*>(m_frameBuffer.bits());

    float posX = player.getX();
    float posY = player.getY();
    float dirX = player.getDirX();
    float dirY = player.getDirY();
    float planeX = player.getPlaneX();
    float planeY = player.getPlaneY();
    bool playerFlashlight = player.hasFlashlight();

    const int horizon = SCREEN_HEIGHT / 2 + (player.isDowned() ? 45 : 0);

    for (int y = horizon; y < SCREEN_HEIGHT; ++y) {
        int yCeil = SCREEN_HEIGHT - 1 - (y - (player.isDowned() ? 45 : 0));
        if (yCeil < 0) yCeil = 0;
        if (yCeil >= SCREEN_HEIGHT) yCeil = SCREEN_HEIGHT - 1;

        float p = static_cast<float>(y - horizon);
        if (p < 0.1f) p = 0.1f;

        float posZ = 0.5f * static_cast<float>(SCREEN_HEIGHT);
        float rowDist = posZ / p;

        float rayDirX0 = dirX - planeX;
        float rayDirY0 = dirY - planeY;
        float rayDirX1 = dirX + planeX;
        float rayDirY1 = dirY + planeY;

        float floorStepX = rowDist * (rayDirX1 - rayDirX0) / static_cast<float>(SCREEN_WIDTH);
        float floorStepY = rowDist * (rayDirY1 - rayDirY0) / static_cast<float>(SCREEN_WIDTH);

        float floorX = posX + rowDist * rayDirX0;
        float floorY = posY + rowDist * rayDirY0;

        float rowFog = 1.0f / (1.0f + 0.12f * rowDist + 0.032f * rowDist * rowDist);
        float playerAmbient = std::max(0.0f, 1.0f - rowDist / 4.8f) * 0.22f;

        uint32_t* floorRow = &scanlines[y * SCREEN_WIDTH];
        uint32_t* ceilRow  = &scanlines[yCeil * SCREEN_WIDTH];

        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int cellX = static_cast<int>(std::floor(floorX));
            int cellY = static_cast<int>(std::floor(floorY));

            int tx = static_cast<int>(64.0f * (floorX - static_cast<float>(cellX))) & 63;
            int ty = static_cast<int>(64.0f * (floorY - static_cast<float>(cellY))) & 63;

            float tileLight = map.getLightIntensity(cellX, cellY, timeSeconds);
            LightState lightState = map.getLightState(cellX, cellY);
            bool isRestored = (lightState == LightState::BRIGHT_RESTORED);

            // Flashlight contribution from local player
            float flashLight = 0.0f;
            if (playerFlashlight && rowDist < 9.0f) {
                float fdx = floorX - posX;
                float fdy = floorY - posY;
                float flDist = std::sqrt(fdx * fdx + fdy * fdy);
                if (flDist > 0.05f) {
                    float fDot = (fdx * dirX + fdy * dirY) / flDist;
                    if (fDot > 0.84f) {
                        float cone = (fDot - 0.84f) / 0.16f;
                        float distF = (1.0f - flDist / 9.0f);
                        flashLight += cone * distF * 0.70f;
                    }
                }
            }

            // Teammates' flashlight contribution
            for (const auto& rp : remotePlayers) {
                if (!rp.isAlive || rp.isDowned) continue;
                float rdx = floorX - rp.posX;
                float rdy = floorY - rp.posY;
                float rDistSq = rdx * rdx + rdy * rdy;
                if (rDistSq < 64.0f) {
                    float rDist = std::sqrt(rDistSq);
                    if (rDist > 0.05f) {
                        float rDot = (rdx * rp.dirX + rdy * rp.dirY) / rDist;
                        if (rDot > 0.84f) {
                            float rCone = (rDot - 0.84f) / 0.16f;
                            float rDistF = (1.0f - rDist / 8.0f);
                            flashLight += rCone * rDistF * 0.55f;
                        }
                    }
                }
            }

            float floorLight = std::min(1.0f, (tileLight + playerAmbient) * rowFog + flashLight);

            uint32_t floorPixel = m_textureManager.getPixel(TextureManager::TEX_FLOOR, tx, ty);
            floorRow[x] = applyLighting(floorPixel, floorLight, isRestored);

            uint32_t ceilPixel = m_textureManager.getPixel(TextureManager::TEX_CEILING, tx, ty);

            if (lightState != LightState::OFF) {
                float dx = static_cast<float>(tx) - 31.5f;
                float dy = static_cast<float>(ty) - 31.5f;
                float bulbDistSq = dx * dx + dy * dy;

                if (bulbDistSq <= 18.0f) {
                    float bulbBrightness = (lightState == LightState::FLICKERING)
                        ? (tileLight * 3.2f)
                        : (isRestored ? 1.35f : 1.1f);
                    bulbBrightness = std::min(1.0f, bulbBrightness * rowFog);

                    uint8_t br = static_cast<uint8_t>(isRestored ? (255.0f * bulbBrightness) : (245.0f * bulbBrightness));
                    uint8_t bg = static_cast<uint8_t>(isRestored ? (230.0f * bulbBrightness) : (250.0f * bulbBrightness));
                    uint8_t bb = static_cast<uint8_t>(isRestored ? (160.0f * bulbBrightness) : (255.0f * bulbBrightness));
                    ceilPixel = 0xFF000000 | (br << 16) | (bg << 8) | bb;
                } else if (bulbDistSq <= 50.0f) {
                    float haloFactor = 1.0f - (bulbDistSq - 18.0f) / 32.0f;
                    float haloLight = std::min(1.0f, (tileLight * 1.5f * haloFactor + playerAmbient) * rowFog + flashLight * 0.5f);
                    ceilPixel = applyLighting(ceilPixel, haloLight + 0.15f * haloFactor, isRestored);
                } else {
                    ceilPixel = applyLighting(ceilPixel, std::min(1.0f, (tileLight + playerAmbient) * rowFog + flashLight * 0.5f), isRestored);
                }
            } else {
                float ceilLight = std::min(1.0f, (tileLight * 0.85f + playerAmbient) * rowFog + flashLight * 0.5f);
                ceilPixel = applyLighting(ceilPixel, ceilLight, isRestored);
            }

            ceilRow[x] = ceilPixel;

            floorX += floorStepX;
            floorY += floorStepY;
        }
    }
}

void RasterRenderer::renderWalls(const Player& player, const Map& map, const std::vector<NetPlayerState>& remotePlayers, const TaskManager* taskManager, float timeSeconds) {
    uint32_t* scanlines = reinterpret_cast<uint32_t*>(m_frameBuffer.bits());

    float posX = player.getX();
    float posY = player.getY();
    float dirX = player.getDirX();
    float dirY = player.getDirY();
    float planeX = player.getPlaneX();
    float planeY = player.getPlaneY();
    bool playerFlashlight = player.hasFlashlight();

    for (int x = 0; x < SCREEN_WIDTH; ++x) {
        float cameraX = 2.0f * static_cast<float>(x) / static_cast<float>(SCREEN_WIDTH) - 1.0f;
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        int mapX = static_cast<int>(std::floor(posX));
        int mapY = static_cast<int>(std::floor(posY));

        float deltaDistX = (rayDirX == 0.0f) ? 1e30f : std::abs(1.0f / rayDirX);
        float deltaDistY = (rayDirY == 0.0f) ? 1e30f : std::abs(1.0f / rayDirY);

        float sideDistX;
        float sideDistY;
        int stepX;
        int stepY;

        if (rayDirX < 0.0f) {
            stepX = -1;
            sideDistX = (posX - static_cast<float>(mapX)) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (static_cast<float>(mapX) + 1.0f - posX) * deltaDistX;
        }

        if (rayDirY < 0.0f) {
            stepY = -1;
            sideDistY = (posY - static_cast<float>(mapY)) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (static_cast<float>(mapY) + 1.0f - posY) * deltaDistY;
        }

        int hitTile = 0;
        int side = 0;

        while (hitTile == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            if (map.isWall(mapX, mapY)) {
                hitTile = map.getTile(mapX, mapY);
            }
        }

        float perpWallDist;
        if (side == 0) {
            perpWallDist = (static_cast<float>(mapX) - posX + static_cast<float>(1 - stepX) / 2.0f) / rayDirX;
        } else {
            perpWallDist = (static_cast<float>(mapY) - posY + static_cast<float>(1 - stepY) / 2.0f) / rayDirY;
        }

        if (perpWallDist < 0.05f) perpWallDist = 0.05f;

        m_zBuffer[x] = perpWallDist;

        int lineHeight = static_cast<int>(static_cast<float>(SCREEN_HEIGHT) / perpWallDist);

        int horizon = SCREEN_HEIGHT / 2 + (player.isDowned() ? 45 : 0);
        int drawStart = -lineHeight / 2 + horizon;
        int drawEnd   = lineHeight / 2 + horizon;

        // Mechanical sliding door animation
        if (hitTile >= 10 && hitTile <= 13 && taskManager != nullptr) {
            int sec = TaskManager::tileToSector(hitTile);
            if (taskManager->isDoorSliding(sec)) {
                float slide = taskManager->getDoorSlideProgress(sec);
                int slideShift = static_cast<int>(slide * static_cast<float>(lineHeight));
                drawStart -= slideShift;
                drawEnd -= slideShift;
            }
        }

        int clampDrawStart = std::max(0, drawStart);
        int clampDrawEnd   = std::min(SCREEN_HEIGHT - 1, drawEnd);

        float wallX;
        if (side == 0) {
            wallX = posY + perpWallDist * rayDirY;
        } else {
            wallX = posX + perpWallDist * rayDirX;
        }
        wallX -= std::floor(wallX);

        int texX = static_cast<int>(wallX * 64.0f);
        if (side == 0 && rayDirX > 0.0f) {
            texX = 63 - texX;
        }
        if (side == 1 && rayDirY < 0.0f) {
            texX = 63 - texX;
        }

        int facingCellX = (side == 0) ? (mapX - stepX) : mapX;
        int facingCellY = (side == 1) ? (mapY - stepY) : mapY;
        float roomLight = map.getLightIntensity(facingCellX, facingCellY, timeSeconds);
        bool isRestored = (map.getLightState(facingCellX, facingCellY) == LightState::BRIGHT_RESTORED);

        // Flashlight cone from local player
        float flashAngle = (rayDirX * dirX + rayDirY * dirY) / std::sqrt(rayDirX * rayDirX + rayDirY * rayDirY);
        float flashBoost = 0.0f;
        if (playerFlashlight && flashAngle > 0.84f && perpWallDist < 9.0f) {
            float cone = (flashAngle - 0.84f) / 0.16f;
            float distF = (1.0f - perpWallDist / 9.0f);
            flashBoost = cone * distF * 0.75f;
        }

        // Teammates' flashlight on wall point
        if (!remotePlayers.empty()) {
            float wallWorldX = (side == 0) ? (static_cast<float>(mapX) + (stepX > 0 ? 0.0f : 1.0f)) : (posX + perpWallDist * rayDirX);
            float wallWorldY = (side == 1) ? (static_cast<float>(mapY) + (stepY > 0 ? 0.0f : 1.0f)) : (posY + perpWallDist * rayDirY);

            for (const auto& rp : remotePlayers) {
                if (!rp.isAlive || rp.isDowned) continue;
                float rdx = wallWorldX - rp.posX;
                float rdy = wallWorldY - rp.posY;
                float rDistSq = rdx * rdx + rdy * rdy;
                if (rDistSq < 64.0f) {
                    float rDist = std::sqrt(rDistSq);
                    if (rDist > 0.05f) {
                        float rDot = (rdx * rp.dirX + rdy * rp.dirY) / rDist;
                        if (rDot > 0.84f) {
                            float rCone = (rDot - 0.84f) / 0.16f;
                            float rDistF = (1.0f - rDist / 8.0f);
                            flashBoost += rCone * rDistF * 0.60f;
                        }
                    }
                }
            }
        }

        float playerLight = std::max(0.0f, 1.0f - perpWallDist / 5.2f) * 0.28f;
        float fogFactor = 1.0f / (1.0f + 0.12f * perpWallDist + 0.032f * perpWallDist * perpWallDist);
        float sideShading = (side == 1) ? 0.75f : 1.0f;
        float wallLight = std::min(1.0f, (roomLight + playerLight) * fogFactor * sideShading + flashBoost);

        float step = 64.0f / static_cast<float>(lineHeight);
        float texPos = (static_cast<float>(clampDrawStart) - static_cast<float>(drawStart)) * step;

        for (int y = clampDrawStart; y <= clampDrawEnd; ++y) {
            int texY = static_cast<int>(texPos) & 63;
            texPos += step;

            uint32_t color = m_textureManager.getPixel(hitTile, texX, texY);
            uint32_t litColor = applyLighting(color, wallLight, isRestored);

            scanlines[y * SCREEN_WIDTH + x] = litColor;
        }
    }
}

void RasterRenderer::renderEntities(const Player& player,
                                    const Map& map,
                                    const std::vector<WorldItem>& items,
                                    const std::vector<Enemy>& enemies,
                                    const std::vector<NetPlayerState>& remotePlayers,
                                    float timeSeconds)
{
    m_projectedPlayers.clear();

    uint32_t* scanlines = reinterpret_cast<uint32_t*>(m_frameBuffer.bits());

    float posX = player.getX();
    float posY = player.getY();
    float dirX = player.getDirX();
    float dirY = player.getDirY();
    float planeX = player.getPlaneX();
    float planeY = player.getPlaneY();

    struct UnifiedSprite {
        float x;
        float y;
        int textureId;
        float vOffset;
        bool isHurt;
        float distSq;
        quint8 playerId;
        QString playerName;
        qint32 playerHealth;
        bool isDowned;
    };

    std::vector<UnifiedSprite> spriteList;
    spriteList.reserve(items.size() + enemies.size() + remotePlayers.size());

    // 1. Collect World Items
    for (const auto& it : items) {
        float dx = it.x - posX;
        float dy = it.y - posY;
        float vOffset = 0.28f + std::sin(timeSeconds * 3.5f + it.bobPhase) * 0.06f;
        spriteList.push_back({ it.x, it.y, it.item.getSpriteTextureId(), vOffset, false, dx * dx + dy * dy, 0xFF, "", 0, false });
    }

    // 2. Collect Enemies
    for (const auto& enemy : enemies) {
        float dx = enemy.getX() - posX;
        float dy = enemy.getY() - posY;

        float vOffset;
        if (!enemy.isAlive()) {
            vOffset = 0.42f; // Dead scrap lies on the ground
        } else if (enemy.getType() == EnemyType::TASER_DRONE || enemy.getType() == EnemyType::GUN_DRONE) {
            vOffset = 0.12f + std::sin(timeSeconds * 5.0f + enemy.getX()) * 0.04f; // Hovering
        } else {
            vOffset = 0.06f; // Bipedal endoskeleton stands at eye level
        }

        spriteList.push_back({ enemy.getX(), enemy.getY(), enemy.getSpriteTextureId(), vOffset, enemy.isHurt(), dx * dx + dy * dy, 0xFF, "", 0, false });
    }

    // 3. Collect Remote Co-op Teammates with dynamic humanoid marine sprites
    for (const auto& rp : remotePlayers) {
        float dx = rp.posX - posX;
        float dy = rp.posY - posY;
        float vOffset = 0.06f;
        int texId = TextureManager::SPRITE_SURVIVOR_RIFLE;

        if (!rp.isAlive) {
            texId = TextureManager::SPRITE_SURVIVOR_DEAD;
            vOffset = 0.42f;
        } else if (rp.isDowned) {
            texId = TextureManager::SPRITE_SURVIVOR_DOWNED;
            vOffset = 0.28f;
        } else {
            ItemType wType = static_cast<ItemType>(rp.activeItemType);
            if (wType == ItemType::WEAPON_PISTOL) {
                texId = TextureManager::SPRITE_SURVIVOR_PISTOL;
            } else if (wType == ItemType::WEAPON_SNIPER) {
                texId = TextureManager::SPRITE_SURVIVOR_SNIPER;
            } else {
                texId = TextureManager::SPRITE_SURVIVOR_RIFLE;
            }
        }
        spriteList.push_back({ rp.posX, rp.posY, texId, vOffset, false, dx * dx + dy * dy, rp.id, rp.name, rp.health, rp.isDowned });
    }

    // Sort unified sprites from farthest to closest (Painter's algorithm)
    std::sort(spriteList.begin(), spriteList.end(), [](const UnifiedSprite& a, const UnifiedSprite& b) {
        return a.distSq > b.distSq;
    });

    float invDet = 1.0f / (planeX * dirY - dirX * planeY);
    int horizon = SCREEN_HEIGHT / 2 + (player.isDowned() ? 45 : 0);

    for (const auto& sp : spriteList) {
        float spriteX = sp.x - posX;
        float spriteY = sp.y - posY;

        // Transform with inverse camera matrix
        float transformX = invDet * (dirY * spriteX - dirX * spriteY);
        float transformY = invDet * (-planeY * spriteX + planeX * spriteY);

        if (transformY <= 0.15f) continue; // Behind player

        int spriteScreenX = static_cast<int>((SCREEN_WIDTH / 2.0f) * (1.0f + transformX / transformY));
        int vMoveScreen = static_cast<int>(sp.vOffset * static_cast<float>(SCREEN_HEIGHT) / transformY);

        int spriteHeight = std::abs(static_cast<int>(static_cast<float>(SCREEN_HEIGHT) / transformY));
        int spriteWidth  = spriteHeight;

        int drawStartY = -spriteHeight / 2 + horizon + vMoveScreen;
        int drawEndY   = spriteHeight / 2 + horizon + vMoveScreen;
        int clampDrawStartY = std::max(0, drawStartY);
        int clampDrawEndY   = std::min(SCREEN_HEIGHT - 1, drawEndY);

        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX   = spriteWidth / 2 + spriteScreenX;
        int clampDrawStartX = std::max(0, drawStartX);
        int clampDrawEndX   = std::min(SCREEN_WIDTH - 1, drawEndX);

        // Record screen position for HUD nameplates and HP bars
        if (sp.playerId != 0xFF) {
            int testStripe = std::clamp(spriteScreenX, 0, SCREEN_WIDTH - 1);
            bool isVisible = (spriteScreenX >= -40 && spriteScreenX <= SCREEN_WIDTH + 40 &&
                              transformY > 0.15f && transformY <= m_zBuffer[testStripe] + 0.35f);
            m_projectedPlayers.push_back({ sp.playerId, sp.playerName, sp.playerHealth, spriteScreenX, drawStartY, isVisible, sp.isDowned });
        }

        // Lighting calculation
        int cellX = static_cast<int>(std::floor(sp.x));
        int cellY = static_cast<int>(std::floor(sp.y));
        float roomLight = map.getLightIntensity(cellX, cellY, timeSeconds);
        bool isRestored = (map.getLightState(cellX, cellY) == LightState::BRIGHT_RESTORED);

        // Flashlight illumination onto sprite
        float flashBoost = 0.0f;
        if (player.hasFlashlight() && transformY < 9.0f) {
            float fdx = sp.x - posX;
            float fdy = sp.y - posY;
            float flDist = std::sqrt(fdx * fdx + fdy * fdy);
            if (flDist > 0.05f) {
                float fDot = (fdx * dirX + fdy * dirY) / flDist;
                if (fDot > 0.84f) {
                    float cone = (fDot - 0.84f) / 0.16f;
                    float distF = (1.0f - flDist / 9.0f);
                    flashBoost = cone * distF * 0.75f;
                }
            }
        }

        float playerLight = std::max(0.0f, 1.0f - transformY / 5.2f) * 0.28f;
        float fogFactor = 1.0f / (1.0f + 0.12f * transformY + 0.032f * transformY * transformY);
        float spriteLight = std::min(1.0f, (roomLight + playerLight) * fogFactor + flashBoost);

        for (int stripe = clampDrawStartX; stripe <= clampDrawEndX; ++stripe) {
            // Depth check against 1D wall zBuffer
            if (transformY > 0.0f && stripe >= 0 && stripe < SCREEN_WIDTH && transformY < m_zBuffer[stripe]) {
                int texX = static_cast<int>((stripe - drawStartX) * 64 / spriteWidth);

                for (int y = clampDrawStartY; y <= clampDrawEndY; ++y) {
                    int d = y - vMoveScreen - SCREEN_HEIGHT / 2 + spriteHeight / 2;
                    int texY = static_cast<int>(d * 64 / spriteHeight);

                    uint32_t color = m_textureManager.getPixel(sp.textureId, texX, texY);
                    uint32_t alpha = (color >> 24) & 0xFF;

                    if (alpha == 0) continue; // Transparent

                    uint32_t litColor;
                    if (alpha == 254) {
                        // EMISSIVE PIXEL: Self-illuminating!
                        // Terminator red eyes, Taser cyan arcs, and marine visors shine in the dark!
                        litColor = 0xFF000000 | (color & 0x00FFFFFF);
                    } else {
                        litColor = applyLighting(color, spriteLight, isRestored);
                    }

                    // Hurt flash effect (stagger flash)
                    if (sp.isHurt) {
                        litColor = 0xFFFF3333; // Bright flash
                    }

                    scanlines[y * SCREEN_WIDTH + stripe] = litColor;
                }
            }
        }
    }
}

void RasterRenderer::renderWeaponViewModel(const Item& activeItem,
                                          float fireCooldownTimer,
                                          float muzzleFlashTimer,
                                          float reloadTimer,
                                          bool isScoped,
                                          float timeSeconds)
{
    (void)isScoped;
    (void)timeSeconds;
    if (activeItem.isEmpty()) return;

    uint32_t* scanlines = reinterpret_cast<uint32_t*>(m_frameBuffer.bits());

    int recoilOffset = 0;
    if (fireCooldownTimer > 0.0f) {
        recoilOffset = static_cast<int>(fireCooldownTimer * 50.0f);
    }

    int reloadOffset = 0;
    if (reloadTimer > 0.0f) {
        reloadOffset = static_cast<int>(std::sin(reloadTimer * 3.14159f) * 60.0f);
    }

    int baseX = SCREEN_WIDTH / 2 + 50;
    int baseY = SCREEN_HEIGHT - 130 + recoilOffset + reloadOffset;

    if (activeItem.type == ItemType::WEAPON_PISTOL) {
        for (int vy = 0; vy < 120; ++vy) {
            for (int vx = 0; vx < 80; ++vx) {
                int px = baseX + vx;
                int py = baseY + vy;
                if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;

                if (vx >= 20 && vx <= 55 && vy >= 0 && vy <= 40) {
                    uint8_t r = 50, g = 54, b = 60;
                    if (vy < 6) { r = 90; g = 95; b = 105; }
                    if (vx >= 48) { r = 35; g = 38; b = 42; }
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                } else if (vx >= 10 && vx <= 42 && vy >= 41 && vy <= 119) {
                    uint8_t r = 28, g = 30, b = 34;
                    if (vy > 75) { r = 40; g = 45; b = 42; }
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }

        if (muzzleFlashTimer > 0.0f) {
            int flashX = baseX + 52;
            int flashY = baseY - 12;
            for (int fy = -22; fy <= 22; ++fy) {
                for (int fx = -22; fx <= 22; ++fx) {
                    int distSq = fx * fx + fy * fy;
                    if (distSq <= 18 * 18) {
                        int px = flashX + fx;
                        int py = flashY + fy;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            if (distSq < 6 * 6) {
                                scanlines[py * SCREEN_WIDTH + px] = 0xFFFFFFFF;
                            } else {
                                scanlines[py * SCREEN_WIDTH + px] = 0xFFFFAA00;
                            }
                        }
                    }
                }
            }
            // Spark particle chaos
            for (int s = 0; s < 16; ++s) {
                float angle = static_cast<float>(s * 22.5f) * 3.14159f / 180.0f;
                float dist = 8.0f + static_cast<float>((s * 19) % 35);
                int sx = flashX + static_cast<int>(std::cos(angle) * dist);
                int sy = flashY + static_cast<int>(std::sin(angle) * dist * 0.6f);
                if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
                    scanlines[sy * SCREEN_WIDTH + sx] = ((s % 2) == 0) ? 0xFFFFFF88 : 0xFFFF8800;
                }
            }
        }
    } else if (activeItem.type == ItemType::WEAPON_RIFLE) {
        int rifleBaseX = SCREEN_WIDTH / 2 + 30;
        int rifleBaseY = SCREEN_HEIGHT - 145 + recoilOffset + reloadOffset;

        for (int vy = 0; vy < 140; ++vy) {
            for (int vx = 0; vx < 110; ++vx) {
                int px = rifleBaseX + vx;
                int py = rifleBaseY + vy;
                if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;

                if (vx >= 30 && vx <= 80 && vy >= 0 && vy <= 50) {
                    uint8_t r = 45, g = 48, b = 55;
                    if (vy < 6) { r = 85; g = 90; b = 100; }
                    if (vx >= 74) { r = 25; g = 28; b = 30; }
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                } else if (vx >= 42 && vx <= 58 && vy >= -12 && vy <= -1) {
                    uint8_t r = 35, g = 38, b = 42;
                    if (vx == 50 && vy == -6) { r = 255; g = 20; b = 20; }
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                } else if (vx >= 20 && vx <= 95 && vy >= 51 && vy <= 139) {
                    uint8_t r = 36, g = 40, b = 44;
                    if (vx >= 60 && vy > 75) { r = 48; g = 52; b = 48; }
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }

        if (muzzleFlashTimer > 0.0f) {
            int flashX = rifleBaseX + 78;
            int flashY = rifleBaseY - 8;
            for (int fy = -26; fy <= 26; ++fy) {
                for (int fx = -26; fx <= 26; ++fx) {
                    int distSq = fx * fx + fy * fy;
                    if (distSq <= 24 * 24) {
                        int px = flashX + fx;
                        int py = flashY + fy;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            if (distSq < 8 * 8) {
                                scanlines[py * SCREEN_WIDTH + px] = 0xFFFFFFFF;
                            } else {
                                scanlines[py * SCREEN_WIDTH + px] = 0xFFFF6600;
                            }
                        }
                    }
                }
            }
            // Fiery spark particle chaos
            for (int s = 0; s < 28; ++s) {
                float angle = static_cast<float>(s * 12.85f) * 3.14159f / 180.0f;
                float dist = 10.0f + static_cast<float>((s * 27) % 55);
                int sx = flashX + static_cast<int>(std::cos(angle) * dist);
                int sy = flashY + static_cast<int>(std::sin(angle) * dist * 0.65f);
                if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
                    scanlines[sy * SCREEN_WIDTH + sx] = ((s % 2) == 0) ? 0xFFFFFFFF : 0xFFFF9900;
                    if (sx + 1 < SCREEN_WIDTH) scanlines[sy * SCREEN_WIDTH + sx + 1] = 0xFFFF4400;
                }
            }
        }
    } else if (activeItem.type == ItemType::WEAPON_SNIPER) {
        int sniperBaseX = SCREEN_WIDTH / 2 + 40;
        int sniperBaseY = SCREEN_HEIGHT - 135 + recoilOffset + reloadOffset;

        for (int vy = 0; vy < 130; ++vy) {
            for (int vx = 0; vx < 100; ++vx) {
                int px = sniperBaseX + vx;
                int py = sniperBaseY + vy;
                if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;

                if (vx >= 25 && vx <= 75 && vy >= 10 && vy <= 45) {
                    uint8_t r = 50, g = 55, b = 60;
                    if (vy < 15) { r = 90; g = 95; b = 105; }
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                } else if (vx >= 35 && vx <= 65 && vy >= -12 && vy <= 9) {
                    uint8_t r = 30, g = 32, b = 36;
                    if (vx == 35 || vx == 65) { r = 85; g = 90; b = 100; }
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                } else if (vx >= 15 && vx <= 85 && vy >= 46 && vy <= 129) {
                    uint8_t r = 32, g = 36, b = 40;
                    scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }

        if (muzzleFlashTimer > 0.0f) {
            int flashX = sniperBaseX + 76;
            int flashY = sniperBaseY + 5;
            for (int fy = -30; fy <= 30; ++fy) {
                for (int fx = -30; fx <= 30; ++fx) {
                    int distSq = fx * fx + fy * fy;
                    if (distSq <= 28 * 28) {
                        int px = flashX + fx;
                        int py = flashY + fy;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            if (distSq < 10 * 10) {
                                scanlines[py * SCREEN_WIDTH + px] = 0xFFFFFFFF;
                            } else {
                                scanlines[py * SCREEN_WIDTH + px] = 0xFFFFCC00;
                            }
                        }
                    }
                }
            }
            // Sniper hypervelocity spark tracer
            for (int s = 0; s < 36; ++s) {
                float angle = static_cast<float>(s * 10.0f) * 3.14159f / 180.0f;
                float dist = 12.0f + static_cast<float>((s * 31) % 65);
                int sx = flashX + static_cast<int>(std::cos(angle) * dist);
                int sy = flashY + static_cast<int>(std::sin(angle) * dist * 0.55f);
                if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
                    scanlines[sy * SCREEN_WIDTH + sx] = ((s % 3) == 0) ? 0xFFFFFFFF : 0xFFFFCC33;
                    if (sx + 1 < SCREEN_WIDTH) scanlines[sy * SCREEN_WIDTH + sx + 1] = 0xFFFF5500;
                }
            }
        }
    } else if (activeItem.isConsumable()) {
        int medBaseX = SCREEN_WIDTH / 2 + 50;
        int medBaseY = SCREEN_HEIGHT - 100;

        for (int vy = 0; vy < 90; ++vy) {
            for (int vx = 0; vx < 70; ++vx) {
                int px = medBaseX + vx;
                int py = medBaseY + vy;
                if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;

                if (vx >= 15 && vx <= 55 && vy >= 15 && vy <= 75) {
                    if (activeItem.type == ItemType::MEDKIT) {
                        uint8_t r = 220, g = 225, b = 230;
                        if ((vx >= 30 && vx <= 40 && vy >= 35 && vy <= 55) ||
                            (vx >= 22 && vx <= 48 && vy >= 41 && vy <= 49)) {
                            r = 230; g = 30; b = 30;
                        }
                        scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                    } else {
                        uint8_t r = 10, g = 220, b = 255;
                        scanlines[py * SCREEN_WIDTH + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                }
            }
        }
    }
}

void RasterRenderer::renderSniperScopeOverlay(const Player& player, float timeSeconds) {
    (void)player;
    (void)timeSeconds;
    uint32_t* scanlines = reinterpret_cast<uint32_t*>(m_frameBuffer.bits());

    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    int radius = 145;
    int radiusSq = radius * radius;

    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        int dy = y - cy;
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int dx = x - cx;
            int distSq = dx * dx + dy * dy;

            if (distSq > radiusSq) {
                scanlines[y * SCREEN_WIDTH + x] = 0xFF05080C;
            } else if (distSq >= (radius - 4) * (radius - 4)) {
                scanlines[y * SCREEN_WIDTH + x] = 0xFF2A3440;
            } else {
                uint32_t orig = scanlines[y * SCREEN_WIDTH + x];
                uint32_t r = (orig >> 16) & 0xFF;
                uint32_t g = (orig >> 8)  & 0xFF;
                uint32_t b = orig         & 0xFF;

                r = static_cast<uint32_t>(r * 0.9f);
                g = static_cast<uint32_t>(g * 1.05f);
                b = static_cast<uint32_t>(b * 1.15f);
                scanlines[y * SCREEN_WIDTH + x] = 0xFF000000 | (std::min(255u, r) << 16) |
                                                              (std::min(255u, g) << 8)  |
                                                               std::min(255u, b);

                bool onX = (y == cy && std::abs(dx) > 4 && std::abs(dx) < radius - 8);
                bool onY = (x == cx && std::abs(dy) > 4 && std::abs(dy) < radius - 8);

                bool hashX = (y >= cy - 3 && y <= cy + 3 && (std::abs(dx) == 25 || std::abs(dx) == 50 || std::abs(dx) == 75 || std::abs(dx) == 100));
                bool hashY = (x >= cx - 3 && x <= cx + 3 && (std::abs(dy) == 25 || std::abs(dy) == 50 || std::abs(dy) == 75 || std::abs(dy) == 100));

                if (onX || onY || hashX || hashY) {
                    scanlines[y * SCREEN_WIDTH + x] = 0xFFFF2222;
                }
            }
        }
    }
}

void RasterRenderer::applyScreenEffects(float damageFlashTimer, float taserShockTimer, float healPulseTimer, float timeSeconds) {
    if (damageFlashTimer <= 0.0f && taserShockTimer <= 0.0f && healPulseTimer <= 0.0f) {
        return;
    }

    uint32_t* bits = reinterpret_cast<uint32_t*>(m_frameBuffer.bits());
    const int w = SCREEN_WIDTH;
    const int h = SCREEN_HEIGHT;
    const float cx = w / 2.0f;
    const float cy = h / 2.0f;
    const float maxDistSq = (cx * cx + cy * cy);

    float damageIntensity = std::clamp(damageFlashTimer / 0.25f, 0.0f, 1.0f);
    float taserIntensity  = std::clamp(taserShockTimer / 0.35f, 0.0f, 1.0f);
    float healIntensity   = std::clamp(healPulseTimer / 0.50f, 0.0f, 1.0f);

    for (int y = 0; y < h; ++y) {
        // Taser scanline jitter displacement
        int jitterX = 0;
        if (taserIntensity > 0.01f) {
            float wave = std::sin(static_cast<float>(y) * 0.45f + timeSeconds * 35.0f);
            if ((y % 2) == 0 && std::abs(wave) > 0.4f) {
                jitterX = static_cast<int>(wave * 6.0f * taserIntensity);
            }
        }

        for (int x = 0; x < w; ++x) {
            int srcX = std::clamp(x + jitterX, 0, w - 1);
            uint32_t pixel = bits[y * w + srcX];

            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            float dx = static_cast<float>(x) - cx;
            float dy = static_cast<float>(y) - cy;
            float distNorm = std::min(1.0f, (dx * dx + dy * dy) / maxDistSq);

            // 1. Red vignette flash (from Gun Drones or Terminators)
            if (damageIntensity > 0.01f) {
                float vignette = distNorm * distNorm * damageIntensity * 0.85f;
                r = static_cast<int>(r * (1.0f - vignette) + 255.0f * vignette);
                g = static_cast<int>(g * (1.0f - vignette));
                b = static_cast<int>(b * (1.0f - vignette));
            }

            // 2. Cyan/blue scanline jitter effect (when zapped by Taser Drone)
            if (taserIntensity > 0.01f) {
                bool isScanline = (y % 3 == 0);
                float cyanFactor = (isScanline ? 0.48f : 0.22f) * taserIntensity;
                r = static_cast<int>(r * (1.0f - cyanFactor));
                g = static_cast<int>(g * (1.0f - cyanFactor) + 225.0f * cyanFactor);
                b = static_cast<int>(b * (1.0f - cyanFactor) + 255.0f * cyanFactor);
            }

            // 3. Subtle green border pulse (when consuming Food Rations or Medkits)
            if (healIntensity > 0.01f) {
                float borderFactor = std::pow(distNorm, 2.5f) * healIntensity * 0.75f;
                r = static_cast<int>(r * (1.0f - borderFactor));
                g = static_cast<int>(g * (1.0f - borderFactor) + 240.0f * borderFactor);
                b = static_cast<int>(b * (1.0f - borderFactor) + 120.0f * borderFactor);
            }

            bits[y * w + x] = qRgba(std::clamp(r, 0, 255),
                                    std::clamp(g, 0, 255),
                                    std::clamp(b, 0, 255),
                                    255);
        }
    }
}
