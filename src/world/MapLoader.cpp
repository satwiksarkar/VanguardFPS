#include "MapLoader.h"
#include "world/Map.h"
#include "world/TaskManager.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

MapLoadResult MapLoader::loadMap(const QString& filePath, Map& outMap, TaskManager& outTaskManager) {
    MapLoadResult res;

    // Resolve path: check direct path, Qt resource path, and local disk assets path
    QString resolvedPath = filePath;
    if (!QFile::exists(resolvedPath)) {
        QString candidateQrc = ":/assets/maps/" + filePath;
        QString candidateDisk = "assets/maps/" + filePath;
        QString candidateQrcDirect = ":/" + filePath;

        if (QFile::exists(candidateQrc)) {
            resolvedPath = candidateQrc;
        } else if (QFile::exists(candidateDisk)) {
            resolvedPath = candidateDisk;
        } else if (QFile::exists(candidateQrcDirect)) {
            resolvedPath = candidateQrcDirect;
        } else {
            QString err = QString("MapLoader Error: Map file not found: %1").arg(filePath);
            qWarning() << err;
            throw MapLoaderException(err.toStdString());
        }
    }

    QFile file(resolvedPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err = QString("MapLoader Error: Failed to open map file: %1 (%2)").arg(resolvedPath, file.errorString());
        qWarning() << err;
        throw MapLoaderException(err.toStdString());
    }

    QTextStream in(&file);
    std::vector<QString> lines;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Skip comment lines and blank lines
        if (line.isEmpty() || line.startsWith('#') || line.startsWith("//")) {
            continue;
        }
        lines.push_back(line);
    }
    file.close();

    if (lines.empty()) {
        QString err = QString("MapLoader Error: Map file is empty: %1").arg(resolvedPath);
        qWarning() << err;
        throw MapLoaderException(err.toStdString());
    }

    int height = static_cast<int>(lines.size());
    int width = static_cast<int>(lines[0].length());

    // Validate consistent row widths
    for (int y = 0; y < height; ++y) {
        if (lines[y].length() != width) {
            QString err = QString("MapLoader Error: Malformed map! Row %1 length (%2) does not match expected width (%3)")
                              .arg(y).arg(lines[y].length()).arg(width);
            qWarning() << err;
            throw MapLoaderException(err.toStdString());
        }
    }

    res.width = width;
    res.height = height;

    bool hasSpawn = false;
    bool hasGateway = false;

    outMap.resize(width, height);
    outMap.clear(1); // Default to solid wall

    // Parse grid characters according to specification legend
    for (int y = 0; y < height; ++y) {
        const QString& row = lines[y];
        for (int x = 0; x < width; ++x) {
            QChar ch = row[x];

            switch (ch.toLatin1()) {
                case '0':
                    // Floor
                    outMap.setTile(x, y, 0);
                    break;

                case '1':
                    // Standard Metal Wall
                    outMap.setTile(x, y, 1);
                    break;

                case '2':
                    // Hazard Warning Wall
                    outMap.setTile(x, y, 2);
                    break;

                case '3':
                    // Vanguard Server Rack
                    outMap.setTile(x, y, 3);
                    break;

                case '4':
                    // Maintenance Concrete Wall
                    outMap.setTile(x, y, 4);
                    break;

                case 'S':
                case 's':
                    // Player Spawn Point
                    outMap.setTile(x, y, 0); // Walkable floor under player
                    res.spawnX = x + 0.5f;
                    res.spawnY = y + 0.5f;
                    hasSpawn = true;
                    break;

                case 'E':
                case 'e':
                    // Electrical Box (Maintenance Terminal)
                    res.electricalBoxes.emplace_back(x, y);
                    break;

                case 'V':
                case 'v':
                    // Vault Control Panel
                    res.vaultPanels.emplace_back(x, y);
                    break;

                case 'D':
                case 'd':
                    // Locked Vault Blast Door
                    res.vaultDoors.emplace_back(x, y);
                    break;

                case 'X':
                case 'x':
                    // Main Server Gateway
                    res.serverGatewayX = x;
                    res.serverGatewayY = y;
                    hasGateway = true;
                    outMap.setTile(x, y, 99);
                    break;

                default:
                    // Treat unknown non-space characters as wall
                    outMap.setTile(x, y, (ch == ' ') ? 0 : 1);
                    break;
            }
        }
    }

    // Validation rules
    if (!hasSpawn) {
        QString err = QString("MapLoader Error: Malformed map! Missing Player Spawn Point ('S') in %1").arg(resolvedPath);
        qWarning() << err;
        throw MapLoaderException(err.toStdString());
    }

    if (!hasGateway) {
        QString err = QString("MapLoader Error: Malformed map! Missing Main Server Gateway ('X') in %1").arg(resolvedPath);
        qWarning() << err;
        throw MapLoaderException(err.toStdString());
    }

    if (res.electricalBoxes.size() < 4) {
        QString err = QString("MapLoader Error: Incomplete map! Requires exactly 4 Electrical Boxes ('E'), found %1 in %2")
                          .arg(res.electricalBoxes.size()).arg(resolvedPath);
        qWarning() << err;
        throw MapLoaderException(err.toStdString());
    }

    if (res.vaultPanels.size() < 4) {
        QString err = QString("MapLoader Error: Incomplete map! Requires exactly 4 Vault Control Panels ('V'), found %1 in %2")
                          .arg(res.vaultPanels.size()).arg(resolvedPath);
        qWarning() << err;
        throw MapLoaderException(err.toStdString());
    }

    if (res.vaultDoors.size() < 4) {
        QString err = QString("MapLoader Error: Incomplete map! Requires exactly 4 Vault Doors ('D'), found %1 in %2")
                          .arg(res.vaultDoors.size()).arg(resolvedPath);
        qWarning() << err;
        throw MapLoaderException(err.toStdString());
    }

    // Configure TaskManager with parsed entity positions (up to 4 sectors)
    for (int i = 0; i < TaskManager::SECTOR_COUNT; ++i) {
        // Electrical boxes: tile IDs 20..23
        int ex = res.electricalBoxes[i].x();
        int ey = res.electricalBoxes[i].y();
        outMap.setTile(ex, ey, 20 + i);
        outTaskManager.setElectricalCoord(i, ex, ey);

        // Vault Control Panels: tile IDs 30..33
        int px = res.vaultPanels[i].x();
        int py = res.vaultPanels[i].y();
        outMap.setTile(px, py, 30 + i);
        outTaskManager.setPanelCoord(i, px, py);

        // Vault Blast Doors: tile IDs 10..13
        int dx = res.vaultDoors[i].x();
        int dy = res.vaultDoors[i].y();
        outMap.setTile(dx, dy, 10 + i);
        outTaskManager.setVaultDoorCoord(i, dx, dy);
    }

    outTaskManager.setServerGatewayCoord(res.serverGatewayX, res.serverGatewayY);

    // Apply atmospheric dark lighting map
    configureLighting(outMap, res);

    res.success = true;
    qDebug() << QString("MapLoader: Successfully loaded '%1' (%2x%3) with 4 sectors and gateway.")
                    .arg(resolvedPath).arg(width).arg(height);
    return res;
}

void MapLoader::configureLighting(Map& map, const MapLoadResult& res) {
    int w = map.getWidth();
    int h = map.getHeight();

    // Default all tiles to OFF (atmospheric dark corridors)
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            map.setLightState(x, y, LightState::OFF);
        }
    }

    // 1. Central spawn lighting
    int sx = static_cast<int>(res.spawnX);
    int sy = static_cast<int>(res.spawnY);
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            map.setLightState(sx + dx, sy + dy, LightState::LOW_GLOW);
        }
    }

    // 2. Gateway approach lighting
    if (res.serverGatewayX >= 0 && res.serverGatewayY >= 0) {
        map.setLightState(res.serverGatewayX, res.serverGatewayY + 1, LightState::LOW_GLOW);
        map.setLightState(res.serverGatewayX, res.serverGatewayY + 2, LightState::FLICKERING);
    }

    // 3. Electrical box rooms (flickering fluorescent bulbs)
    for (size_t i = 0; i < res.electricalBoxes.size() && i < 4; ++i) {
        int ex = res.electricalBoxes[i].x();
        int ey = res.electricalBoxes[i].y();
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (map.getTile(ex + dx, ey + dy) == 0) {
                    map.setLightState(ex + dx, ey + dy, LightState::FLICKERING);
                }
            }
        }
    }

    // 4. Vault entrances
    for (size_t i = 0; i < res.vaultDoors.size() && i < 4; ++i) {
        int vx = res.vaultDoors[i].x();
        int vy = res.vaultDoors[i].y();
        map.setLightState(vx, vy, LightState::LOW_GLOW);
    }
}

