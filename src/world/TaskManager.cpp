#include "TaskManager.h"
#include "world/Map.h"
#include "audio/AudioManager.h"

TaskManager::TaskManager() {
    // Default Level 1 layout fallback coordinates
    m_vaultDoors[0] = { 8, 6 };
    m_vaultDoors[1] = { 24, 6 };
    m_vaultDoors[2] = { 8, 26 };
    m_vaultDoors[3] = { 24, 26 };

    m_electricalBoxes[0] = { 1, 9 };
    m_electricalBoxes[1] = { 30, 9 };
    m_electricalBoxes[2] = { 1, 22 };
    m_electricalBoxes[3] = { 30, 22 };

    m_vaultPanels[0] = { 7, 6 };
    m_vaultPanels[1] = { 25, 6 };
    m_vaultPanels[2] = { 7, 26 };
    m_vaultPanels[3] = { 25, 26 };

    m_serverGateway = { 16, 4 };

    for (auto& s : m_sectors) {
        s = SectorProgress();
    }
}

void TaskManager::setVaultDoorCoord(int sector, int x, int y) {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        m_vaultDoors[sector] = { x, y };
    }
}

void TaskManager::setElectricalCoord(int sector, int x, int y) {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        m_electricalBoxes[sector] = { x, y };
    }
}

void TaskManager::setPanelCoord(int sector, int x, int y) {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        m_vaultPanels[sector] = { x, y };
    }
}

void TaskManager::setServerGatewayCoord(int x, int y) {
    m_serverGateway = { x, y };
}

void TaskManager::getVaultDoorCoord(int sector, int& outX, int& outY) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        outX = m_vaultDoors[sector].x;
        outY = m_vaultDoors[sector].y;
    } else {
        outX = 8;
        outY = 6;
    }
}

void TaskManager::getElectricalCoord(int sector, int& outX, int& outY) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        outX = m_electricalBoxes[sector].x;
        outY = m_electricalBoxes[sector].y;
    } else {
        outX = 1;
        outY = 9;
    }
}

void TaskManager::getPanelCoord(int sector, int& outX, int& outY) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        outX = m_vaultPanels[sector].x;
        outY = m_vaultPanels[sector].y;
    } else {
        outX = 7;
        outY = 6;
    }
}

void TaskManager::getServerGatewayCoord(int& outX, int& outY) const {
    outX = m_serverGateway.x;
    outY = m_serverGateway.y;
}

bool TaskManager::isElectricalFixed(int sector) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        return m_sectors[sector].electricalFixed;
    }
    return false;
}

void TaskManager::setElectricalFixed(int sector, bool fixed, Map& map) {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        m_sectors[sector].electricalFixed = fixed;
        if (fixed) {
            applySectorLighting(sector, map);
        }
    }
}

void TaskManager::update(float dt, Map& map) {
    for (int i = 0; i < SECTOR_COUNT; ++i) {
        if (m_sectors[i].isDoorSliding) {
            m_sectors[i].doorSlideTimer += dt;
            if (m_sectors[i].doorSlideTimer >= 0.85f) {
                m_sectors[i].isDoorSliding = false;
                m_sectors[i].doorSlideTimer = 0.85f;
                int dx, dy;
                getVaultDoorCoord(i, dx, dy);
                map.setTile(dx, dy, 0); // Open the door completely into walkable floor!
                m_sectors[i].vaultDoorOpen = true;
            }
        }
    }
}

bool TaskManager::isRiddleSolved(int sector) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        return m_sectors[sector].riddleSolved;
    }
    return false;
}

void TaskManager::setRiddleSolved(int sector, bool solved, Map& map) {
    (void)map;
    if (sector >= 0 && sector < SECTOR_COUNT) {
        m_sectors[sector].riddleSolved = solved;
        if (solved) {
            int dx, dy;
            getVaultDoorCoord(sector, dx, dy);
            m_sectors[sector].isDoorSliding = true;
            m_sectors[sector].doorSlideTimer = 0.0f;
            AudioManager::instance().playSoundAt(SoundType::VAULT_DOOR_OPEN, dx + 0.5f, dy + 0.5f);
        }
    }
}

bool TaskManager::isVaultDoorOpen(int sector) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        return m_sectors[sector].vaultDoorOpen;
    }
    return false;
}

void TaskManager::setVaultDoorOpen(int sector, bool open, Map& map) {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        m_sectors[sector].vaultDoorOpen = open;
        m_sectors[sector].isDoorSliding = false;
        m_sectors[sector].doorSlideTimer = open ? 0.85f : 0.0f;
        int dx, dy;
        getVaultDoorCoord(sector, dx, dy);
        map.setTile(dx, dy, open ? 0 : (10 + sector)); // Open floor or restore blast door
        if (open) {
            AudioManager::instance().playSoundAt(SoundType::VAULT_DOOR_OPEN, dx + 0.5f, dy + 0.5f);
        }
    }
}

bool TaskManager::isDoorSliding(int sector) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        return m_sectors[sector].isDoorSliding;
    }
    return false;
}

float TaskManager::getDoorSlideProgress(int sector) const {
    if (sector >= 0 && sector < SECTOR_COUNT) {
        if (m_sectors[sector].vaultDoorOpen) return 1.0f;
        if (m_sectors[sector].isDoorSliding) {
            return std::clamp(m_sectors[sector].doorSlideTimer / 0.85f, 0.0f, 1.0f);
        }
    }
    return 0.0f;
}

int TaskManager::tileToSector(int tileId) {
    switch (tileId) {
        // Electrical Boxes
        case 20: return 0;
        case 21: return 1;
        case 22: return 2;
        case 23: return 3;
        // Vault Control Panels
        case 30: return 0;
        case 31: return 1;
        case 32: return 2;
        case 33: return 3;
        // Vault Doors
        case 10: return 0;
        case 11: return 1;
        case 12: return 2;
        case 13: return 3;
        default: return -1;
    }
}

std::string TaskManager::getSectorName(int sector) {
    switch (sector) {
        case 0: return "Sector 1 [North-West / Cyan]";
        case 1: return "Sector 2 [North-East / Orange]";
        case 2: return "Sector 3 [South-West / Green]";
        case 3: return "Sector 4 [South-East / Purple]";
        default: return "Unknown Sector";
    }
}

int TaskManager::getFixedElectricalCount() const {
    int c = 0;
    for (const auto& s : m_sectors) {
        if (s.electricalFixed) c++;
    }
    return c;
}

int TaskManager::getSolvedRiddlesCount() const {
    int c = 0;
    for (const auto& s : m_sectors) {
        if (s.riddleSolved) c++;
    }
    return c;
}

int TaskManager::getOpenVaultsCount() const {
    int c = 0;
    for (const auto& s : m_sectors) {
        if (s.vaultDoorOpen) c++;
    }
    return c;
}

bool TaskManager::areAllVaultDoorsOpen() const {
    return getOpenVaultsCount() >= SECTOR_COUNT;
}

void TaskManager::reset(Map& map) {
    for (int i = 0; i < SECTOR_COUNT; ++i) {
        m_sectors[i] = SectorProgress();
        int dx, dy;
        getVaultDoorCoord(i, dx, dy);
        map.setTile(dx, dy, 10 + i); // Restore locked doors
    }
}

void TaskManager::applySectorLighting(int sector, Map& map) {
    if (sector < 0 || sector >= SECTOR_COUNT) return;

    // Restore full warm lighting (0.85 brightness) across the entire sector quadrant
    map.restoreSectorLighting(sector);

    // Ensure bright illumination on the electrical box and vault panel
    int ex = m_electricalBoxes[sector].x;
    int ey = m_electricalBoxes[sector].y;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            map.setLightState(ex + dx, ey + dy, LightState::BRIGHT_RESTORED);
        }
    }

    int px = m_vaultPanels[sector].x;
    int py = m_vaultPanels[sector].y;
    map.setLightState(px, py, LightState::BRIGHT_RESTORED);
}
