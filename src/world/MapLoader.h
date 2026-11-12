#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <QString>
#include <QPoint>
#include <vector>
#include <stdexcept>
#include <string>

class Map;
class TaskManager;

class MapLoaderException : public std::runtime_error {
public:
    explicit MapLoaderException(const std::string& message)
        : std::runtime_error(message) {}
};

struct MapLoadResult {
    bool success = false;
    QString errorMessage;
    int width = 0;
    int height = 0;
    float spawnX = 16.5f;
    float spawnY = 16.5f;
    int serverGatewayX = -1;
    int serverGatewayY = -1;
    std::vector<QPoint> electricalBoxes;
    std::vector<QPoint> vaultPanels;
    std::vector<QPoint> vaultDoors;
};

class MapLoader {
public:
    // Parses and validates an external .txt map, populating the Map and TaskManager
    static MapLoadResult loadMap(const QString& filePath, Map& outMap, TaskManager& outTaskManager);

private:
    static void configureLighting(Map& map, const MapLoadResult& res);
};

#endif // MAP_LOADER_H

