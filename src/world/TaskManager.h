#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <string>

class Map;

struct SectorProgress {
    bool electricalFixed = false;
    bool riddleSolved = false;
    bool vaultDoorOpen = false;
    float doorSlideTimer = 0.0f;
    bool isDoorSliding = false;
};

struct TaskEntityCoord {
    int x = 0;
    int y = 0;
};

class TaskManager {
public:
    static constexpr int SECTOR_COUNT = 4;

    TaskManager();

    // Frame update for sliding doors & mechanisms
    void update(float dt, Map& map);

    // Dynamic Entity Coordinate Configuration
    void setVaultDoorCoord(int sector, int x, int y);
    void setElectricalCoord(int sector, int x, int y);
    void setPanelCoord(int sector, int x, int y);
    void setServerGatewayCoord(int x, int y);

    void getVaultDoorCoord(int sector, int& outX, int& outY) const;
    void getElectricalCoord(int sector, int& outX, int& outY) const;
    void getPanelCoord(int sector, int& outX, int& outY) const;
    void getServerGatewayCoord(int& outX, int& outY) const;

    // Sector query / setters
    bool isElectricalFixed(int sector) const;
    void setElectricalFixed(int sector, bool fixed, Map& map);

    bool isRiddleSolved(int sector) const;
    void setRiddleSolved(int sector, bool solved, Map& map);

    bool isVaultDoorOpen(int sector) const;
    void setVaultDoorOpen(int sector, bool open, Map& map);

    // Mechanical sliding door animation query
    bool isDoorSliding(int sector) const;
    float getDoorSlideProgress(int sector) const;

    // Mappings
    static int tileToSector(int tileId);
    static std::string getSectorName(int sector);

    // Summary counts
    int getFixedElectricalCount() const;
    int getSolvedRiddlesCount() const;
    int getOpenVaultsCount() const;
    bool areAllVaultDoorsOpen() const;

    void reset(Map& map);

private:
    void applySectorLighting(int sector, Map& map);

    std::array<SectorProgress, SECTOR_COUNT> m_sectors;

    std::array<TaskEntityCoord, SECTOR_COUNT> m_vaultDoors;
    std::array<TaskEntityCoord, SECTOR_COUNT> m_electricalBoxes;
    std::array<TaskEntityCoord, SECTOR_COUNT> m_vaultPanels;
    TaskEntityCoord m_serverGateway{ 16, 4 };
};

#endif // TASK_MANAGER_H
