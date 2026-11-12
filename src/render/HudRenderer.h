#ifndef HUD_RENDERER_H
#define HUD_RENDERER_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <QPainter>
#include <QColor>
#include <QRect>
#include <QRectF>
#include <QPointF>
#include <QFontDatabase>

#include "world/Map.h"
#include "world/TaskManager.h"
#include "game/Player.h"
#include "net/NetProtocol.h"

class HudRenderer {
public:
    static constexpr int MINIMAP_SIZE = 150;
    static constexpr float VIEW_RADIUS_TILES = 7.5f;

    HudRenderer();

    // Reset explored fog-of-war on mission launch/restart
    void reset(int width = 32, int height = 32);

    // Update explored tiles around local player and alive teammates
    void updateExploration(float playerX, float playerY, const std::vector<NetPlayerState>& teammates, const Map& map);

    // Render permanent 150x150 minimap anchored at (x, y) in the HUD
    void renderMinimap(QPainter& painter, int x, int y, int size,
                       const Player& player, const Map& map,
                       const TaskManager& taskManager,
                       const std::vector<NetPlayerState>& teammates,
                       float timeSeconds);

private:
    void markExploredCircle(float cx, float cy, float radius, int width, int height);

    int m_width = 32;
    int m_height = 32;
    // Fog of war exploration mask: true if tile has been uncovered
    std::vector<std::vector<bool>> m_explored;
};

#endif // HUD_RENDERER_H
