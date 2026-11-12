#ifndef GAME_WIDGET_H
#define GAME_WIDGET_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <string>

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QFontDatabase>

#include "world/Map.h"
#include "world/WorldItem.h"
#include "world/TaskManager.h"
#include "game/Player.h"
#include "game/Inventory.h"
#include "game/EnemyManager.h"
#include "render/RasterRenderer.h"
#include "render/HudRenderer.h"
#include "net/NetworkManager.h"

struct HudNotification {
    QString text;
    QColor color;
    float timeRemaining;
};

class GameWidget : public QWidget {
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget() override = default;

    void startMission();
    void returnToLobby();

signals:
    void exitToLobbyRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void gameLoop();
    void onClientInputReceived(quint8 playerId, const ClientInputPacket& input);
    void onWorldStateReceived(const WorldStatePacket& state);

private:
    void clearInputStates();
    void prepareForModalOverlay();
    void restoreFromModalOverlay();
    void captureMouse(bool capture);

    void fireActiveWeapon();
    void reloadActiveWeapon();
    void useActiveConsumable();
    void dropActiveItem();
    void checkPickups();
    void handleInteraction();
    void restartGame();

    void addNotification(const QString& text, const QColor& color = QColor(0, 240, 255));

    void renderHUD(QPainter& painter);
    void renderHotbar(QPainter& painter, int bottomY);
    void renderHealthBar(QPainter& painter, int leftX, int bottomY);
    void renderAmmoStatus(QPainter& painter, int rightX, int bottomY);
    void renderNotifications(QPainter& painter);
    void renderDamageOverlays(QPainter& painter);
    void renderGameOver(QPainter& painter);
    void renderVictory(QPainter& painter);
    void renderLevelComplete(QPainter& painter);
    void renderSpectatorDeath(QPainter& painter);
    void renderCoopSquadRoster(QPainter& painter);
    void renderRemotePlayerOverlays(QPainter& painter);
    void renderTacticalRadar(QPainter& painter);
    void renderVanguardBroadcast(QPainter& painter);
    void renderRevivalProgressBar(QPainter& painter);

    void triggerVanguardBroadcast(const QString& text, float duration = 7.0f);
    void checkNarrativeMilestones();
    void handleTeammateRevival(float dt);

    Map m_map;
    Player m_player;
    Inventory m_inventory;
    TaskManager m_taskManager;
    EnemyManager m_enemyManager;
    std::vector<WorldItem> m_worldItems;
    RasterRenderer m_renderer;
    HudRenderer m_hudRenderer;

    // Co-op remote teammates
    std::vector<NetPlayerState> m_remotePlayers;

    // 20 Hz synchronization tick timer (0.05s)
    float m_netTickTimer = 0.0f;

    // Client action triggers pending transmission
    bool m_clientFiredShot = false;
    bool m_clientDroppedItem = false;
    quint8 m_clientRepairSector = 0xFF;
    quint8 m_clientSolveRiddleSector = 0xFF;
    bool m_clientOpenBulkhead = false;
    quint8 m_clientReviveTargetId = 0xFF;

    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
    float m_totalTimeSeconds = 0.0f;
    float m_countdownSeconds = 1800.0f; // 30:00 Vanguard uplink timer

    bool m_isGameOver = false;
    bool m_isVictory = false;
    bool m_isLevelComplete = false;
    int m_currentLevel = 1;
    float m_spawnX = 16.5f;
    float m_spawnY = 16.5f;

    bool loadLevel(int levelNumber);
    void proceedToNextLevel();

    // Weapon mechanics
    float m_fireCooldownTimer = 0.0f;
    float m_muzzleFlashTimer = 0.0f;
    float m_reloadTimer = 0.0f;
    bool m_isMouseDown = false;
    bool m_isRightMouseDown = false;

    // Damage & Screen FX feedback timers
    float m_taserShockTimer = 0.0f;
    float m_damageFlashTimer = 0.0f;
    float m_healPulseTimer = 0.0f;

    // Tactical Radar Overlay
    bool m_showRadar = false;

    // Vanguard AI Narrative Ticker
    struct VanguardBroadcast {
        QString text;
        float timer = 0.0f;
        float maxDuration = 7.0f;
    };
    VanguardBroadcast m_activeBroadcast;
    bool m_milestoneStart = false;
    bool m_milestoneFirstElec = false;
    bool m_milestoneFirstKey = false;
    bool m_milestone15Min = false;
    bool m_milestone5Min = false;
    bool m_milestoneAllKeys = false;

    // Teammate Revival
    float m_reviveHoldTimer = 0.0f;
    quint8 m_reviveTargetId = 0xFF;
    bool m_isHoldingReviveKey = false;

    bool m_mouseCaptured = false;
    float m_mouseSensitivity = 0.0022f;

    // Target detection
    std::string m_aimPrompt;
    std::vector<HudNotification> m_notifications;
};

#endif // GAME_WIDGET_H