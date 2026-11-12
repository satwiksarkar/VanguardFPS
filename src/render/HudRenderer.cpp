#include "HudRenderer.h"

HudRenderer::HudRenderer() {
    reset(32, 32);
}

void HudRenderer::reset(int width, int height) {
    m_width = std::max(1, width);
    m_height = std::max(1, height);
    m_explored.assign(m_width, std::vector<bool>(m_height, false));

    // Pre-explore initial player spawn sector (around center)
    markExploredCircle(m_width * 0.5f, m_height * 0.5f, 4.5f, m_width, m_height);
}

void HudRenderer::markExploredCircle(float cx, float cy, float radius, int width, int height) {
    int minX = std::max(0, static_cast<int>(cx - radius));
    int maxX = std::min(width - 1, static_cast<int>(cx + radius));
    int minY = std::max(0, static_cast<int>(cy - radius));
    int maxY = std::min(height - 1, static_cast<int>(cy + radius));
    float rSq = radius * radius;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float dx = (x + 0.5f) - cx;
            float dy = (y + 0.5f) - cy;
            if (dx * dx + dy * dy <= rSq) {
                if (x < static_cast<int>(m_explored.size()) && y < static_cast<int>(m_explored[x].size())) {
                    m_explored[x][y] = true;
                }
            }
        }
    }
}

void HudRenderer::updateExploration(float playerX, float playerY, const std::vector<NetPlayerState>& teammates, const Map& map) {
    int w = map.getWidth();
    int h = map.getHeight();
    if (w != m_width || h != m_height) {
        reset(w, h);
    }

    // Reveal around local player (view radius 8.0 tiles)
    markExploredCircle(playerX, playerY, 8.0f, w, h);

    // Teammates in co-op also uncover fog of war for the squad
    for (const auto& tp : teammates) {
        if (tp.isAlive) {
            markExploredCircle(tp.posX, tp.posY, 6.0f, w, h);
        }
    }
}

void HudRenderer::renderMinimap(QPainter& painter, int x, int y, int size,
                                const Player& player, const Map& map,
                                const TaskManager& taskManager,
                                const std::vector<NetPlayerState>& teammates,
                                float timeSeconds)
{
    QRect box(x, y, size, size);

    // 1. Semi-transparent dark background
    painter.fillRect(box, QColor(0, 0, 0, 150));

    // Save state and set clipping rect to minimap boundary
    painter.save();
    painter.setClipRect(box);

    float cx = x + size * 0.5f;
    float cy = y + size * 0.5f;
    float tileSize = static_cast<float>(size) / (VIEW_RADIUS_TILES * 2.0f); // ~10.0 pixels per tile

    // Faint radar concentric range rings & crosshair
    painter.setPen(QPen(QColor(0, 240, 255, 30), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(cx, cy), tileSize * 3.5f, tileSize * 3.5f);
    painter.drawEllipse(QPointF(cx, cy), tileSize * 7.0f, tileSize * 7.0f);
    painter.drawLine(QPointF(x, cy), QPointF(x + size, cy));
    painter.drawLine(QPointF(cx, y), QPointF(cx, y + size));

    QFont markerFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    markerFont.setBold(true);
    markerFont.setPointSize(7);

    int mapW = map.getWidth();
    int mapH = map.getHeight();

    // Determine tile range visible in the 150x150 window
    int minTileX = std::max(0, static_cast<int>(player.getX() - VIEW_RADIUS_TILES - 1.0f));
    int maxTileX = std::min(mapW - 1, static_cast<int>(player.getX() + VIEW_RADIUS_TILES + 1.0f));
    int minTileY = std::max(0, static_cast<int>(player.getY() - VIEW_RADIUS_TILES - 1.0f));
    int maxTileY = std::min(mapH - 1, static_cast<int>(player.getY() + VIEW_RADIUS_TILES + 1.0f));

    // 2. Render Environment
    for (int ty = minTileY; ty <= maxTileY; ++ty) {
        for (int tx = minTileX; tx <= maxTileX; ++tx) {
            if (tx >= static_cast<int>(m_explored.size()) || ty >= static_cast<int>(m_explored[tx].size()) || !m_explored[tx][ty]) {
                continue; // Fog of war - unexplored
            }

            float sx = cx + (tx - player.getX()) * tileSize;
            float sy = cy + (ty - player.getY()) * tileSize;
            QRectF tileRect(sx, sy, tileSize + 0.6f, tileSize + 0.6f);

            int tile = map.getTile(tx, ty);

            if (map.isWall(tx, ty)) {
                if (tile >= 1 && tile <= 4) {
                    // Explored Walls: Dark Gray
                    painter.fillRect(tileRect, QColor(55, 62, 70));
                } else if (tile >= 20 && tile <= 23) {
                    // Electrical Box Terminals
                    int sector = TaskManager::tileToSector(tile);
                    bool fixed = taskManager.isElectricalFixed(sector);

                    if (fixed) {
                        // Fixed Electrical Rooms: Solid Green 'E'
                        painter.fillRect(tileRect, QColor(0, 220, 100));
                        painter.setPen(QColor(0, 35, 15));
                        painter.setFont(markerFont);
                        painter.drawText(tileRect, Qt::AlignCenter, "E");
                    } else {
                        // Unfixed Electrical Rooms: Blinking Amber 'E'
                        bool blinkOn = static_cast<int>(timeSeconds * 3.2f) % 2 == 0;
                        QColor amberCol = blinkOn ? QColor(255, 170, 0) : QColor(140, 90, 0);
                        painter.fillRect(tileRect, amberCol);
                        painter.setPen(blinkOn ? QColor(25, 12, 0) : QColor(255, 210, 80));
                        painter.setFont(markerFont);
                        painter.drawText(tileRect, Qt::AlignCenter, "E");
                    }
                } else if (tile >= 10 && tile <= 13) {
                    // Vault Doors: Cyan when open, Red when locked
                    int sector = TaskManager::tileToSector(tile);
                    bool open = taskManager.isVaultDoorOpen(sector);
                    QColor doorCol = open ? QColor(0, 240, 255) : QColor(255, 60, 40);
                    painter.fillRect(tileRect, doorCol);
                } else if (tile == 99) {
                    // Main Server Gateway: Purple or Red 'G' marker
                    bool allKeys = (taskManager.getFixedElectricalCount() >= 4 && taskManager.getOpenVaultsCount() >= 4);
                    QColor gateCol = allKeys ? QColor(180, 50, 255) : QColor(255, 30, 50);
                    painter.fillRect(tileRect, gateCol);
                    painter.setPen(QColor(255, 255, 255));
                    painter.setFont(markerFont);
                    painter.drawText(tileRect, Qt::AlignCenter, "G");
                } else if (tile >= 30 && tile <= 33) {
                    // Vault Control Panels
                    int sector = TaskManager::tileToSector(tile);
                    bool solved = taskManager.isRiddleSolved(sector);
                    QColor panelCol = solved ? QColor(0, 240, 255) : QColor(255, 200, 30);
                    painter.fillRect(tileRect, panelCol);
                } else {
                    // Generic obstacle / border wall
                    painter.fillRect(tileRect, QColor(48, 54, 62));
                }
            } else {
                // Explored walkable floor (subtle dark gray grid tint)
                painter.fillRect(tileRect, QColor(14, 20, 28, 90));
            }
        }
    }

    // 3. Render Teammates: Green dots
    for (const auto& tp : teammates) {
        float tmScreenX = cx + (tp.posX - player.getX()) * tileSize;
        float tmScreenY = cy + (tp.posY - player.getY()) * tileSize;

        if (tp.isDowned) {
            // Downed teammate: Pulsing red cross
            painter.setPen(QPen(QColor(255, 40, 40), 2));
            painter.drawLine(QPointF(tmScreenX - 3.5f, tmScreenY), QPointF(tmScreenX + 3.5f, tmScreenY));
            painter.drawLine(QPointF(tmScreenX, tmScreenY - 3.5f), QPointF(tmScreenX, tmScreenY + 3.5f));
        } else if (tp.isAlive) {
            // Teammate: Green dot
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 255, 120));
            painter.drawEllipse(QPointF(tmScreenX, tmScreenY), 2.8, 2.8);

            // Teammate look heading line
            painter.setPen(QPen(QColor(0, 255, 120), 1.5));
            painter.drawLine(QPointF(tmScreenX, tmScreenY),
                             QPointF(tmScreenX + tp.dirX * 5.0f, tmScreenY + tp.dirY * 5.0f));
        } else {
            // K.I.A. Teammate: small gray cross
            painter.setPen(QPen(QColor(130, 130, 130), 1.5));
            painter.drawLine(QPointF(tmScreenX - 2.5f, tmScreenY), QPointF(tmScreenX + 2.5f, tmScreenY));
            painter.drawLine(QPointF(tmScreenX, tmScreenY - 2.5f), QPointF(tmScreenX, tmScreenY + 2.5f));
        }
    }

    // 4. Render Local Player: Cyan dot/arrow (matching Taser drone cyan glow)
    if (player.isDead()) {
        painter.setPen(QPen(QColor(140, 140, 140), 2.5));
        painter.drawLine(QPointF(cx - 3.5f, cy), QPointF(cx + 3.5f, cy));
        painter.drawLine(QPointF(cx, cy - 3.5f), QPointF(cx, cy + 3.5f));
    } else if (player.isDowned()) {
        painter.setPen(QPen(QColor(255, 50, 50), 2.5));
        painter.drawLine(QPointF(cx - 4.0f, cy), QPointF(cx + 4.0f, cy));
        painter.drawLine(QPointF(cx, cy - 4.0f), QPointF(cx, cy + 4.0f));
    } else {
        // Cyan dot
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 240, 255));
        painter.drawEllipse(QPointF(cx, cy), 3.2, 3.2);

        // Directional Cyan Arrow / Pointer
        painter.setPen(QPen(QColor(0, 240, 255), 2.0));
        float arrowX = cx + player.getDirX() * 7.5f;
        float arrowY = cy + player.getDirY() * 7.5f;
        painter.drawLine(QPointF(cx, cy), QPointF(arrowX, arrowY));

        // Arrow tip
        float normalX = -player.getDirY();
        float normalY = player.getDirX();
        painter.drawLine(QPointF(arrowX, arrowY),
                         QPointF(arrowX - player.getDirX() * 2.5f + normalX * 2.0f,
                                 arrowY - player.getDirY() * 2.5f + normalY * 2.0f));
        painter.drawLine(QPointF(arrowX, arrowY),
                         QPointF(arrowX - player.getDirX() * 2.5f - normalX * 2.0f,
                                 arrowY - player.getDirY() * 2.5f - normalY * 2.0f));
    }

    // Restore painter clipping
    painter.restore();

    // 5. High-tech Outer Border & Corner Badges
    painter.setPen(QPen(QColor(0, 240, 255, 160), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(box);

    QFont labelFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    labelFont.setBold(true);
    labelFont.setPointSize(7);
    painter.setFont(labelFont);
    painter.setPen(QColor(0, 240, 255, 190));
    painter.drawText(QRect(x + 5, y + 3, 70, 14), Qt::AlignLeft, "MINIMAP");

    painter.setPen(QColor(160, 190, 210, 160));
    painter.drawText(QRect(x + size - 42, y + 3, 38, 14), Qt::AlignRight, "7.5m");
}
