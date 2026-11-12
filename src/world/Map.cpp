#include "Map.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdlib>

Map::Map()
    : Map(DEFAULT_WIDTH, DEFAULT_HEIGHT)
{
    initializeMap();
    initializeLights();
}

Map::Map(int width, int height)
    : m_width(width)
    , m_height(height)
{
    resize(width, height);
}

void Map::resize(int width, int height) {
    m_width = std::max(1, width);
    m_height = std::max(1, height);

    m_tiles.assign(m_width, std::vector<int>(m_height, 1));
    m_lightStates.assign(m_width, std::vector<LightState>(m_height, LightState::OFF));
    m_flickerPhase.assign(m_width, std::vector<float>(m_height, 0.0f));

    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_height; ++y) {
            m_flickerPhase[x][y] = static_cast<float>(rand() % 1000) / 100.0f;
        }
    }
}

void Map::clear(int fillTile) {
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_height; ++y) {
            m_tiles[x][y] = fillTile;
            m_lightStates[x][y] = LightState::OFF;
        }
    }
}

int Map::getTile(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return 1; // Outer boundary is solid wall
    }
    return m_tiles[x][y];
}

void Map::setTile(int x, int y, int tile) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_tiles[x][y] = tile;
    }
}

bool Map::isWall(int x, int y) const {
    int tile = getTile(x, y);
    return tile != 0;
}

bool Map::isWalkable(float x, float y, float radius) const {
    int minX = static_cast<int>(std::floor(x - radius));
    int maxX = static_cast<int>(std::floor(x + radius));
    int minY = static_cast<int>(std::floor(y - radius));
    int maxY = static_cast<int>(std::floor(y + radius));

    for (int ix = minX; ix <= maxX; ++ix) {
        for (int iy = minY; iy <= maxY; ++iy) {
            if (isWall(ix, iy)) {
                return false;
            }
        }
    }
    return true;
}

LightState Map::getLightState(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return LightState::OFF;
    }
    return m_lightStates[x][y];
}

void Map::setLightState(int x, int y, LightState state) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_lightStates[x][y] = state;
    }
}

float Map::getLightIntensity(int x, int y, float timeSeconds) const {
    LightState state = getLightState(x, y);
    switch (state) {
        case LightState::BRIGHT_RESTORED:
            return 0.85f;
        case LightState::LOW_GLOW:
            return 0.35f;
        case LightState::FLICKERING: {
            float phase = (x >= 0 && x < m_width && y >= 0 && y < m_height) ? m_flickerPhase[x][y] : 0.0f;
            // Dual frequency harmonic oscillation simulating faulty fluorescent gas discharge
            float osc = 0.5f + 0.35f * std::sin(timeSeconds * 8.5f + phase)
                             + 0.15f * std::sin(timeSeconds * 23.3f + phase * 3.7f);
            osc = std::clamp(osc, 0.0f, 1.0f);
            // Range 0.12f to 0.45f
            return 0.12f + osc * 0.33f;
        }
        case LightState::OFF:
        default:
            return 0.08f;
    }
}

void Map::restoreSectorLighting(int sector) {
    if (sector < 0 || sector >= 4) return;

    int halfW = m_width / 2;
    int halfH = m_height / 2;

    int x0 = (sector == 1 || sector == 3) ? halfW : 0;
    int x1 = (sector == 1 || sector == 3) ? m_width : halfW;
    int y0 = (sector == 2 || sector == 3) ? halfH : 0;
    int y1 = (sector == 2 || sector == 3) ? m_height : halfH;

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            if (x > 0 && x < m_width - 1 && y > 0 && y < m_height - 1) {
                m_lightStates[x][y] = LightState::BRIGHT_RESTORED;
            }
        }
    }
}

std::string Map::getTileName(int tileId) {
    switch (tileId) {
        case 1:  return "Reinforced Lab Wall";
        case 2:  return "Hazard Warning Wall";
        case 3:  return "Vanguard Server Rack";
        case 4:  return "Maintenance Concrete Wall";
        case 10: return "Vault 1 Blast Door [LOCKED]";
        case 11: return "Vault 2 Blast Door [LOCKED]";
        case 12: return "Vault 3 Blast Door [LOCKED]";
        case 13: return "Vault 4 Blast Door [LOCKED]";
        case 20: return "Electrical Terminal 1 [MAINTENANCE NW]";
        case 21: return "Electrical Terminal 2 [MAINTENANCE NE]";
        case 22: return "Electrical Terminal 3 [MAINTENANCE SW]";
        case 23: return "Electrical Terminal 4 [MAINTENANCE SE]";
        case 30: return "Vault 1 Security Panel";
        case 31: return "Vault 2 Security Panel";
        case 32: return "Vault 3 Security Panel";
        case 33: return "Vault 4 Security Panel";
        case 99: return "Main Server Room Bulkhead [SEALED - 4 KEYS REQUIRED]";
        default: return "Floor";
    }
}

bool Map::isInteractive(int tileId) {
    return (tileId >= 10 && tileId <= 13) ||
           (tileId >= 20 && tileId <= 23) ||
           (tileId >= 30 && tileId <= 33) ||
           (tileId == 99);
}

void Map::initializeMap() {
    clear(1);

    auto carveRoom = [this](int x0, int y0, int x1, int y1) {
        for (int x = x0; x <= x1; ++x) {
            for (int y = y0; y <= y1; ++y) {
                if (x > 0 && x < m_width - 1 && y > 0 && y < m_height - 1) {
                    m_tiles[x][y] = 0;
                }
            }
        }
    };

    auto carveHCorridor = [this](int x0, int x1, int y) {
        int start = std::min(x0, x1);
        int end = std::max(x0, x1);
        for (int x = start; x <= end; ++x) {
            if (x > 0 && x < m_width - 1 && y > 0 && y < m_height - 1) {
                m_tiles[x][y] = 0;
            }
        }
    };

    auto carveVCorridor = [this](int x, int y0, int y1) {
        int start = std::min(y0, y1);
        int end = std::max(y0, y1);
        for (int y = start; y <= end; ++y) {
            if (x > 0 && x < m_width - 1 && y > 0 && y < m_height - 1) {
                m_tiles[x][y] = 0;
            }
        }
    };

    // 1. Central Spawn Room (14-18, 14-18)
    carveRoom(14, 14, 18, 18);

    // Corridors exiting Central Spawn Room
    carveVCorridor(16, 10, 14); // North
    carveVCorridor(16, 18, 22); // South
    carveHCorridor(10, 14, 16); // West
    carveHCorridor(18, 22, 16); // East

    // Inner ring corridors connecting the 4 sectors
    carveHCorridor(10, 22, 10);
    carveHCorridor(10, 22, 22);
    carveVCorridor(10, 10, 22);
    carveVCorridor(22, 10, 22);

    // North Sector: Path to Main Server Room
    carveVCorridor(16, 5, 10);
    carveRoom(14, 1, 18, 3); // Main Server Room chamber
    // Exit door at (16, 4)
    m_tiles[16][4] = 99;
    m_tiles[15][4] = 2; // Hazard wall flanking door
    m_tiles[17][4] = 2; // Hazard wall flanking door
    // Vanguard Server Racks inside Server Room
    m_tiles[14][2] = 3;
    m_tiles[18][2] = 3;
    m_tiles[16][1] = 3;

    // --- Sector 1 (North-West) ---
    carveHCorridor(5, 10, 10);
    carveVCorridor(8, 7, 10); // Corridor approaching Vault 1 Door
    carveVCorridor(5, 8, 10); // Corridor to Electrical Box 1
    carveRoom(4, 3, 8, 5);    // Vault 1 completely enclosed sealed room
    m_tiles[8][6] = 10;       // Vault Door 1
    m_tiles[7][6] = 30;       // Vault Control Panel 1
    m_tiles[6][6] = 2;        // Hazard wall
    m_tiles[5][6] = 1;        // Perimeter wall sealing room
    m_tiles[9][6] = 2;        // Hazard wall
    carveRoom(2, 8, 4, 11);
    m_tiles[1][8] = 4;
    m_tiles[1][9] = 20; // Electrical Box 1
    m_tiles[1][10] = 4;
    m_tiles[1][11] = 4;

    // --- Sector 2 (North-East) ---
    carveHCorridor(22, 27, 10);
    carveVCorridor(24, 7, 10); // Corridor approaching Vault 2 Door
    carveVCorridor(27, 8, 10); // Corridor to Electrical Box 2
    carveRoom(24, 3, 28, 5);   // Vault 2 completely enclosed sealed room
    m_tiles[24][6] = 11;       // Vault Door 2
    m_tiles[25][6] = 31;       // Vault Control Panel 2
    m_tiles[23][6] = 2;        // Hazard wall
    m_tiles[26][6] = 1;        // Perimeter wall sealing room
    m_tiles[27][6] = 1;        // Perimeter wall sealing room
    carveRoom(27, 8, 29, 11);
    m_tiles[30][8] = 4;
    m_tiles[30][9] = 21; // Electrical Box 2
    m_tiles[30][10] = 4;
    m_tiles[30][11] = 4;

    // --- Sector 3 (South-West) ---
    carveHCorridor(5, 10, 22);
    carveVCorridor(8, 23, 25); // Corridor approaching Vault 3 Door
    carveVCorridor(5, 21, 24); // Corridor to Electrical Box 3
    carveRoom(4, 27, 8, 29);   // Vault 3 completely enclosed sealed room
    m_tiles[8][26] = 12;       // Vault Door 3
    m_tiles[7][26] = 32;       // Vault Control Panel 3
    m_tiles[6][26] = 2;        // Hazard wall
    m_tiles[5][26] = 1;        // Perimeter wall sealing room
    m_tiles[9][26] = 2;        // Hazard wall
    carveRoom(2, 21, 4, 24);
    m_tiles[1][21] = 4;
    m_tiles[1][22] = 22; // Electrical Box 3
    m_tiles[1][23] = 4;
    m_tiles[1][24] = 4;

    // --- Sector 4 (South-East) ---
    carveHCorridor(22, 27, 22);
    carveVCorridor(24, 23, 25); // Corridor approaching Vault 4 Door
    carveVCorridor(27, 21, 24); // Corridor to Electrical Box 4
    carveRoom(24, 27, 28, 29);  // Vault 4 completely enclosed sealed room
    m_tiles[24][26] = 13;       // Vault Door 4
    m_tiles[25][26] = 33;       // Vault Control Panel 4
    m_tiles[23][26] = 2;        // Hazard wall
    m_tiles[26][26] = 1;        // Perimeter wall sealing room
    m_tiles[27][26] = 1;        // Perimeter wall sealing room
    carveRoom(27, 21, 29, 24);
    m_tiles[30][21] = 4;
    m_tiles[30][22] = 23; // Electrical Box 4
    m_tiles[30][23] = 4;
    m_tiles[30][24] = 4;

    // Consoles along the outer rim
    m_tiles[13][14] = 3;
    m_tiles[19][14] = 3;
    m_tiles[13][18] = 3;
    m_tiles[19][18] = 3;
}

void Map::initializeLights() {
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_height; ++y) {
            m_lightStates[x][y] = LightState::OFF;
        }
    }

    // Default Level 1 key light fixtures (if within bounds)
    auto setGlow = [this](int x, int y, LightState s) {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            m_lightStates[x][y] = s;
        }
    };

    setGlow(16, 16, LightState::LOW_GLOW);
    setGlow(15, 16, LightState::LOW_GLOW);
    setGlow(17, 16, LightState::LOW_GLOW);
    setGlow(16, 15, LightState::LOW_GLOW);
    setGlow(16, 17, LightState::LOW_GLOW);

    setGlow(16, 12, LightState::FLICKERING);
    setGlow(16, 20, LightState::FLICKERING);

    setGlow(16, 5, LightState::FLICKERING);
    setGlow(16, 6, LightState::LOW_GLOW);

    setGlow(8, 7, LightState::LOW_GLOW);
    setGlow(24, 7, LightState::LOW_GLOW);
    setGlow(8, 25, LightState::LOW_GLOW);
    setGlow(24, 25, LightState::LOW_GLOW);

    setGlow(6, 4, LightState::LOW_GLOW);
    setGlow(26, 4, LightState::LOW_GLOW);
    setGlow(6, 28, LightState::LOW_GLOW);
    setGlow(26, 28, LightState::LOW_GLOW);

    setGlow(3, 9, LightState::FLICKERING);
    setGlow(28, 9, LightState::FLICKERING);
    setGlow(3, 22, LightState::FLICKERING);
    setGlow(28, 22, LightState::FLICKERING);

    setGlow(10, 16, LightState::LOW_GLOW);
    setGlow(22, 16, LightState::LOW_GLOW);
}
