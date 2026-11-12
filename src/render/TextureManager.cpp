#include "TextureManager.h"
#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <cstdlib>

TextureManager::TextureManager() {
    generateTextures();
}

const std::vector<uint32_t>& TextureManager::getTextureBuffer(int texId) const {
    auto it = m_textures.find(texId);
    if (it != m_textures.end()) {
        return it->second;
    }
    return m_defaultTexture;
}

void TextureManager::generateTextures() {
    // Generate base wall types
    generateLabWall(1);
    generateHazardWall(2);
    generateServerRack(3);
    generateConcreteWall(4);

    // Generate Vault Doors (IDs 10-13)
    generateVaultDoor(10, rgba(0, 229, 255), 1); // Cyan
    generateVaultDoor(11, rgba(255, 140, 0), 2); // Orange
    generateVaultDoor(12, rgba(0, 255, 102), 3); // Green
    generateVaultDoor(13, rgba(191, 0, 255), 4); // Purple

    // Generate Electrical Boxes (IDs 20-23)
    for (int i = 0; i < 4; ++i) {
        generateElectricalBox(20 + i, i + 1);
    }

    // Generate Vault Control Panels (IDs 30-33)
    generateControlPanel(30, rgba(0, 229, 255), 1);
    generateControlPanel(31, rgba(255, 140, 0), 2);
    generateControlPanel(32, rgba(0, 255, 102), 3);
    generateControlPanel(33, rgba(191, 0, 255), 4);

    // Generate Main Server Exit Bulkhead (ID 99)
    generateServerExitDoor(99);

    // Floor and Ceiling
    generateFloorTexture();
    generateCeilingTexture();

    // Item Pickup Billboard Sprites
    generateMedkitSprite(SPRITE_MEDKIT);
    generateStimpackSprite(SPRITE_STIMPACK);
    generatePistolSprite(SPRITE_PISTOL);
    generateRifleSprite(SPRITE_RIFLE);
    generateSniperSprite(SPRITE_SNIPER);
    generateAmmoBoxSprite(SPRITE_AMMO_PISTOL, rgba(50, 80, 50), "9MM");
    generateAmmoBoxSprite(SPRITE_AMMO_RIFLE, rgba(40, 70, 90), "556");
    generateAmmoBoxSprite(SPRITE_AMMO_SNIPER, rgba(90, 60, 30), "50");

    generateKeycardSprite(SPRITE_VAULT_KEY_1, rgba(0, 229, 255));
    generateKeycardSprite(SPRITE_VAULT_KEY_2, rgba(255, 140, 0));
    generateKeycardSprite(SPRITE_VAULT_KEY_3, rgba(0, 255, 102));
    generateKeycardSprite(SPRITE_VAULT_KEY_4, rgba(191, 0, 255));

    // Enemy Billboard Sprites
    generateTaserDroneSprite(SPRITE_TASER_DRONE);
    generateGunDroneSprite(SPRITE_GUN_DRONE);
    generateTerminatorSprite(SPRITE_TERMINATOR_UNARMED, false);
    generateTerminatorSprite(SPRITE_TERMINATOR_ARMED, true);
    generateEnemyDeadSprite(SPRITE_ENEMY_DEAD);

    // Co-op Teammate Survivor Marine Sprites
    generateSurvivorRifleSprite(SPRITE_SURVIVOR_RIFLE, false);
    generateSurvivorRifleSprite(SPRITE_SURVIVOR_RIFLE_FIRE, true);
    generateSurvivorPistolSprite(SPRITE_SURVIVOR_PISTOL, false);
    generateSurvivorPistolSprite(SPRITE_SURVIVOR_PISTOL_FIRE, true);
    generateSurvivorSniperSprite(SPRITE_SURVIVOR_SNIPER);
    generateSurvivorDownedSprite(SPRITE_SURVIVOR_DOWNED);
    generateSurvivorDeadSprite(SPRITE_SURVIVOR_DEAD);

    // Default texture (fallback)
    m_defaultTexture = m_textures[1];
}

void TextureManager::generateLabWall(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            int noise = (x * 7 + y * 13) % 9 - 4;
            uint8_t base = static_cast<uint8_t>(std::clamp(70 + noise, 0, 255));
            uint8_t r = base;
            uint8_t g = base + 4;
            uint8_t b = base + 10;

            if (x == 0 || y == 0 || x == 1 || y == 1) {
                r = std::min(255, r + 45);
                g = std::min(255, g + 45);
                b = std::min(255, b + 50);
            } else if (x >= 62 || y >= 62) {
                r = static_cast<uint8_t>(r * 0.4f);
                g = static_cast<uint8_t>(g * 0.4f);
                b = static_cast<uint8_t>(b * 0.4f);
            }

            if (y == 31 || y == 32) {
                r = 25; g = 28; b = 35;
            } else if (y == 30) {
                r = std::min(255, r + 35);
                g = std::min(255, g + 35);
                b = std::min(255, b + 40);
            }

            auto isRivet = [](int px, int py, int rx, int ry) {
                int dx = px - rx;
                int dy = py - ry;
                return (dx * dx + dy * dy) <= 3;
            };

            bool rivet = isRivet(x, y, 6, 6) || isRivet(x, y, 57, 6) ||
                         isRivet(x, y, 6, 57) || isRivet(x, y, 57, 57) ||
                         isRivet(x, y, 6, 26) || isRivet(x, y, 57, 26) ||
                         isRivet(x, y, 6, 37) || isRivet(x, y, 57, 37);

            if (rivet) {
                r = 175; g = 180; b = 195;
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateHazardWall(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            uint8_t r, g, b;

            if (x < 4 || x >= 60 || y < 4 || y >= 60) {
                int frameNoise = (x * 3 + y * 7) % 5;
                r = static_cast<uint8_t>(60 + frameNoise);
                g = static_cast<uint8_t>(64 + frameNoise);
                b = static_cast<uint8_t>(70 + frameNoise);
            } else {
                int stripe = (x + y) % 18;
                if (stripe < 9) {
                    r = 220; g = 180; b = 20;
                } else {
                    r = 30; g = 30; b = 32;
                }
            }

            if ((x == 2 || x == 61) && (y == 2 || y == 61)) {
                r = 190; g = 190; b = 200;
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateServerRack(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            uint8_t r = 24;
            uint8_t g = 26;
            uint8_t b = 30;

            if (x < 3 || x >= 61 || y < 3 || y >= 61) {
                r = 45; g = 50; b = 58;
            } else {
                int slotY = (y - 3) % 10;
                if (slotY == 0) {
                    r = 65; g = 70; b = 80;
                } else if (slotY == 1 || slotY == 2) {
                    if (x >= 8 && x <= 20) {
                        if ((x % 3 == 0) && (y % 4 == 0)) {
                            if ((x + y) % 5 == 0) {
                                r = 255; g = 160; b = 20;
                            } else {
                                r = 30; g = 255; b = 80;
                            }
                        } else {
                            r = 10; g = 20; b = 15;
                        }
                    } else if (x >= 24 && x <= 56) {
                        r = 40; g = 44; b = 50;
                    }
                } else {
                    if (x >= 8 && x <= 56 && (x % 2 == 0)) {
                        r = 10; g = 12; b = 15;
                    } else {
                        r = 35; g = 38; b = 44;
                    }
                }
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateConcreteWall(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            int n = (x * 19 + y * 31 + (x * y) % 17) % 25 - 12;
            uint8_t c = static_cast<uint8_t>(std::clamp(65 + n, 30, 110));
            uint8_t r = c;
            uint8_t g = c - 2;
            uint8_t b = c - 6;

            if (x == 32) {
                r = 20; g = 18; b = 16;
            } else if (x == 33) {
                r = std::min(255, r + 20);
                g = std::min(255, g + 20);
                b = std::min(255, b + 20);
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateVaultDoor(int id, uint32_t accentColor, int vaultNum) {
    (void)vaultNum;
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    uint8_t accR = (accentColor >> 16) & 0xFF;
    uint8_t accG = (accentColor >> 8)  & 0xFF;
    uint8_t accB = accentColor         & 0xFF;

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            uint8_t r = 50;
            uint8_t g = 54;
            uint8_t b = 62;

            if (x < 5 || x >= 59 || y < 5 || y >= 59) {
                r = 30; g = 32; b = 38;
            } else if (x == 5 || y == 5 || x == 58 || y == 58) {
                r = 90; g = 95; b = 110;
            } else {
                if (x == 31 || x == 32) {
                    r = 15; g = 18; b = 22;
                } else if (x == 30 || x == 33) {
                    r = 80; g = 85; b = 95;
                }

                if (y >= 14 && y <= 18) {
                    r = 40; g = 44; b = 50;
                } else if (y >= 46 && y <= 50) {
                    r = 40; g = 44; b = 50;
                }

                int cx = 31;
                int cy = 31;
                int distSq = (x - cx) * (x - cx) + (y - cy) * (y - cy);
                if (distSq <= 12 * 12) {
                    if (distSq <= 11 * 11 && distSq >= 9 * 9) {
                        r = accR; g = accG; b = accB;
                    } else if (distSq < 9 * 9 && distSq >= 4 * 4) {
                        r = 25; g = 28; b = 34;
                    } else if (distSq < 4 * 4) {
                        r = 240; g = 30; b = 30;
                    }
                }

                if (y >= 15 && y <= 17 && ((x >= 12 && x <= 22) || (x >= 42 && x <= 52))) {
                    r = accR; g = accG; b = accB;
                }
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateElectricalBox(int id, int boxNum) {
    (void)boxNum;
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            int n = (x * 17 + y * 23) % 11 - 5;
            uint8_t r = static_cast<uint8_t>(std::clamp(55 + n, 0, 255));
            uint8_t g = static_cast<uint8_t>(std::clamp(53 + n, 0, 255));
            uint8_t b = static_cast<uint8_t>(std::clamp(50 + n, 0, 255));

            if (x >= 29 && x <= 34 && y < 14) {
                r = 90; g = 95; b = 105;
                if (x == 29) { r = 120; g = 125; b = 135; }
            }

            if (x >= 12 && x <= 51 && y >= 14 && y <= 54) {
                r = 45; g = 52; b = 48;

                if (x == 12 || y == 14) {
                    r = 85; g = 95; b = 88;
                } else if (x == 51 || y == 54) {
                    r = 20; g = 25; b = 22;
                } else if (x == 13 || y == 15 || x == 50 || y == 53) {
                    r = 30; g = 35; b = 32;
                } else {
                    int triTop = 21;
                    int triBot = 33;
                    if (y >= triTop && y <= triBot) {
                        int halfW = (y - triTop) * 7 / (triBot - triTop);
                        int triLeft = 31 - halfW;
                        int triRight = 31 + halfW;
                        if (x >= triLeft && x <= triRight) {
                            r = 240; g = 200; b = 20;
                            if (x >= 30 && x <= 32 && y >= 24 && y <= 30) {
                                r = 20; g = 20; b = 20;
                            }
                        }
                    }

                    if (x >= 40 && x <= 45 && y >= 37 && y <= 48) {
                        r = 25; g = 25; b = 25;
                        if (y >= 40 && y <= 44 && x >= 41 && x <= 44) {
                            r = 220; g = 30; b = 20;
                        }
                    }

                    if (x >= 18 && x <= 22) {
                        if (y >= 38 && y <= 41) {
                            r = 255; g = 40; b = 40;
                        } else if (y >= 44 && y <= 47) {
                            r = 30; g = 255; b = 60;
                        }
                    }
                }
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateControlPanel(int id, uint32_t accentColor, int panelNum) {
    (void)panelNum;
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    uint8_t accR = (accentColor >> 16) & 0xFF;
    uint8_t accG = (accentColor >> 8)  & 0xFF;
    uint8_t accB = accentColor         & 0xFF;

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            uint8_t r = 60;
            uint8_t g = 64;
            uint8_t b = 70;

            if (x >= 10 && x <= 53 && y >= 8 && y <= 56) {
                r = 28; g = 30; b = 35;

                if (x == 10 || y == 8) {
                    r = 75; g = 80; b = 90;
                } else if (x == 53 || y == 56) {
                    r = 15; g = 17; b = 20;
                } else {
                    if (x >= 16 && x <= 47 && y >= 14 && y <= 28) {
                        r = static_cast<uint8_t>(accR * 0.25f);
                        g = static_cast<uint8_t>(accG * 0.25f);
                        b = static_cast<uint8_t>(accB * 0.25f);

                        if (y % 2 == 0) {
                            r = static_cast<uint8_t>(r * 0.7f);
                            g = static_cast<uint8_t>(g * 0.7f);
                            b = static_cast<uint8_t>(b * 0.7f);
                        }

                        if ((y == 18 || y == 23) && x >= 20 && x <= 43 && (x % 3 != 0)) {
                            r = accR; g = accG; b = accB;
                        }
                    }

                    if (x >= 16 && x <= 34 && y >= 34 && y <= 50) {
                        int kx = (x - 16) % 6;
                        int ky = (y - 34) % 5;
                        if (kx >= 1 && kx <= 4 && ky >= 1 && ky <= 3) {
                            r = 85; g = 90; b = 100;
                        } else {
                            r = 18; g = 20; b = 24;
                        }
                    }

                    if (x >= 39 && x <= 47 && y >= 36 && y <= 48) {
                        r = 20; g = 22; b = 26;
                        if (x >= 41 && x <= 45 && y == 42) {
                            r = 5; g = 5; b = 5;
                        }
                        if (x >= 42 && x <= 44 && y == 38) {
                            r = 255; g = 30; b = 30;
                        }
                    }
                }
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateServerExitDoor(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            uint8_t r = 38;
            uint8_t g = 40;
            uint8_t b = 48;

            if (x < 6 || x >= 58 || y < 6 || y >= 58) {
                int stripe = (x + y) % 12;
                if (stripe < 6) {
                    r = 210; g = 170; b = 20;
                } else {
                    r = 20; g = 20; b = 20;
                }
            } else {
                int cx = 31;
                int cy = 31;
                int dx = x - cx;
                int dy = y - cy;
                int distSq = dx * dx + dy * dy;

                if (distSq <= 14 * 14 && distSq >= 12 * 12) {
                    r = 180; g = 30; b = 30;
                } else if (distSq < 12 * 12 && distSq >= 6 * 6) {
                    r = 25; g = 10; b = 15;
                    if ((std::abs(dx) == std::abs(dy)) || dx == 0 || dy == 0) {
                        r = 220; g = 40; b = 40;
                    }
                } else if (distSq < 6 * 6) {
                    r = 255; g = 20; b = 20;
                }

                if ((x >= 8 && x <= 14) || (x >= 49 && x <= 55)) {
                    if ((y >= 14 && y <= 20) || (y >= 43 && y <= 49)) {
                        r = 140; g = 145; b = 160;
                    }
                }

                if (y >= 8 && y <= 10 && (x % 8 >= 2 && x % 8 <= 4)) {
                    r = 255; g = 25; b = 25;
                }
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateFloorTexture() {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            int grain = (x * 17 + y * 23) % 11 - 5;
            uint8_t base = static_cast<uint8_t>(std::clamp(48 + grain, 28, 72));
            uint8_t r = base;
            uint8_t g = base + 2;
            uint8_t b = base + 5;

            // 4 large concrete slab seams (at 0, 31, 32, 63)
            if (x == 0 || x == 31 || x == 32 || x == 63 ||
                y == 0 || y == 31 || y == 32 || y == 63) {
                r = 16; g = 18; b = 22; // Deep expansion seam
            } else if (x == 1 || x == 30 || x == 33 || x == 62 ||
                       y == 1 || y == 30 || y == 33 || y == 62) {
                r = std::min(255, r + 15);
                g = std::min(255, g + 15);
                b = std::min(255, b + 18); // Beveled concrete edge
            }

            // Metal drainage grate in quadrant 1 (x: 8..23, y: 8..23)
            if (x >= 8 && x <= 23 && y >= 8 && y <= 23) {
                if (x == 8 || x == 23 || y == 8 || y == 23) {
                    r = 85; g = 90; b = 100; // Steel rim
                } else if (x % 3 == 0) {
                    r = 12; g = 14; b = 18; // Grate slot opening
                } else {
                    r = 65; g = 70; b = 80; // Metal bar
                }
            }

            // Scattered research documents / paper clutter (matching image_6.png)
            // Document 1 (x: 40..52, y: 12..22)
            if (x >= 40 && x <= 52 && y >= 12 && y <= 22) {
                if (x == 40 || x == 52 || y == 12 || y == 22) {
                    r = 160; g = 155; b = 140; // Paper edge shadow
                } else {
                    r = 215; g = 210; b = 195; // Aged white paper sheet
                    if (y % 2 == 0 && x >= 42 && x <= 50) {
                        r = 70; g = 65; b = 60; // Tiny printed text lines
                    }
                }
            }

            // Document 2 (x: 36..46, y: 44..54)
            if (x >= 36 && x <= 46 && y >= 44 && y <= 54) {
                if (x == 36 || x == 46 || y == 44 || y == 54) {
                    r = 150; g = 145; b = 135;
                } else {
                    r = 205; g = 200; b = 185;
                    if (y % 2 == 1 && x >= 38 && x <= 44) {
                        r = 75; g = 70; b = 65;
                    }
                }
            }

            // Corner rivets on concrete panels
            auto isRivet = [](int px, int py, int rx, int ry) {
                int dx = px - rx;
                int dy = py - ry;
                return (dx * dx + dy * dy) <= 2;
            };
            if (isRivet(x, y, 4, 4) || isRivet(x, y, 28, 4) || isRivet(x, y, 4, 28) || isRivet(x, y, 28, 28) ||
                isRivet(x, y, 35, 35) || isRivet(x, y, 59, 35) || isRivet(x, y, 35, 59) || isRivet(x, y, 59, 59)) {
                r = 120; g = 125; b = 135;
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[TEX_FLOOR] = buf;
}

void TextureManager::generateCeilingTexture() {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Industrial coffered metallic ceiling panels
            int grain = (x * 19 + y * 11) % 9 - 4;
            uint8_t base = static_cast<uint8_t>(std::clamp(52 + grain, 30, 75));
            uint8_t r = base;
            uint8_t g = base + 3;
            uint8_t b = base + 7;

            // Panel borders & deep steel recesses
            if (x < 3 || x >= 61 || y < 3 || y >= 61 ||
                (x >= 30 && x <= 33) || (y >= 30 && y <= 33)) {
                r = 20; g = 22; b = 28; // Deep structural girder seam
            } else if (x == 3 || x == 60 || y == 3 || y == 60 ||
                       x == 29 || x == 34 || y == 29 || y == 34) {
                r = std::min(255, r + 24);
                g = std::min(255, g + 24);
                b = std::min(255, b + 28); // Beveled metal girder edge
            }

            // Horizontal Heavy Steel Conduit Pipe (y: 13..18)
            if (y >= 13 && y <= 18) {
                float pipeY = static_cast<float>(y - 13) / 5.0f; // 0.0 to 1.0
                float cylindrical = std::sin(pipeY * 3.14159f);
                r = static_cast<uint8_t>(60 + 90 * cylindrical);
                g = static_cast<uint8_t>(65 + 95 * cylindrical);
                b = static_cast<uint8_t>(75 + 110 * cylindrical);
                if (y == 13) { r = 18; g = 20; b = 24; } // Pipe drop-shadow top
                if (y == 18) { r = 15; g = 17; b = 20; } // Pipe drop-shadow bottom
            }

            // Pipe mounting bracket / clamp at x: 18..20 & x: 44..46
            if ((y >= 12 && y <= 19) && ((x >= 18 && x <= 20) || (x >= 44 && x <= 46))) {
                r = 110; g = 115; b = 130; // Steel mounting bracket
            }

            // Vertical Copper Coolant Conduit (x: 48..52)
            if (x >= 48 && x <= 52) {
                float pipeX = static_cast<float>(x - 48) / 4.0f;
                float cyl = std::sin(pipeX * 3.14159f);
                r = static_cast<uint8_t>(110 + 60 * cyl);
                g = static_cast<uint8_t>(65 + 35 * cyl);
                b = static_cast<uint8_t>(40 + 20 * cyl);
            }

            // Dangling dark wiring cables (matching image_6.png)
            // Cable 1: curved diagonal arc from (10, 2) to (28, 28)
            int cableY1 = 2 + static_cast<int>(26.0f * std::sin(static_cast<float>(x - 10) / 18.0f * 1.57f));
            if (x >= 10 && x <= 28 && std::abs(y - cableY1) <= 1) {
                r = 12; g = 14; b = 16;
            }

            // Cable 2: curved arc from (35, 28) to (58, 6)
            int cableY2 = 6 + static_cast<int>(22.0f * std::cos(static_cast<float>(x - 35) / 23.0f * 1.57f));
            if (x >= 35 && x <= 58 && std::abs(y - cableY2) <= 1) {
                r = 14; g = 12; b = 10;
            }

            // Central Industrial Lamp Fixture (x: 25..38, y: 25..38)
            if (x >= 25 && x <= 38 && y >= 25 && y <= 38) {
                if (x == 25 || x == 38 || y == 25 || y == 38) {
                    r = 85; g = 90; b = 105; // Outer metallic fixture chassis
                } else if (x == 26 || x == 37 || y == 26 || y == 37) {
                    r = 40; g = 44; b = 52;
                } else {
                    // Diffuser bulb glow core
                    r = 230; g = 240; b = 255;
                }
            }

            // Perimeter Rivets / Bolts on panels
            auto isBolt = [](int px, int py, int bx, int by) {
                int dx = px - bx;
                int dy = py - by;
                return (dx * dx + dy * dy) <= 2;
            };
            if (isBolt(x, y, 7, 7) || isBolt(x, y, 26, 7) || isBolt(x, y, 7, 26) || isBolt(x, y, 26, 26) ||
                isBolt(x, y, 37, 7) || isBolt(x, y, 56, 7) || isBolt(x, y, 37, 26) || isBolt(x, y, 56, 26) ||
                isBolt(x, y, 7, 37) || isBolt(x, y, 26, 37) || isBolt(x, y, 7, 56) || isBolt(x, y, 26, 56) ||
                isBolt(x, y, 37, 37) || isBolt(x, y, 56, 37) || isBolt(x, y, 37, 56) || isBolt(x, y, 56, 56)) {
                r = 145; g = 150; b = 165;
            }

            buf[y * TEX_SIZE + x] = rgba(r, g, b);
        }
    }
    m_textures[TEX_CEILING] = buf;
}

// ==================== ITEM BILLBOARD SPRITES ====================

void TextureManager::generateMedkitSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            if (x >= 14 && x <= 50 && y >= 22 && y <= 52) {
                uint8_t r = 230;
                uint8_t g = 232;
                uint8_t b = 238;

                if (x == 14 || x == 50 || y == 22 || y == 52) {
                    r = 100; g = 105; b = 115;
                } else if (y >= 35 && y <= 36) {
                    r = 140; g = 145; b = 155;
                }

                bool hBar = (x >= 26 && x <= 38 && y >= 34 && y <= 40);
                bool vBar = (x >= 30 && x <= 34 && y >= 28 && y <= 46);
                if (hBar || vBar) {
                    r = 220; g = 25; b = 25;
                }

                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            } else if (x >= 27 && x <= 37 && y >= 16 && y <= 21) {
                if (x == 27 || x == 37 || y == 16) {
                    buf[y * TEX_SIZE + x] = rgba(80, 85, 95, 255);
                }
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateStimpackSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            if ((x == 31 || x == 32) && y >= 48 && y <= 56) {
                buf[y * TEX_SIZE + x] = rgba(190, 200, 215, 255);
            } else if (x >= 26 && x <= 37 && y >= 22 && y <= 47) {
                if (x == 26 || x == 37 || y == 22 || y == 47) {
                    buf[y * TEX_SIZE + x] = rgba(90, 100, 115, 255);
                } else {
                    uint8_t r = 10;
                    uint8_t g = static_cast<uint8_t>(200 + (y % 4) * 12);
                    uint8_t b = 255;
                    if (x == 28) { r = 180; g = 255; b = 255; }
                    buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
                }
            } else if (x >= 28 && x <= 35 && y >= 12 && y <= 21) {
                buf[y * TEX_SIZE + x] = rgba(130, 135, 145, 255);
            } else if (x >= 22 && x <= 41 && y >= 22 && y <= 24) {
                buf[y * TEX_SIZE + x] = rgba(70, 75, 85, 255);
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generatePistolSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            if (x >= 16 && x <= 52 && y >= 26 && y <= 34) {
                uint8_t r = 55, g = 58, b = 65;
                if (y == 26) { r += 40; g += 40; b += 45; }
                if (x >= 46) { r = 40; g = 42; b = 48; }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            } else if (x >= 18 && x <= 26 && y >= 35 && y <= 50) {
                uint8_t r = 30, g = 32, b = 36;
                if ((x + y) % 3 == 0) r += 15;
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            } else if (x >= 27 && x <= 33 && y >= 35 && y <= 41) {
                if (x == 33 || y == 41) {
                    buf[y * TEX_SIZE + x] = rgba(60, 65, 75, 255);
                } else if (x == 29 && y == 37) {
                    buf[y * TEX_SIZE + x] = rgba(180, 185, 195, 255);
                }
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateRifleSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            if (x >= 8 && x <= 58 && y >= 26 && y <= 33) {
                uint8_t r = 48, g = 52, b = 58;
                if (y == 26) { r += 35; g += 35; b += 40; }
                if (x >= 52) { r = 35; g = 38; b = 42; }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            } else if (x >= 8 && x <= 18 && y >= 28 && y <= 42) {
                buf[y * TEX_SIZE + x] = rgba(35, 38, 44, 255);
            } else if (x >= 24 && x <= 30 && y >= 34 && y <= 46) {
                buf[y * TEX_SIZE + x] = rgba(30, 32, 38, 255);
            } else if (x >= 34 && x <= 40 && y >= 34 && y <= 48) {
                int curve = (y - 34) / 4;
                if (x + curve >= 34 && x + curve <= 40) {
                    buf[y * TEX_SIZE + (x + curve)] = rgba(65, 68, 75, 255);
                }
            } else if (x >= 26 && x <= 38 && y >= 21 && y <= 25) {
                if (x == 32 && y == 23) {
                    buf[y * TEX_SIZE + x] = rgba(255, 30, 30, 255);
                } else {
                    buf[y * TEX_SIZE + x] = rgba(40, 44, 50, 255);
                }
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateSniperSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            if (x >= 4 && x <= 62 && y >= 28 && y <= 32) {
                uint8_t r = 50, g = 55, b = 60;
                if (y == 28) { r += 40; g += 40; b += 45; }
                if (x >= 56) { r = 30; g = 32; b = 36; }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            } else if (x >= 4 && x <= 16 && y >= 28 && y <= 42) {
                buf[y * TEX_SIZE + x] = rgba(35, 38, 42, 255);
            } else if (x >= 20 && x <= 26 && y >= 33 && y <= 45) {
                buf[y * TEX_SIZE + x] = rgba(28, 30, 34, 255);
            } else if (x >= 18 && x <= 42 && y >= 19 && y <= 26) {
                uint8_t r = 40, g = 44, b = 50;
                if (x == 18 || x == 42) { r = 80; g = 85; b = 95; }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            } else if (y >= 33 && y <= 47) {
                if (x == 48 - (y - 33) / 3 || x == 52 + (y - 33) / 3) {
                    buf[y * TEX_SIZE + x] = rgba(90, 95, 105, 255);
                }
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateAmmoBoxSprite(int id, uint32_t boxColor, const char* label) {
    (void)label;
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    uint8_t bR = (boxColor >> 16) & 0xFF;
    uint8_t bG = (boxColor >> 8)  & 0xFF;
    uint8_t bB = boxColor         & 0xFF;

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            if (x >= 16 && x <= 48 && y >= 26 && y <= 50) {
                uint8_t r = bR;
                uint8_t g = bG;
                uint8_t b = bB;

                if (x == 16 || y == 26) {
                    r = std::min(255, r + 40);
                    g = std::min(255, g + 40);
                    b = std::min(255, b + 40);
                } else if (x == 48 || y == 50) {
                    r = static_cast<uint8_t>(r * 0.5f);
                    g = static_cast<uint8_t>(g * 0.5f);
                    b = static_cast<uint8_t>(b * 0.5f);
                }

                if (y >= 35 && y <= 41 && x >= 20 && x <= 44) {
                    r = 220; g = 190; b = 30;
                }

                if (x >= 30 && x <= 34 && y >= 28 && y <= 34) {
                    r = 160; g = 165; b = 175;
                }

                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            } else if (x >= 26 && x <= 38 && y >= 20 && y <= 25) {
                if (x == 26 || x == 38 || y == 20) {
                    buf[y * TEX_SIZE + x] = rgba(130, 135, 145, 255);
                }
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateKeycardSprite(int id, uint32_t glowColor) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    uint8_t gR = (glowColor >> 16) & 0xFF;
    uint8_t gG = (glowColor >> 8)  & 0xFF;
    uint8_t gB = glowColor         & 0xFF;

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            if (x >= 16 && x <= 48 && y >= 18 && y <= 46) {
                uint8_t r = 24, g = 26, b = 32;

                if (x == 16 || x == 48 || y == 18 || y == 46) {
                    r = gR; g = gG; b = gB;
                } else if (x >= 19 && x <= 22 && y >= 22 && y <= 42) {
                    r = 15; g = 15; b = 15;
                } else if (x >= 27 && x <= 37 && y >= 26 && y <= 36) {
                    r = 230; g = 185; b = 40;
                    if (x == 32 || y == 31) { r = 120; g = 90; b = 20; }
                } else if (x >= 41 && x <= 44 && y >= 22 && y <= 25) {
                    r = gR; g = gG; b = gB;
                }

                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateTaserDroneSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Main drone disc chassis: center (31.5, 30.0), radius ~16
            float dx = x - 31.5f;
            float dy = y - 30.0f;
            float distSq = dx * dx + dy * dy;

            // Hover rotor pods on left & right
            bool leftRotor = (x >= 8 && x <= 18 && y >= 24 && y <= 34);
            bool rightRotor = (x >= 45 && x <= 55 && y >= 24 && y <= 34);

            if (leftRotor || rightRotor) {
                uint8_t r = 40, g = 44, b = 50;
                if (x == 8 || x == 18 || x == 45 || x == 55 || y == 24 || y == 34) {
                    r = 80; g = 85; b = 95;
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Central drone body
            else if (distSq <= 15.0f * 15.0f) {
                uint8_t r = 48, g = 52, b = 60;
                if (distSq >= 13.0f * 13.0f) {
                    r = 90; g = 95; b = 105; // Chassis rim
                }

                // Central sensor camera (emissive cyan!)
                if (distSq <= 4.0f * 4.0f) {
                    buf[y * TEX_SIZE + x] = emissive(0, 240, 255);
                } else {
                    buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
                }
            }
            // Protruding taser prongs at the bottom: x: 22..25 and x: 38..41, y: 44..55
            else if ((x >= 22 && x <= 25 && y >= 44 && y <= 55) ||
                     (x >= 38 && x <= 41 && y >= 44 && y <= 55)) {
                buf[y * TEX_SIZE + x] = rgba(160, 165, 175, 255); // Titanium prongs
            }
            // Active electric taser arc between prongs (emissive glowing cyan arc!)
            else if (x >= 26 && x <= 37 && y >= 48 && y <= 53) {
                int wave = (x * 7 + y * 13) % 4;
                if (wave == 0 || wave == 1) {
                    buf[y * TEX_SIZE + x] = emissive(0, 255, 255); // Electric plasma arc
                }
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateGunDroneSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Hexagonal armored hull: x: 16..47, y: 16..42
            if (x >= 16 && x <= 47 && y >= 16 && y <= 42) {
                uint8_t r = 50, g = 54, b = 62;

                // Armor plate seams
                if (x == 16 || x == 47 || y == 16 || y == 42) {
                    r = 85; g = 90; b = 100;
                } else if (y >= 27 && y <= 29) {
                    r = 30; g = 32; b = 38;
                }

                // Central Vanguard orange targeting optics (emissive!)
                int dx = x - 31;
                int dy = y - 24;
                if (dx * dx + dy * dy <= 4 * 4) {
                    buf[y * TEX_SIZE + x] = emissive(255, 140, 0); // Orange optic lens
                    continue;
                }

                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Rotor struts: x: 6..15 and x: 48..57, y: 18..24
            else if ((x >= 6 && x <= 15 && y >= 18 && y <= 24) ||
                     (x >= 48 && x <= 57 && y >= 18 && y <= 24)) {
                buf[y * TEX_SIZE + x] = rgba(70, 75, 85, 255);
            }
            // Underslung kinetic cannon barrel: x: 28..35, y: 43..56
            else if (x >= 28 && x <= 35 && y >= 43 && y <= 56) {
                uint8_t r = 32, g = 34, b = 38;
                if (x == 28 || x == 35) r = 60;
                // Muzzle crown
                if (y >= 54) r = 80;
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateTerminatorSprite(int id, bool armed) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // -------------------------------------------------------------
            // 1. Dual Shoulder Exhaust Stacks (Angled chimneys behind shoulders)
            // Left stack: x: 11..16, y: 7..24
            // Right stack: x: 47..52, y: 7..24
            // -------------------------------------------------------------
            bool leftStack = (x >= 11 && x <= 16 && y >= 7 && y <= 24);
            bool rightStack = (x >= 47 && x <= 52 && y >= 7 && y <= 24);

            if (leftStack || rightStack) {
                // Fiery heat plume / superheated flame at the top of chimney (y: 7..10)
                if (y <= 10) {
                    if (y == 7 || y == 8) {
                        buf[y * TEX_SIZE + x] = emissive(255, 140, 20); // Blazing orange fire
                    } else {
                        buf[y * TEX_SIZE + x] = emissive(255, 60, 10);  // Hot red ember core
                    }
                    continue;
                }
                // Heavy heat-scorched iron exhaust cylinder
                uint8_t iron = (x == 11 || x == 16 || x == 47 || x == 52) ? 32 : 55;
                if ((y - 11) % 4 == 0) iron += 20; // Reinforcing steel collar bands
                buf[y * TEX_SIZE + x] = rgba(iron, iron - 2, iron - 4, 255);
                continue;
            }

            // -------------------------------------------------------------
            // 2. Biomechanical Cranium & Chrome Skull: x: 22..41, y: 8..26
            // -------------------------------------------------------------
            if (x >= 22 && x <= 41 && y >= 8 && y <= 26) {
                uint8_t cranium = 145; // Dark tungsten steel

                // Cranium beveling & rivets
                if (x == 22 || x == 41 || y == 8) {
                    cranium = 75;
                } else if (y >= 9 && y <= 13) {
                    // Curved skull brow highlight
                    cranium = (x >= 26 && x <= 37) ? 175 : 130;
                }

                // Cranial center suture seam line
                if (x == 31 || x == 32) cranium -= 30;

                // Glowing ruby-red optical eye sockets (EMISSIVE!)
                // Left eye: x: 26..28, y: 15..17; Right eye: x: 35..37, y: 15..17
                bool leftEye = (x >= 26 && x <= 28 && y >= 15 && y <= 17);
                bool rightEye = (x >= 35 && x <= 37 && y >= 15 && y <= 17);
                if (leftEye || rightEye) {
                    if ((x == 27 && y == 16) || (x == 36 && y == 16)) {
                        buf[y * TEX_SIZE + x] = emissive(255, 230, 230); // Blinding laser focal pupil
                    } else {
                        buf[y * TEX_SIZE + x] = emissive(255, 15, 25);   // Ruby red iris glow
                    }
                    continue;
                }

                // Dark sunken eye socket cavity shadows
                if ((x >= 25 && x <= 29 && y >= 14 && y <= 18) ||
                    (x >= 34 && x <= 38 && y >= 14 && y <= 18)) {
                    buf[y * TEX_SIZE + x] = rgba(20, 22, 26, 255);
                    continue;
                }

                // Nasal aperture slit
                if ((x == 31 || x == 32) && (y == 19 || y == 20)) {
                    buf[y * TEX_SIZE + x] = rgba(15, 16, 20, 255);
                    continue;
                }

                // Serrated metallic jaw & chrome teeth: y: 22..25, x: 26..37
                if (y >= 22 && y <= 25 && x >= 26 && x <= 37) {
                    if (x % 2 == 0) {
                        cranium = 210; // Polished chrome tooth
                    } else {
                        cranium = 40;  // Gap / hydraulic recess
                    }
                }

                // Jaw hinge pneumatic cylinder (x: 23..25 and x: 38..40, y: 22..26)
                if ((x <= 25 || x >= 38) && y >= 22) {
                    cranium = 90;
                }

                buf[y * TEX_SIZE + x] = rgba(cranium, cranium + 2, cranium + 6, 255);
                continue;
            }

            // -------------------------------------------------------------
            // 3. Cervical Vertebrae & Neck Hydraulic Cables: x: 27..36, y: 27..30
            // -------------------------------------------------------------
            if (x >= 27 && x <= 36 && y >= 27 && y <= 30) {
                uint8_t neck = (x == 31 || x == 32) ? 120 : 65; // Central spinal column
                buf[y * TEX_SIZE + x] = rgba(neck, neck, neck + 5, 255);
                continue;
            }

            // -------------------------------------------------------------
            // 4. Bulky Scrap Armor Pauldrons (Shoulders):
            // Left pauldron: x: 10..22, y: 24..34
            // Right pauldron: x: 41..53, y: 24..34
            // -------------------------------------------------------------
            bool leftPauldron = (x >= 10 && x <= 22 && y >= 24 && y <= 34);
            bool rightPauldron = (x >= 41 && x <= 53 && y >= 24 && y <= 34);

            if (leftPauldron || rightPauldron) {
                // Tiered heavy armor plate shading
                uint8_t armor = 46;
                int tierY = (y - 24) % 4;
                if (tierY == 0) armor = 85;       // Plate lip highlight
                else if (tierY == 3) armor = 28;  // Plate underside shadow

                // Corner rivets
                if ((x == 12 || x == 20 || x == 43 || x == 51) && (y == 26 || y == 30)) {
                    armor = 160;
                }

                buf[y * TEX_SIZE + x] = rgba(armor, armor + 3, armor + 7, 255);
                continue;
            }

            // -------------------------------------------------------------
            // 5. Heavy Armored Cybernetic Ribcage & Torso: x: 18..45, y: 31..47
            // -------------------------------------------------------------
            if (x >= 18 && x <= 45 && y >= 31 && y <= 47) {
                // Central Vanguard AI reactor core conduit: x: 30..33, y: 36..39
                if (x >= 30 && x <= 33 && y >= 36 && y <= 39) {
                    buf[y * TEX_SIZE + x] = emissive(255, 30, 30); // Glowing internal nuclear core
                    continue;
                }

                uint8_t rib = 70;
                // Armored rib horizontal slats
                if ((y - 31) % 4 == 0) {
                    rib = 120; // Heavy rib edge
                } else if ((y - 31) % 4 == 3) {
                    rib = 32;  // Crevice shadow
                } else {
                    rib = 58;
                }

                // Flanking hydraulic actuators
                if (x <= 20 || x >= 43) rib = 95;

                buf[y * TEX_SIZE + x] = rgba(rib, rib + 4, rib + 8, 255);
                continue;
            }

            // -------------------------------------------------------------
            // 6. Armored Pelvic Girdle & Hips: x: 23..40, y: 48..52
            // -------------------------------------------------------------
            if (x >= 23 && x <= 40 && y >= 48 && y <= 52) {
                uint8_t pelvis = (y == 48) ? 80 : 45;
                if (x == 31 || x == 32) pelvis = 65; // Central ball-socket spine base
                buf[y * TEX_SIZE + x] = rgba(pelvis, pelvis + 2, pelvis + 5, 255);
                continue;
            }

            // -------------------------------------------------------------
            // 7. Skeletal Pneumatic Legs & Clawed Feet
            // Thighs: Left x: 19..26, Right x: 37..44, y: 53..58
            // Feet / Talons: Left x: 17..27, Right x: 36..46, y: 59..63
            // -------------------------------------------------------------
            bool leftThigh = (x >= 19 && x <= 26 && y >= 53 && y <= 58);
            bool rightThigh = (x >= 37 && x <= 44 && y >= 53 && y <= 58);
            if (leftThigh || rightThigh) {
                uint8_t piston = (x == 22 || x == 23 || x == 40 || x == 41) ? 140 : 50; // Chrome cylinder rod
                if (y >= 56) piston = 80; // Knee armor plate
                buf[y * TEX_SIZE + x] = rgba(piston, piston + 3, piston + 6, 255);
                continue;
            }

            bool leftFoot = (x >= 17 && x <= 27 && y >= 59 && y <= 63);
            bool rightFoot = (x >= 36 && x <= 46 && y >= 59 && y <= 63);
            if (leftFoot || rightFoot) {
                uint8_t talon = (y == 63) ? 160 : 60; // Metal talons touching floor
                buf[y * TEX_SIZE + x] = rgba(talon, talon + 2, talon + 4, 255);
                continue;
            }

            // -------------------------------------------------------------
            // 8. Forearms & Claws (for Unarmed melee stance)
            // -------------------------------------------------------------
            if (!armed) {
                // Heavy metallic claw fists hanging ready: x: 10..17 (left), x: 46..53 (right), y: 35..48
                bool leftArm = (x >= 10 && x <= 17 && y >= 35 && y <= 48);
                bool rightArm = (x >= 46 && x <= 53 && y >= 35 && y <= 48);
                if (leftArm || rightArm) {
                    uint8_t armMetal = 65;
                    if (y >= 44) armMetal = 130; // Heavy steel claw knuckles
                    buf[y * TEX_SIZE + x] = rgba(armMetal, armMetal + 2, armMetal + 5, 255);
                    continue;
                }
            }

            // -------------------------------------------------------------
            // 9. Massive Industrial Rotary Minigun (when armed == true)
            // As seen in image_6.png: Large cylindrical multi-barrel array
            // extending across x: 24..63, y: 37..49 with ammo drum at x: 22..33, y: 43..53
            // -------------------------------------------------------------
            if (armed) {
                // Circular Ammunition Drum under the receiver: x: 22..33, y: 43..53
                if (x >= 22 && x <= 33 && y >= 43 && y <= 53) {
                    int dx = x - 27;
                    int dy = y - 48;
                    if (dx * dx + dy * dy <= 28) {
                        uint8_t drum = (dx * dx + dy * dy <= 12) ? 75 : 42;
                        // Brass ammunition belt feed links
                        if (y == 44 || y == 45) {
                            buf[y * TEX_SIZE + x] = rgba(190, 150, 40, 255); // Brass bullet casing
                        } else {
                            buf[y * TEX_SIZE + x] = rgba(drum, drum + 2, drum + 5, 255);
                        }
                        continue;
                    }
                }

                // Minigun Receiver & Motor Housing: x: 25..38, y: 36..46
                if (x >= 25 && x <= 38 && y >= 36 && y <= 46) {
                    uint8_t body = 35;
                    if (y == 36 || y == 37) body = 80; // Top mounting bracket
                    if (x == 37 || x == 38) body = 60; // Barrel junction collar
                    buf[y * TEX_SIZE + x] = rgba(body, body + 2, body + 5, 255);
                    continue;
                }

                // Heavy 6-Barrel Rotary Gatling Cluster: x: 39..62, y: 38..48
                if (x >= 39 && x <= 62 && y >= 38 && y <= 48) {
                    // Barrel support discs (circular clamps at x: 43..45 and x: 57..59)
                    if ((x >= 43 && x <= 45) || (x >= 57 && x <= 59)) {
                        buf[y * TEX_SIZE + x] = rgba(85, 90, 100, 255); // Steel clamp ring
                        continue;
                    }

                    // Rotating barrel rows (horizontal fluted steel barrels)
                    if (y == 39 || y == 41 || y == 43 || y == 45 || y == 47) {
                        uint8_t barrelSteel = (y == 41 || y == 43) ? 140 : 100;
                        buf[y * TEX_SIZE + x] = rgba(barrelSteel, barrelSteel + 4, barrelSteel + 8, 255);
                    } else {
                        // Shadow between individual barrels
                        buf[y * TEX_SIZE + x] = rgba(20, 22, 26, 255);
                    }

                    // Rotary Muzzle Crown / Flash Hider: x: 61..62
                    if (x >= 61) {
                        buf[y * TEX_SIZE + x] = rgba(160, 165, 175, 255);
                    }
                    continue;
                }
            }
        }
    }
    m_textures[id] = buf;
}



void TextureManager::generateEnemyDeadSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Shattered metal debris pile on the floor: x: 10..53, y: 40..58
            if (x >= 10 && x <= 53 && y >= 40 && y <= 58) {
                int noise = (x * 19 + y * 23) % 17;
                if (noise < 12) {
                    uint8_t r = static_cast<uint8_t>(50 + noise * 5);
                    uint8_t g = static_cast<uint8_t>(52 + noise * 5);
                    uint8_t b = static_cast<uint8_t>(58 + noise * 5);

                    // Residual dying spark
                    if ((x == 32 || x == 33) && y == 47) {
                        buf[y * TEX_SIZE + x] = emissive(255, 60, 20);
                    } else {
                        buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
                    }
                }
            }
        }
    }
    m_textures[id] = buf;
}

void TextureManager::generateSurvivorSprite(int id) {
    generateSurvivorRifleSprite(id, false);
}

void TextureManager::generateSurvivorRifleSprite(int id, bool firing) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Tactical Comm Antenna: x: 22, y: 2..7
            if (x == 22 && y >= 2 && y <= 7) {
                buf[y * TEX_SIZE + x] = rgba(60, 65, 75, 255);
            }
            // Tactical Ballistic Marine Helmet (x: 21..42, y: 7..23)
            else if (x >= 21 && x <= 42 && y >= 7 && y <= 23) {
                uint8_t r = 36, g = 40, b = 48; // Matte dark tactical shell
                if (x == 21 || x == 42 || y == 7) {
                    r = 22; g = 25; b = 30; // Rim shadow
                } else if (y >= 12 && y <= 13) {
                    r = 55; g = 60; b = 72; // Goggle strap / brow highlight
                }

                // Glowing Neon Cyan Tactical Visor (x: 26..37, y: 14..18)
                if (x >= 26 && x <= 37 && y >= 14 && y <= 18) {
                    if (x >= 27 && x <= 29 && y == 15) {
                        buf[y * TEX_SIZE + x] = emissive(220, 255, 255); // Specular highlight
                    } else {
                        buf[y * TEX_SIZE + x] = emissive(0, 220, 255); // Cyan HUD optics
                    }
                    continue;
                }

                // Respirator & Chin Strap (x: 27..36, y: 19..23)
                if (x >= 27 && x <= 36 && y >= 19 && y <= 23) {
                    r = ((x + y) % 2 == 0) ? 35 : 55;
                    g = r + 4; b = r + 8;
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Armored Pauldrons (Shoulders): x: 13..21 & x: 42..50, y: 24..32
            else if ((x >= 13 && x <= 21 && y >= 24 && y <= 32) ||
                     (x >= 42 && x <= 50 && y >= 24 && y <= 32)) {
                uint8_t r = 32, g = 36, b = 44;
                if (x == 13 || x == 50 || y == 24) {
                    r = 20; g = 22; b = 28;
                }
                // Survivor orange identification chevron on left arm: x: 15..17, y: 26..28
                if (x >= 15 && x <= 17 && y >= 26 && y <= 28) {
                    buf[y * TEX_SIZE + x] = rgba(255, 140, 0, 255);
                    continue;
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Tactical Body Armor / Kevlar Vest (x: 19..44, y: 24..43)
            else if (x >= 19 && x <= 44 && y >= 24 && y <= 43) {
                uint8_t r = 38, g = 42, b = 50;

                // MOLLE webbing bands (y: 28, 33, 38)
                if (y == 28 || y == 33 || y == 38) {
                    r = 20; g = 22; b = 28;
                }
                // Ammo pouches (x: 25..38, y: 31..41)
                else if (x >= 25 && x <= 38 && y >= 31 && y <= 41) {
                    if (x == 25 || x == 31 || x == 38) {
                        r = 18; g = 20; b = 25; // Pouch separator
                    } else {
                        r = 48; g = 54; b = 64; // Heavy pouch fabric
                    }
                }

                // Vanguard Chest Beacon: x: 31..32, y: 26..27
                if (x >= 31 && x <= 32 && y >= 26 && y <= 27) {
                    buf[y * TEX_SIZE + x] = emissive(0, 255, 180);
                    continue;
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Hip-Mounted Tactical Flashlight Housing (x: 16..20, y: 40..45)
            else if (x >= 16 && x <= 20 && y >= 40 && y <= 45) {
                if (x == 20) {
                    buf[y * TEX_SIZE + x] = emissive(255, 250, 200); // Glowing projector lamp lens
                } else {
                    buf[y * TEX_SIZE + x] = rgba(50, 55, 60, 255); // Black metal casing
                }
            }
            // Tactical Assault Rifle held in forward firing stance: x: 22..52, y: 33..42
            else if (x >= 22 && x <= 52 && y >= 33 && y <= 42) {
                uint8_t r = 26, g = 28, b = 32;
                if (y == 35 || y == 36) {
                    r = 85; g = 95; b = 110; // Steel barrel reflection
                }
                if (x >= 48 && x <= 52 && (y == 35 || y == 36)) {
                    r = 120; g = 130; b = 145; // Flash suppressor
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Forearm Gauntlets & Tactical Gloves: x: 12..18 & x: 44..50, y: 33..43
            else if ((x >= 12 && x <= 18 && y >= 33 && y <= 43) ||
                     (x >= 44 && x <= 50 && y >= 33 && y <= 43)) {
                buf[y * TEX_SIZE + x] = rgba(28, 30, 35, 255);
            }
            // Combat Fatigues / Cargo Pants: x: 22..29 & x: 34..41, y: 44..56
            else if ((x >= 22 && x <= 29 && y >= 44 && y <= 56) ||
                     (x >= 34 && x <= 41 && y >= 44 && y <= 56)) {
                uint8_t r = 36, g = 42, b = 52; // Navy-slate tactical fabric
                // Reinforced composite knee pads: y: 48..52
                if (y >= 48 && y <= 52) {
                    r = 22; g = 24; b = 30; // Matte pad
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Combat Assault Boots: x: 20..29 & x: 34..43, y: 57..63
            else if ((x >= 20 && x <= 29 && y >= 57 && y <= 63) ||
                     (x >= 34 && x <= 43 && y >= 57 && y <= 63)) {
                uint8_t r = 18, g = 20, b = 24;
                if (y == 63) r = 40; // Heavy tread sole
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }

            // Blazing Muzzle Flash Starburst (when firing)
            if (firing && x >= 49 && x <= 63 && y >= 27 && y <= 43) {
                float mdx = static_cast<float>(x - 56);
                float mdy = static_cast<float>(y - 35);
                float mdist = std::sqrt(mdx * mdx + mdy * mdy);
                if (mdist <= 2.2f) {
                    buf[y * TEX_SIZE + x] = emissive(255, 255, 240); // Blinding white core
                } else if (mdist <= 5.5f) {
                    buf[y * TEX_SIZE + x] = emissive(255, 215, 60); // Golden plasma flame
                } else if (mdist <= 7.5f) {
                    buf[y * TEX_SIZE + x] = emissive(255, 120, 20); // Orange fringe flare
                }
            }
        }
    }
    m_textures[id] = buf;
}

void TextureManager::generateSurvivorPistolSprite(int id, bool firing) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Tactical Comm Antenna
            if (x == 22 && y >= 2 && y <= 7) {
                buf[y * TEX_SIZE + x] = rgba(60, 65, 75, 255);
            }
            // Helmet (x: 21..42, y: 7..23)
            else if (x >= 21 && x <= 42 && y >= 7 && y <= 23) {
                uint8_t r = 36;
                if (x == 21 || x == 42 || y == 7) r = 22;
                if (x >= 26 && x <= 37 && y >= 14 && y <= 18) {
                    buf[y * TEX_SIZE + x] = emissive(0, 220, 255);
                    continue;
                }
                buf[y * TEX_SIZE + x] = rgba(r, r + 4, r + 8, 255);
            }
            // Pauldrons
            else if ((x >= 14 && x <= 21 && y >= 24 && y <= 32) ||
                     (x >= 42 && x <= 49 && y >= 24 && y <= 32)) {
                buf[y * TEX_SIZE + x] = rgba(32, 36, 44, 255);
            }
            // Tactical Vest
            else if (x >= 19 && x <= 44 && y >= 24 && y <= 43) {
                uint8_t r = 38, g = 42, b = 50;
                if (y == 28 || y == 33 || y == 38) r = 20;
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Hip Flashlight
            else if (x >= 16 && x <= 20 && y >= 40 && y <= 45) {
                buf[y * TEX_SIZE + x] = (x == 20) ? emissive(255, 250, 200) : rgba(50, 55, 60, 255);
            }
            // Handgun weaver stance (x: 26..46, y: 34..42)
            else if (x >= 26 && x <= 46 && y >= 34 && y <= 42) {
                uint8_t r = 28, g = 30, b = 34;
                if (y == 36 && x >= 36) r = 90;
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Legs
            else if ((x >= 22 && x <= 29 && y >= 44 && y <= 56) ||
                     (x >= 34 && x <= 41 && y >= 44 && y <= 56)) {
                uint8_t r = (y >= 48 && y <= 52) ? 22 : 36;
                buf[y * TEX_SIZE + x] = rgba(r, r + 6, r + 16, 255);
            }
            // Boots
            else if ((x >= 20 && x <= 29 && y >= 57 && y <= 63) ||
                     (x >= 34 && x <= 43 && y >= 57 && y <= 63)) {
                buf[y * TEX_SIZE + x] = rgba(18, 20, 24, 255);
            }

            // Pistol Muzzle Flash
            if (firing && x >= 44 && x <= 56 && y >= 31 && y <= 41) {
                float mdx = static_cast<float>(x - 49);
                float mdy = static_cast<float>(y - 36);
                float mdist = std::sqrt(mdx * mdx + mdy * mdy);
                if (mdist <= 1.8f) {
                    buf[y * TEX_SIZE + x] = emissive(255, 255, 230);
                } else if (mdist <= 4.2f) {
                    buf[y * TEX_SIZE + x] = emissive(255, 160, 40);
                }
            }
        }
    }
    m_textures[id] = buf;
}

void TextureManager::generateSurvivorSniperSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Helmet (x: 21..42, y: 7..23)
            if (x >= 21 && x <= 42 && y >= 7 && y <= 23) {
                uint8_t r = 36, g = 40, b = 48;
                if (x >= 26 && x <= 37 && y >= 14 && y <= 18) {
                    buf[y * TEX_SIZE + x] = emissive(0, 220, 255);
                    continue;
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Pauldrons & Vest
            else if (x >= 16 && x <= 47 && y >= 24 && y <= 43) {
                uint8_t r = 38, g = 42, b = 50;
                if (y == 28 || y == 33 || y == 38) r = 20;
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Hip Flashlight
            else if (x >= 16 && x <= 20 && y >= 40 && y <= 45) {
                buf[y * TEX_SIZE + x] = (x == 20) ? emissive(255, 250, 200) : rgba(50, 55, 60, 255);
            }
            // Heavy Sniper Rifle with Elevated Optical Scope (x: 20..58, y: 31..42)
            else if (x >= 20 && x <= 58 && y >= 31 && y <= 42) {
                uint8_t r = 30, g = 32, b = 36;
                // Optical Scope at y: 31..33, x: 30..42
                if (y >= 31 && y <= 33 && x >= 30 && x <= 42) {
                    r = 50; g = 55; b = 65;
                    if (x == 42) {
                        buf[y * TEX_SIZE + x] = emissive(0, 240, 255); // Scope lens glint
                        continue;
                    }
                }
                if (y == 35 && x >= 46) r = 110; // Long steel barrel
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Legs & Boots
            else if ((x >= 22 && x <= 29 && y >= 44 && y <= 63) ||
                     (x >= 34 && x <= 41 && y >= 44 && y <= 63)) {
                uint8_t r = (y >= 57) ? 18 : ((y >= 48 && y <= 52) ? 22 : 36);
                buf[y * TEX_SIZE + x] = rgba(r, r + 4, r + 10, 255);
            }
        }
    }
    m_textures[id] = buf;
}

void TextureManager::generateSurvivorDownedSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Downed / Crawling Posture: lower center of gravity
            // Slumped Helmet (x: 20..38, y: 24..38)
            if (x >= 20 && x <= 38 && y >= 24 && y <= 38) {
                uint8_t r = 35, g = 38, b = 45;
                if (x == 20 || x == 38 || y == 24) r = 20;

                // Flickering Red Emergency Bio-Pulse Visor (x: 22..32, y: 30..34)
                if (x >= 22 && x <= 32 && y >= 30 && y <= 34) {
                    buf[y * TEX_SIZE + x] = emissive(255, 40, 40); // Pulsing warning red
                    continue;
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Hunched Body Armor & Shoulders (x: 18..46, y: 36..52)
            else if (x >= 18 && x <= 46 && y >= 36 && y <= 52) {
                uint8_t r = 34, g = 38, b = 46;
                // Orange chevron
                if (x >= 18 && x <= 20 && y >= 38 && y <= 40) {
                    buf[y * TEX_SIZE + x] = rgba(255, 140, 0, 255);
                    continue;
                }
                buf[y * TEX_SIZE + x] = rgba(r, g, b, 255);
            }
            // Arms Supporting Body on the Ground (x: 40..52, y: 48..60)
            else if (x >= 40 && x <= 52 && y >= 48 && y <= 60) {
                uint8_t r = (y >= 56) ? 22 : 32; // Glove palm on floor
                buf[y * TEX_SIZE + x] = rgba(r, r + 4, r + 8, 255);
            }
            // Folded Crawling Legs & Knees on Floor (x: 12..36, y: 50..63)
            else if (x >= 12 && x <= 36 && y >= 50 && y <= 63) {
                uint8_t r = (y >= 58) ? 18 : 34; // Combat trousers and boots on floor
                buf[y * TEX_SIZE + x] = rgba(r, r + 4, r + 10, 255);
            }
        }
    }
    m_textures[id] = buf;
}

void TextureManager::generateSurvivorDeadSprite(int id) {
    std::vector<uint32_t> buf(TEX_SIZE * TEX_SIZE, 0);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            // Horizontal prone fallen silhouette (y: 50..63, x: 8..58)
            if (y >= 50 && y <= 63 && x >= 8 && x <= 58) {
                // Pool of dark coagulated blood underneath
                if (y >= 58 && x >= 14 && x <= 52) {
                    buf[y * TEX_SIZE + x] = rgba(70, 8, 8, 255);
                    continue;
                }
                // Cracked helmet at x: 10..22
                if (x >= 10 && x <= 22 && y >= 50 && y <= 57) {
                    buf[y * TEX_SIZE + x] = rgba(30, 32, 38, 255);
                    continue;
                }
                // Torso & legs
                buf[y * TEX_SIZE + x] = rgba(25, 28, 34, 255);
            }
        }
    }
    m_textures[id] = buf;
}
