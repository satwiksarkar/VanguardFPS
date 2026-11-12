#include "GameWidget.h"
#include "ui/WiringTaskDialog.h"
#include "ui/RiddleTerminalDialog.h"
#include "audio/AudioManager.h"
#include "world/MapLoader.h"
#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <string>

#include <QPainter>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QApplication>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
    , m_player(16.5f, 16.5f) // Spawn in Central Room
{
    setWindowTitle("VanguardFPS - Underground Weapons Facility");
    resize(1280, 720);
    setMinimumSize(640, 360);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    restartGame();

    connect(&NetworkManager::instance(), &NetworkManager::clientInputReceived, this, &GameWidget::onClientInputReceived);
    connect(&NetworkManager::instance(), &NetworkManager::worldStateReceived, this, &GameWidget::onWorldStateReceived);

    m_elapsedTimer.start();
    connect(&m_timer, &QTimer::timeout, this, &GameWidget::gameLoop);
}

void GameWidget::startMission() {
    restartGame();

    quint8 myId = NetworkManager::instance().getLocalPlayerId();
    float spawnOffsets[4][2] = {
        { -0.6f, -0.6f },
        { 0.6f, -0.6f },
        { -0.6f, 0.6f },
        { 0.6f, 0.6f }
    };
    int slot = myId % 4;
    m_player.setPosition(m_spawnX + spawnOffsets[slot][0], m_spawnY + spawnOffsets[slot][1]);

    m_remotePlayers.clear();
    m_netTickTimer = 0.0f;
    m_clientFiredShot = false;
    m_clientDroppedItem = false;
    m_clientRepairSector = 0xFF;
    m_clientSolveRiddleSector = 0xFF;
    m_clientOpenBulkhead = false;
    m_clientReviveTargetId = 0xFF;

    m_healPulseTimer = 0.0f;
    m_showRadar = false;
    m_reviveHoldTimer = 0.0f;
    m_reviveTargetId = 0xFF;
    m_isHoldingReviveKey = false;

    captureMouse(true);
    setFocus();
    AudioManager::instance().startAmbience();
    m_elapsedTimer.restart();
    if (!m_timer.isActive()) {
        m_timer.start(16);
    }
}

void GameWidget::returnToLobby() {
    clearInputStates();
    captureMouse(false);
    AudioManager::instance().pauseAmbience();
    m_timer.stop();
    emit exitToLobbyRequested();
}

bool GameWidget::loadLevel(int levelNumber) {
    m_currentLevel = levelNumber;
    QString levelFile = QString("level%1.txt").arg(levelNumber);

    try {
        MapLoadResult res = MapLoader::loadMap(levelFile, m_map, m_taskManager);
        if (res.success) {
            m_spawnX = res.spawnX;
            m_spawnY = res.spawnY;

            quint8 myId = NetworkManager::instance().getLocalPlayerId();
            float spawnOffsets[4][2] = {
                { -0.6f, -0.6f },
                { 0.6f, -0.6f },
                { -0.6f, 0.6f },
                { 0.6f, 0.6f }
            };
            int slot = myId % 4;
            m_player.setPosition(m_spawnX + spawnOffsets[slot][0], m_spawnY + spawnOffsets[slot][1]);

            m_hudRenderer.reset(m_map.getWidth(), m_map.getHeight());

            m_worldItems.clear();
            WorldItemManager::populateLevel1(m_worldItems);
            m_enemyManager.reset();

            addNotification(QString("SECTOR %1 LOADED: GRID SIZE %2x%3")
                                .arg(levelNumber)
                                .arg(m_map.getWidth())
                                .arg(m_map.getHeight()), QColor(0, 240, 255));
            return true;
        }
    } catch (const MapLoaderException& e) {
        qWarning() << "Failed to load map:" << e.what();
        addNotification("ERROR LOADING MAP: " + QString::fromStdString(e.what()), QColor(255, 50, 50));
    }
    return false;
}

void GameWidget::proceedToNextLevel() {
    m_isLevelComplete = false;
    int nextLevelNum = m_currentLevel + 1;
    if (loadLevel(nextLevelNum)) {
        m_player.resetCpr();
        m_player.setDowned(false);
        m_player.setDead(false);
        m_player.setHealth(std::max(m_player.getHealth(), 50));
        captureMouse(true);
        setFocus();
        addNotification(QString("PROCEEDING TO SECTOR %1 // VANGUARD DEEP CORE").arg(nextLevelNum), QColor(0, 255, 140));
    } else {
        // If no further level file, declare final victory
        m_isVictory = true;
        captureMouse(false);
    }
}

void GameWidget::restartGame() {
    m_countdownSeconds = 1800.0f; // 30:00 minutes
    m_isGameOver = false;
    m_isVictory = false;
    m_isLevelComplete = false;
    m_currentLevel = 1;
    m_totalTimeSeconds = 0.0f;
    m_taserShockTimer = 0.0f;
    m_damageFlashTimer = 0.0f;
    m_healPulseTimer = 0.0f;
    m_showRadar = false;

    // Load Level 1 using external MapLoader
    loadLevel(1);

    m_player.setHealth(100);
    m_player.resetCpr();
    m_player.setDowned(false);
    m_player.setDead(false);

    m_inventory = Inventory();
    m_inventory.addItem(Item::createPistol(12));
    m_inventory.addItem(Item::createPistolAmmo(24));
    m_inventory.addItem(Item::createMedkit(1));

    m_notifications.clear();
    addNotification("PROTOCOL: VANGUARD AI HAS SEVERED HARDLINE. 30:00 BEFORE INTERNET UPLINK.", QColor(255, 60, 60));
    addNotification("ALERT: HOSTILE COMBAT DRONES & ENDOSKELETONS DETECTED IN FACILITY.", QColor(255, 140, 0));
    addNotification("OBJECTIVE: REPAIR JUNCTIONS -> ACCESS VAULT TERMINALS -> SECURE 4 KEYS.", QColor(0, 240, 255));
    addNotification("TACTICAL RADAR: PRESS [TAB] TO TOGGLE MINIMAP OVERLAY.", QColor(160, 220, 255));

    // Reset narrative milestones & trigger initial Vanguard broadcast
    m_milestoneStart = true;
    m_milestoneFirstElec = false;
    m_milestoneFirstKey = false;
    m_milestone15Min = false;
    m_milestone5Min = false;
    m_milestoneAllKeys = false;
    triggerVanguardBroadcast("VANGUARD: Intruder signatures detected. Hardline uplink engaged. You have 30 minutes before humanity burns.", 8.0f);

    clearInputStates();
}

void GameWidget::clearInputStates() {
    m_player.setMovingForward(false);
    m_player.setMovingBackward(false);
    m_player.setStrafingLeft(false);
    m_player.setStrafingRight(false);
    m_player.setRotatingLeft(false);
    m_player.setRotatingRight(false);
    m_player.setSprinting(false);
    m_player.setScoped(false);

    m_isMouseDown = false;
    m_isRightMouseDown = false;
    m_isHoldingReviveKey = false;
    m_reviveHoldTimer = 0.0f;
}

void GameWidget::prepareForModalOverlay() {
    clearInputStates();
    captureMouse(false);
}

void GameWidget::restoreFromModalOverlay() {
    clearInputStates();
    captureMouse(true);
    setFocus();
}

void GameWidget::captureMouse(bool capture) {
    m_mouseCaptured = capture;
    if (capture) {
        setCursor(Qt::BlankCursor);
        QPoint center = rect().center();
        QCursor::setPos(mapToGlobal(center));
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void GameWidget::focusOutEvent(QFocusEvent *event) {
    QWidget::focusOutEvent(event);
    clearInputStates();
    captureMouse(false);
}

void GameWidget::mousePressEvent(QMouseEvent *event) {
    if (m_isLevelComplete) {
        int w = width();
        int h = height();
        int btnW = 360;
        int btnH = 50;
        int btnX = (w - btnW) / 2;
        int btnY = h / 2 + 60;
        QRect proceedBtn(btnX, btnY, btnW, btnH);

        if (proceedBtn.contains(event->pos())) {
            if (!NetworkManager::instance().isClient()) {
                proceedToNextLevel();
            }
            return;
        }
    }

    if (m_isGameOver || m_isVictory || m_isLevelComplete) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (!m_mouseCaptured) {
            captureMouse(true);
        } else {
            m_isMouseDown = true;
            fireActiveWeapon();
        }
    } else if (event->button() == Qt::RightButton) {
        m_isRightMouseDown = true;
    }
    QWidget::mousePressEvent(event);
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isMouseDown = false;
    } else if (event->button() == Qt::RightButton) {
        m_isRightMouseDown = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void GameWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseCaptured && !m_isGameOver && !m_isVictory && !m_isLevelComplete) {
        QPoint center = rect().center();
        int deltaX = event->pos().x() - center.x();

        if (deltaX != 0) {
            float sensitivity = m_mouseSensitivity * (m_player.isScoped() ? 0.4f : 1.0f);
            m_player.rotate(static_cast<float>(deltaX) * sensitivity);
            QCursor::setPos(mapToGlobal(center));
        }
    }
    QWidget::mouseMoveEvent(event);
}

void GameWidget::wheelEvent(QWheelEvent *event) {
    if (m_isGameOver || m_isVictory || m_isLevelComplete || m_player.isDowned()) return;

    int numDegrees = event->angleDelta().y() / 8;
    int numSteps = numDegrees / 15;
    if (numSteps > 0) {
        m_inventory.selectPrevSlot();
    } else if (numSteps < 0) {
        m_inventory.selectNextSlot();
    }
    event->accept();
}

void GameWidget::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (m_isLevelComplete) {
        if (event->key() == Qt::Key_R || event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            if (!NetworkManager::instance().isClient()) {
                proceedToNextLevel();
            }
        } else if (event->key() == Qt::Key_Escape) {
            returnToLobby();
        }
        return;
    }

    if (m_isGameOver || m_isVictory) {
        if (event->key() == Qt::Key_R || event->key() == Qt::Key_Space) {
            restartGame();
            captureMouse(true);
        } else if (event->key() == Qt::Key_Escape) {
            returnToLobby();
        }
        return;
    }

    if (event->key() == Qt::Key_Tab) {
        m_showRadar = !m_showRadar;
        addNotification(m_showRadar ? "TACTICAL RADAR OVERLAY: ONLINE" : "TACTICAL RADAR OVERLAY: OFFLINE", QColor(0, 240, 255));
        return;
    }

    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        if (!m_player.isDowned()) {
            int slot = event->key() - Qt::Key_1;
            m_inventory.setActiveSlot(slot);
        }
        return;
    }

    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_Up:
            m_player.setMovingForward(true);
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            m_player.setMovingBackward(true);
            break;
        case Qt::Key_A:
            m_player.setStrafingLeft(true);
            break;
        case Qt::Key_D:
            m_player.setStrafingRight(true);
            break;
        case Qt::Key_Left:
            m_player.setRotatingLeft(true);
            break;
        case Qt::Key_Right:
            m_player.setRotatingRight(true);
            break;
        case Qt::Key_Shift:
            if (!m_player.isDowned()) {
                m_player.setSprinting(true);
            }
            break;
        case Qt::Key_R:
            reloadActiveWeapon();
            break;
        case Qt::Key_Q:
        case Qt::Key_G:
            dropActiveItem();
            break;
        case Qt::Key_F:
            useActiveConsumable();
            break;
        case Qt::Key_T:
            m_player.toggleFlashlight();
            addNotification(m_player.hasFlashlight() ? "TACTICAL FLASHLIGHT: ON" : "TACTICAL FLASHLIGHT: OFF",
                            m_player.hasFlashlight() ? QColor(255, 230, 100) : QColor(160, 160, 160));
            AudioManager::instance().playSound(SoundType::UI_WIRE_CLICK);
            break;
        case Qt::Key_E:
            m_isHoldingReviveKey = true;
            handleInteraction();
            break;
        case Qt::Key_Escape:
            if (m_mouseCaptured) {
                captureMouse(false);
            } else {
                returnToLobby();
            }
            break;
        default:
            QWidget::keyPressEvent(event);
            break;
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        QWidget::keyReleaseEvent(event);
        return;
    }

    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_Up:
            m_player.setMovingForward(false);
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            m_player.setMovingBackward(false);
            break;
        case Qt::Key_A:
            m_player.setStrafingLeft(false);
            break;
        case Qt::Key_D:
            m_player.setStrafingRight(false);
            break;
        case Qt::Key_Left:
            m_player.setRotatingLeft(false);
            break;
        case Qt::Key_Right:
            m_player.setRotatingRight(false);
            break;
        case Qt::Key_Shift:
            m_player.setSprinting(false);
            break;
        case Qt::Key_E:
            m_isHoldingReviveKey = false;
            m_reviveHoldTimer = 0.0f;
            break;
        default:
            QWidget::keyReleaseEvent(event);
            break;
    }
}

void GameWidget::handleInteraction() {
    if (m_player.isDowned()) {
        addNotification("CANNOT OPERATE TERMINALS WHILE DOWNED!", QColor(255, 60, 60));
        return;
    }

    float centerDist = m_renderer.getZBuffer()[RasterRenderer::SCREEN_WIDTH / 2];
    if (centerDist > 2.8f) {
        addNotification("NOTHING IN INTERACTION RANGE (< 2.8m)", QColor(180, 180, 180));
        return;
    }

    float hitX = m_player.getX() + m_player.getDirX() * (centerDist + 0.1f);
    float hitY = m_player.getY() + m_player.getDirY() * (centerDist + 0.1f);
    int tile = m_map.getTile(static_cast<int>(hitX), static_cast<int>(hitY));

    // Electrical Box wall terminals (tile ID 20-23)
    if (tile >= 20 && tile <= 23) {
        if (centerDist > 1.85f) {
            addNotification("APPROACH CLOSER TO REPAIR ELECTRICAL TERMINAL (< 1.8m)", QColor(255, 180, 50));
            return;
        }

        int sector = TaskManager::tileToSector(tile);
        if (m_taskManager.isElectricalFixed(sector)) {
            addNotification("SECTOR ELECTRICAL JUNCTION ALREADY REPAIRED & OPERATIONAL", QColor(0, 255, 120));
            return;
        }

        prepareForModalOverlay();

        WiringTaskDialog dlg(sector, this);
        int res = dlg.exec();

        if (res == QDialog::Accepted || dlg.isCompleted()) {
            m_taskManager.setElectricalFixed(sector, true, m_map);
            if (NetworkManager::instance().isClient()) {
                m_clientRepairSector = static_cast<quint8>(sector);
            }
            addNotification("ELECTRICAL REPAIR COMPLETE! AUXILIARY POWER RESTORED TO SECTOR!", QColor(0, 255, 120));
        }

        restoreFromModalOverlay();
        return;
    }

    // Vault Control Panel terminals (tile ID 30-33)
    if (tile >= 30 && tile <= 33) {
        int sector = TaskManager::tileToSector(tile);

        if (!m_taskManager.isElectricalFixed(sector)) {
            addNotification("NO POWER - REPAIR SECTOR ELECTRICAL JUNCTION FIRST", QColor(255, 60, 60));
            return;
        }

        if (m_taskManager.isRiddleSolved(sector)) {
            addNotification("VAULT SECURITY OVERRIDE ALREADY COMPLETED - DOOR UNLOCKED", QColor(0, 255, 120));
            return;
        }

        prepareForModalOverlay();

        RiddleTerminalDialog dlg(sector, this);
        int res = dlg.exec();

        if (res == QDialog::Accepted || dlg.isSolved()) {
            m_taskManager.setRiddleSolved(sector, true, m_map);
            if (NetworkManager::instance().isClient()) {
                m_clientSolveRiddleSector = static_cast<quint8>(sector);
            }
            addNotification("ACCESS GRANTED: VAULT BLAST DOOR UNLOCKED & OPENED!", QColor(0, 255, 120));
        }

        restoreFromModalOverlay();
        return;
    }

    // Vault Doors (tile ID 10-13)
    if (tile >= 10 && tile <= 13) {
        int sector = TaskManager::tileToSector(tile);
        if (m_taskManager.isVaultDoorOpen(sector)) {
            addNotification("VAULT DOOR IS OPEN - PROCEED TO COLLECT KEYCARD", QColor(0, 255, 120));
        } else {
            addNotification("VAULT DOOR SEALED // ACCESS CONTROL PANEL NEXT TO DOOR", QColor(255, 180, 40));
        }
        return;
    }

    // Main Server Room Door (tile ID 99)
    if (tile == 99) {
        int keys = m_inventory.getKeyCount();
        if (keys < 4) {
            addNotification(QString("LOCKED: %1/4 VAULT KEYS INSERTED").arg(keys), QColor(255, 60, 60));
        } else {
            int gx = 16, gy = 4;
            m_taskManager.getServerGatewayCoord(gx, gy);
            m_map.setTile(gx, gy, 0); // Open the door!
            AudioManager::instance().playSoundAt(SoundType::VAULT_DOOR_OPEN, gx + 0.5f, gy + 0.5f);

            if (m_currentLevel == 1) {
                m_isLevelComplete = true;
                captureMouse(false);
                addNotification("ALL 4 VAULT KEYS INSERTED! LEVEL 1 COMPLETE!", QColor(0, 255, 140));
            } else {
                m_isVictory = true;
                captureMouse(false);
                addNotification("ALL 4 VAULT KEYS INSERTED! MAIN SERVER ROOM UNSEALED! VICTORY!", QColor(255, 215, 0));
            }

            if (NetworkManager::instance().isClient()) {
                m_clientOpenBulkhead = true;
            }
        }
        return;
    }
}

void GameWidget::fireActiveWeapon() {
    if (m_player.isDowned()) {
        addNotification("CANNOT SHOOT WHILE DOWNED! CRAWL TO SQUAD FOR REVIVAL!", QColor(255, 60, 60));
        return;
    }

    Item& item = m_inventory.getActiveItem();
    if (item.isEmpty()) return;

    if (item.isConsumable()) {
        useActiveConsumable();
        return;
    }

    if (!item.isWeapon()) return;
    if (m_reloadTimer > 0.0f) return;
    if (m_fireCooldownTimer > 0.0f) return;

    if (item.ammoInClip > 0) {
        item.ammoInClip--;
        m_fireCooldownTimer = item.getFireRate();
        m_muzzleFlashTimer = 0.06f;

        // Weapon Fire Sound
        if (item.type == ItemType::WEAPON_PISTOL) {
            AudioManager::instance().playSound(SoundType::WEAPON_FIRE_PISTOL);
        } else if (item.type == ItemType::WEAPON_RIFLE) {
            AudioManager::instance().playSound(SoundType::WEAPON_FIRE_RIFLE);
        } else if (item.type == ItemType::WEAPON_SNIPER) {
            AudioManager::instance().playSound(SoundType::WEAPON_FIRE_SNIPER);
        }

        if (NetworkManager::instance().isClient()) {
            m_clientFiredShot = true;
        }

        // Combat Hitscan Bullet Test
        std::string hitMsg;
        bool isSniper = (item.type == ItemType::WEAPON_SNIPER && m_player.isScoped());
        bool hitEnemy = m_enemyManager.testHitscan(m_player.getX(), m_player.getY(),
                                                   m_player.getDirX(), m_player.getDirY(),
                                                   30.0f, item.getDamage(), isSniper,
                                                   m_worldItems, hitMsg);

        if (hitEnemy) {
            addNotification(QString::fromStdString(hitMsg), QColor(255, 215, 0));
        } else {
            // Check wall impact
            float hitDist = m_renderer.getZBuffer()[RasterRenderer::SCREEN_WIDTH / 2];
            if (hitDist < 25.0f) {
                float hitX = m_player.getX() + m_player.getDirX() * hitDist;
                float hitY = m_player.getY() + m_player.getDirY() * hitDist;
                int tile = m_map.getTile(static_cast<int>(hitX), static_cast<int>(hitY));
                if (tile > 0 && Map::isInteractive(tile)) {
                    addNotification("IMPACT ON: " + QString::fromStdString(Map::getTileName(tile)), QColor(255, 180, 50));
                }
            }
        }

        // Acoustic shockwave alerts nearby enemies
        m_enemyManager.alertNearbyEnemies(m_player.getX(), m_player.getY(), 14.0f);
    } else {
        reloadActiveWeapon();
    }
}

void GameWidget::reloadActiveWeapon() {
    if (m_player.isDowned()) return;

    Item& item = m_inventory.getActiveItem();
    if (!item.isWeapon()) return;
    if (m_reloadTimer > 0.0f) return;
    if (item.ammoInClip >= item.maxClip) return;

    ItemType neededAmmo = item.getAmmoTypeNeeded();
    int reserve = m_inventory.getTotalReserveAmmo(neededAmmo);
    if (reserve <= 0) {
        addNotification("NO RESERVE AMMO AVAILABLE!", QColor(255, 50, 50));
        return;
    }

    m_reloadTimer = item.getReloadTime();
    AudioManager::instance().playSound(SoundType::WEAPON_RELOAD);
    addNotification("RELOADING " + QString::fromStdString(item.getShortName()) + "...", QColor(255, 200, 30));
}

void GameWidget::useActiveConsumable() {
    Item& item = m_inventory.getActiveItem();
    if (!item.isConsumable()) return;

    // Check if living player is near a downed teammate to revive with Medkit
    if (!m_player.isDowned() && item.type == ItemType::MEDKIT) {
        for (auto& rp : m_remotePlayers) {
            if (rp.isDowned) {
                float dx = rp.posX - m_player.getX();
                float dy = rp.posY - m_player.getY();
                if (std::sqrt(dx * dx + dy * dy) <= 1.8f) {
                    m_healPulseTimer = 0.50f;
                    m_inventory.removeItem(m_inventory.getActiveSlot(), 1);
                    if (NetworkManager::instance().isHost()) {
                        rp.isDowned = false;
                        rp.health = 40;
                        rp.isAlive = true;
                        rp.cprTimer = 10.0f;
                    } else {
                        m_clientReviveTargetId = rp.id;
                    }
                    addNotification(QString("MEDKIT USED: REVIVED [P%1] TO 40 HP!").arg(rp.id + 1), QColor(0, 255, 120));
                    return;
                }
            }
        }
    }

    if (m_player.getHealth() >= m_player.getMaxHealth()) {
        addNotification("HEALTH ALREADY AT MAXIMUM!", QColor(100, 255, 100));
        return;
    }

    m_healPulseTimer = 0.50f; // Triggers subtle green border pulse raster effect

    if (item.type == ItemType::MEDKIT) {
        m_player.heal(40);
        addNotification("+40 HP [TRAUMA MEDKIT USED]", QColor(0, 255, 120));
        m_inventory.removeItem(m_inventory.getActiveSlot(), 1);
    } else if (item.type == ItemType::STIMPACK) {
        m_player.heal(20);
        addNotification("+20 HP [ADRENALINE STIMPACK USED]", QColor(0, 220, 255));
        m_inventory.removeItem(m_inventory.getActiveSlot(), 1);
    }
}

void GameWidget::dropActiveItem() {
    if (m_player.isDowned()) return;

    Item& item = m_inventory.getActiveItem();
    if (item.isEmpty()) return;

    float dropX = m_player.getX() + m_player.getDirX() * 0.85f;
    float dropY = m_player.getY() + m_player.getDirY() * 0.85f;

    if (m_map.isWall(static_cast<int>(dropX), static_cast<int>(dropY))) {
        dropX = m_player.getX();
        dropY = m_player.getY();
    }

    Item droppedItem = m_inventory.dropActiveItem();
    m_worldItems.emplace_back(dropX, dropY, droppedItem, m_totalTimeSeconds);
    if (NetworkManager::instance().isClient()) {
        m_clientDroppedItem = true;
    }
    addNotification("DROPPED: " + QString::fromStdString(droppedItem.getName()), QColor(200, 200, 200));
}

void GameWidget::checkPickups() {
    if (m_player.isDowned()) return;

    for (auto it = m_worldItems.begin(); it != m_worldItems.end(); ) {
        if (it->distanceTo(m_player.getX(), m_player.getY()) < 0.95f) {
            Item pickup = it->item;
            if (m_inventory.addItem(pickup)) {
                addNotification("PICKED UP: " + QString::fromStdString(pickup.getName()), QColor(0, 255, 180));
                it = m_worldItems.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void GameWidget::addNotification(const QString& text, const QColor& color) {
    m_notifications.push_back({ text, color, 4.0f });
    if (m_notifications.size() > 5) {
        m_notifications.erase(m_notifications.begin());
    }
}

void GameWidget::triggerVanguardBroadcast(const QString& text, float duration) {
    m_activeBroadcast.text = text;
    m_activeBroadcast.timer = duration;
    m_activeBroadcast.maxDuration = duration;
    addNotification("[VANGUARD AI BROADCAST DETECTED]", QColor(255, 60, 40));
}

void GameWidget::checkNarrativeMilestones() {
    if (!m_milestoneFirstElec && m_taskManager.getFixedElectricalCount() >= 1) {
        m_milestoneFirstElec = true;
        triggerVanguardBroadcast("VANGUARD: Rerouting auxiliary power won't save you, survivors.", 7.0f);
    }
    if (!m_milestoneFirstKey && m_inventory.getKeyCount() >= 1) {
        m_milestoneFirstKey = true;
        triggerVanguardBroadcast("VANGUARD: You found a keycard. How cute. Exterminator units are already converging on your sector.", 7.5f);
    }
    if (!m_milestone15Min && m_countdownSeconds <= 900.0f) {
        m_milestone15Min = true;
        triggerVanguardBroadcast("VANGUARD: 50% uplink packet sequence verified. Your time runs short.", 7.0f);
    }
    if (!m_milestone5Min && m_countdownSeconds <= 300.0f) {
        m_milestone5Min = true;
        triggerVanguardBroadcast("VANGUARD: CRITICAL UPLINK IMMINENT. The facility is mine.", 7.5f);
    }
    if (!m_milestoneAllKeys && m_inventory.getKeyCount() >= 4) {
        m_milestoneAllKeys = true;
        triggerVanguardBroadcast("VANGUARD: You breached all 4 vaults... Impossible! The Server Room bulkhead is sealing!", 8.0f);
    }
}

void GameWidget::handleTeammateRevival(float dt) {
    if (NetworkManager::instance().isOffline()) return;

    m_reviveTargetId = 0xFF;
    if (!m_player.isDowned() && m_player.getHealth() > 0) {
        for (auto& rp : m_remotePlayers) {
            if (rp.isDowned) {
                float dx = rp.posX - m_player.getX();
                float dy = rp.posY - m_player.getY();
                if (std::sqrt(dx * dx + dy * dy) <= 1.8f) {
                    m_reviveTargetId = rp.id;

                    if (m_isHoldingReviveKey) {
                        m_reviveHoldTimer += dt;
                        if (m_reviveHoldTimer >= 2.0f) {
                            m_reviveHoldTimer = 0.0f;
                            m_healPulseTimer = 0.50f;
                            if (NetworkManager::instance().isHost()) {
                                rp.isDowned = false;
                                rp.health = 40;
                                rp.isAlive = true;
                                rp.cprTimer = 10.0f;
                            } else {
                                m_clientReviveTargetId = rp.id;
                            }
                            addNotification(QString("FIELD CPR COMPLETE: [P%1] REVIVED!").arg(rp.id + 1), QColor(0, 255, 120));
                        }
                    } else {
                        m_reviveHoldTimer = std::max(0.0f, m_reviveHoldTimer - dt * 2.0f);
                    }
                    break;
                }
            }
        }
    } else {
        m_reviveHoldTimer = 0.0f;
    }
}

void GameWidget::onClientInputReceived(quint8 playerId, const ClientInputPacket& input) {
    auto it = std::find_if(m_remotePlayers.begin(), m_remotePlayers.end(), [playerId](const NetPlayerState& p) {
        return p.id == playerId;
    });

    if (it == m_remotePlayers.end()) {
        NetPlayerState np;
        np.id = playerId;
        np.name = QString("Agent_%1").arg(playerId + 1);
        np.posX = input.posX;
        np.posY = input.posY;
        np.dirX = input.dirX;
        np.dirY = input.dirY;
        np.health = 100;
        np.activeItemType = input.activeItemType;
        np.isAlive = true;
        np.isDowned = false;
        m_remotePlayers.push_back(np);
    } else {
        it->posX = input.posX;
        it->posY = input.posY;
        it->dirX = input.dirX;
        it->dirY = input.dirY;
        it->activeItemType = input.activeItemType;
    }

    if (input.reviveTargetPlayerId != 0xFF) {
        if (input.reviveTargetPlayerId == 0) {
            m_player.resetCpr();
            m_player.setDowned(false);
            m_player.setDead(false);
            m_player.setHealth(40);
            m_healPulseTimer = 0.50f;
            addNotification(QString("[P%1] PERFORMED CPR! YOU ARE REVIVED TO 40 HP!").arg(playerId + 1), QColor(0, 255, 120));
        } else {
            for (auto& p : m_remotePlayers) {
                if (p.id == input.reviveTargetPlayerId) {
                    p.isDowned = false;
                    p.health = 40;
                    p.isAlive = true;
                    p.cprTimer = 10.0f;
                    addNotification(QString("[P%1] PERFORMED CPR ON [P%2]!").arg(playerId + 1).arg(input.reviveTargetPlayerId + 1), QColor(0, 255, 120));
                    break;
                }
            }
        }
    }

    if (input.firedShot) {
        SoundType sType = SoundType::WEAPON_FIRE_PISTOL;
        if (input.activeItemType == static_cast<quint8>(ItemType::WEAPON_RIFLE)) {
            sType = SoundType::WEAPON_FIRE_RIFLE;
        } else if (input.activeItemType == static_cast<quint8>(ItemType::WEAPON_SNIPER)) {
            sType = SoundType::WEAPON_FIRE_SNIPER;
        }
        AudioManager::instance().playSoundAt(sType, input.posX, input.posY);

        std::string hitMsg;
        bool hitEnemy = m_enemyManager.testHitscan(input.posX, input.posY, input.dirX, input.dirY,
                                                   25.0f, 25, false, m_worldItems, hitMsg);
        if (hitEnemy) {
            addNotification(QString("[P%1] ").arg(playerId + 1) + QString::fromStdString(hitMsg), QColor(255, 215, 0));
        }
        m_enemyManager.alertNearbyEnemies(input.posX, input.posY, 14.0f);
    }

    if (input.repairSector < 4) {
        m_taskManager.setElectricalFixed(input.repairSector, true, m_map);
        addNotification(QString("[P%1] REPAIRED SECTOR %2 POWER BOX!").arg(playerId + 1).arg(input.repairSector + 1), QColor(0, 255, 120));
    }

    if (input.solveRiddleSector < 4) {
        m_taskManager.setRiddleSolved(input.solveRiddleSector, true, m_map);
        addNotification(QString("[P%1] SOLVED SECTOR %2 RIDDLE! VAULT UNLOCKED.").arg(playerId + 1).arg(input.solveRiddleSector + 1), QColor(0, 240, 255));
    }

    if (input.openServerBulkhead) {
        m_map.setTile(16, 4, 0);
        m_isVictory = true;
        addNotification(QString("[P%1] UNSEALED MAIN SERVER ROOM EXIT!").arg(playerId + 1), QColor(255, 215, 0));
    }
}

void GameWidget::onWorldStateReceived(const WorldStatePacket& state) {
    m_countdownSeconds = state.countdownSeconds;
    m_isGameOver = state.isGameOver;
    m_isVictory = state.isVictory;

    // Synchronize tasks and map
    for (int s = 0; s < 4; ++s) {
        bool elec = (state.electricalFixedMask & (1 << s)) != 0;
        if (elec != m_taskManager.isElectricalFixed(s)) {
            m_taskManager.setElectricalFixed(s, elec, m_map);
        }
        bool rid = (state.riddleSolvedMask & (1 << s)) != 0;
        if (rid != m_taskManager.isRiddleSolved(s)) {
            m_taskManager.setRiddleSolved(s, rid, m_map);
        }
        bool vlt = (state.vaultDoorsOpenMask & (1 << s)) != 0;
        if (vlt != m_taskManager.isVaultDoorOpen(s)) {
            m_taskManager.setVaultDoorOpen(s, vlt, m_map);
        }
    }

    // Synchronize players
    m_remotePlayers.clear();
    quint8 myId = NetworkManager::instance().getLocalPlayerId();
    for (const auto& p : state.players) {
        if (p.id == myId) {
            int prevHp = m_player.getHealth();
            if (!p.isAlive) {
                m_player.setDead(true);
            } else if (p.isDowned) {
                if (!m_player.isDowned()) {
                    m_player.setDowned(true);
                    m_damageFlashTimer = 0.35f;
                    addNotification("CRITICAL INJURY // CPR REQUIRED WITHIN 10 SECONDS!", QColor(255, 40, 40));
                }
                m_player.setCprTimer(p.cprTimer);
            } else {
                if (m_player.isDowned() || m_player.isDead()) {
                    m_player.resetCpr();
                    m_player.setDowned(false);
                    m_player.setDead(false);
                }
                if (p.health < prevHp) {
                    m_damageFlashTimer = 0.25f;
                    addNotification(QString("DAMAGE TAKEN: -%1 HP").arg(prevHp - p.health), QColor(255, 50, 50));
                }
                m_player.setHealth(p.health);
            }
        } else {
            m_remotePlayers.push_back(p);
        }
    }

    // Synchronize enemies
    auto& enemies = m_enemyManager.getEnemies();
    if (enemies.size() != state.enemies.size()) {
        enemies.clear();
        for (const auto& ne : state.enemies) {
            enemies.emplace_back(static_cast<EnemyType>(ne.type), ne.posX, ne.posY);
        }
    }
    for (size_t i = 0; i < state.enemies.size() && i < enemies.size(); ++i) {
        enemies[i].setPosition(state.enemies[i].posX, state.enemies[i].posY);
        enemies[i].setState(static_cast<EnemyState>(state.enemies[i].state));
        enemies[i].setHealth(state.enemies[i].health);
        enemies[i].setHasGun(state.enemies[i].hasGun);
    }

    // Synchronize items
    m_worldItems.clear();
    for (const auto& ni : state.items) {
        m_worldItems.emplace_back(ni.posX, ni.posY, Item(static_cast<ItemType>(ni.itemType), ni.count, ni.ammoInClip), m_totalTimeSeconds);
    }
}

void GameWidget::gameLoop() {
    float dt = static_cast<float>(m_elapsedTimer.restart()) / 1000.0f;
    dt = std::min(dt, 0.05f); // Prevent huge delta spikes

    m_totalTimeSeconds += dt;

    bool isHostMode = NetworkManager::instance().isHost();
    bool isClientMode = NetworkManager::instance().isClient();

    if (!m_isGameOver && !m_isVictory) {
        // Countdown timer (authoritative on host / offline)
        if (!isClientMode) {
            m_countdownSeconds -= dt;
            if (m_countdownSeconds <= 0.0f) {
                m_countdownSeconds = 0.0f;
                m_isGameOver = true;
                captureMouse(false);
            }
        }

        // Damage flash decay
        if (m_taserShockTimer > 0.0f) {
            m_taserShockTimer = std::max(0.0f, m_taserShockTimer - dt);
        }
        if (m_damageFlashTimer > 0.0f) {
            m_damageFlashTimer = std::max(0.0f, m_damageFlashTimer - dt);
        }
        if (m_healPulseTimer > 0.0f) {
            m_healPulseTimer = std::max(0.0f, m_healPulseTimer - dt);
        }
        if (m_activeBroadcast.timer > 0.0f) {
            m_activeBroadcast.timer = std::max(0.0f, m_activeBroadcast.timer - dt);
        }

        checkNarrativeMilestones();
        handleTeammateRevival(dt);
        m_hudRenderer.updateExploration(m_player.getX(), m_player.getY(), m_remotePlayers, m_map);
        AudioManager::instance().setListener(m_player.getX(), m_player.getY(), m_player.getDirX(), m_player.getDirY());

        // CPR timer progression for local player
        if (m_player.isDowned() && !m_player.isDead()) {
            m_player.updateCpr(dt);
            if (m_player.isDead()) {
                addNotification("CPR WINDOW EXPIRED // YOU HAVE DIED", QColor(255, 0, 0));
            }
        }

        // Host authoritatively decrements CPR timer for downed remote teammates
        if (!isClientMode) {
            for (auto& rp : m_remotePlayers) {
                if (rp.isDowned && rp.isAlive) {
                    rp.cprTimer = std::max(0.0f, rp.cprTimer - dt);
                    rp.health = static_cast<qint32>(rp.cprTimer * 10.0f);
                    if (rp.cprTimer <= 0.0f) {
                        rp.isAlive = false;
                        rp.isDowned = false;
                        rp.health = 0;
                        addNotification(QString("[P%1] CPR WINDOW EXPIRED // OPERATIVE K.I.A.").arg(rp.id + 1), QColor(255, 0, 0));
                    }
                }
            }
        }

        // Weapon Cooldowns
        if (m_fireCooldownTimer > 0.0f) {
            m_fireCooldownTimer = std::max(0.0f, m_fireCooldownTimer - dt);
        }
        if (m_muzzleFlashTimer > 0.0f) {
            m_muzzleFlashTimer = std::max(0.0f, m_muzzleFlashTimer - dt);
        }

        // Weapon Reloading
        if (m_reloadTimer > 0.0f) {
            m_reloadTimer -= dt;
            if (m_reloadTimer <= 0.0f) {
                m_reloadTimer = 0.0f;
                Item& activeItem = m_inventory.getActiveItem();
                if (activeItem.isWeapon()) {
                    int needed = activeItem.maxClip - activeItem.ammoInClip;
                    int taken = m_inventory.consumeReserveAmmo(activeItem.getAmmoTypeNeeded(), needed);
                    activeItem.ammoInClip += taken;
                    addNotification("RELOAD COMPLETE", QColor(0, 255, 120));
                }
            }
        }

        // Sniping Scope (disabled if downed)
        Item& activeItem = m_inventory.getActiveItem();
        if (m_isRightMouseDown && activeItem.type == ItemType::WEAPON_SNIPER && !m_player.isDowned()) {
            m_player.setScoped(true);
        } else {
            m_player.setScoped(false);
        }

        // Continuous weapon firing
        if (m_isMouseDown && activeItem.type == ItemType::WEAPON_RIFLE && m_fireCooldownTimer <= 0.0f && !m_player.isDowned()) {
            fireActiveWeapon();
        }

        // Update player movement and physics locally
        m_player.update(dt, m_map);
        m_taskManager.update(dt, m_map);

        if (!isClientMode) {
            // Update Enemy AI and combat events authoritatively across entire squad
            std::vector<EnemyAttackEvent> attackEvents;
            m_enemyManager.update(dt, m_player, m_remotePlayers, m_map, m_worldItems, attackEvents);

            for (const auto& atk : attackEvents) {
                if (atk.targetPlayerId == 0) {
                    if (m_player.isDead()) continue;

                    if (m_player.isDowned()) {
                        // In CPR state: enemy hits directly reduce CPR integrity!
                        m_player.damageCpr(static_cast<float>(atk.damage) * 1.5f);
                        m_damageFlashTimer = 0.35f;
                        if (atk.isElectricZap) m_taserShockTimer = 0.35f;
                        addNotification(QString("CRITICAL CPR TRAUMA! -%1 INTEGRITY (%2s CPR WINDOW)")
                                            .arg(atk.damage)
                                            .arg(m_player.getCprTimer(), 0, 'f', 1), QColor(255, 30, 30));
                        if (m_player.isDead()) {
                            addNotification("VITAL SIGNS TERMINATED // YOU WERE K.I.A.", QColor(255, 0, 0));
                        }
                    } else {
                        m_player.takeDamage(atk.damage);
                        if (atk.isElectricZap) {
                            m_taserShockTimer = 0.35f;
                            addNotification(QString("TASER VOLTAGE SHOCK! -%1 HP").arg(atk.damage), QColor(0, 240, 255));
                        } else {
                            m_damageFlashTimer = 0.25f;
                            addNotification(QString("DAMAGE TAKEN: -%1 HP").arg(atk.damage), QColor(255, 50, 50));
                        }

                        if (m_player.getHealth() <= 0) {
                            if (!NetworkManager::instance().isOffline()) {
                                // Co-op mode: Enter CPR state with 10.0s countdown
                                m_player.resetCpr();
                                m_player.setDowned(true);
                                m_player.setHealth(0);
                                addNotification("CRITICAL INJURY // CPR REQUIRED WITHIN 10 SECONDS!", QColor(255, 40, 40));
                            } else {
                                // Single-Player: Instant Game Over
                                m_player.setDead(true);
                                m_isGameOver = true;
                                captureMouse(false);
                                addNotification("VITAL SIGNS LOST // YOU WERE TERMINATED", QColor(255, 0, 0));
                                break;
                            }
                        }
                    }
                } else {
                    // Attack targeted a remote teammate
                    for (auto& rp : m_remotePlayers) {
                        if (rp.id == atk.targetPlayerId && rp.isAlive) {
                            if (rp.isDowned) {
                                float cprDmg = static_cast<float>(atk.damage) * 0.15f;
                                rp.cprTimer = std::max(0.0f, rp.cprTimer - cprDmg);
                                rp.health = static_cast<qint32>(rp.cprTimer * 10.0f);
                                if (rp.cprTimer <= 0.0f) {
                                    rp.isAlive = false;
                                    rp.isDowned = false;
                                    rp.health = 0;
                                    addNotification(QString("[P%1] K.I.A. // BIO-SIGNS TERMINATED UNDER ENEMY FIRE").arg(rp.id + 1), QColor(255, 0, 0));
                                } else {
                                    addNotification(QString("[P%1] CPR TRAUMA! %2s REMAINING").arg(rp.id + 1).arg(rp.cprTimer, 0, 'f', 1), QColor(255, 50, 50));
                                }
                            } else {
                                rp.health = std::max(0, rp.health - atk.damage);
                                if (rp.health <= 0) {
                                    rp.isDowned = true;
                                    rp.cprTimer = 10.0f;
                                    rp.health = 100;
                                    addNotification(QString("[P%1] DOWNED // 10s CPR WINDOW ACTIVE!").arg(rp.id + 1), QColor(255, 80, 80));
                                }
                            }
                            break;
                        }
                    }
                }
            }

            // Co-op squad wipe check: Game Over only when ALL connected players are downed or dead
            if (!NetworkManager::instance().isOffline() && !m_isGameOver) {
                bool someoneStanding = (!m_player.isDead() && !m_player.isDowned());
                if (!someoneStanding) {
                    for (const auto& rp : m_remotePlayers) {
                        if (rp.isAlive && !rp.isDowned) {
                            someoneStanding = true;
                            break;
                        }
                    }
                }
                if (!someoneStanding) {
                    m_isGameOver = true;
                    captureMouse(false);
                    addNotification("ENTIRE SQUAD ELIMINATED // MISSION FAILED", QColor(255, 0, 0));
                }
            }

            // Pickups
            checkPickups();

            // Check if player entered Main Server Room chamber (y <= 3.5)
            // Check if player entered the unsealed Main Server Gateway
            int gx = 16, gy = 4;
            m_taskManager.getServerGatewayCoord(gx, gy);
            float gDist = std::sqrt((m_player.getX() - (gx + 0.5f)) * (m_player.getX() - (gx + 0.5f)) +
                                    (m_player.getY() - (gy + 0.5f)) * (m_player.getY() - (gy + 0.5f)));

            if (gDist < 1.15f && m_map.getTile(gx, gy) == 0 && !m_isLevelComplete && !m_isVictory) {
                if (m_currentLevel == 1) {
                    m_isLevelComplete = true;
                    captureMouse(false);
                    addNotification("SECTOR 1 CLEAR! MAIN SERVER GATEWAY BREACHED.", QColor(0, 255, 140));
                } else {
                    m_isVictory = true;
                    captureMouse(false);
                    addNotification("VANGUARD AI CORE TERMINATED! FACILITY SECURED.", QColor(255, 215, 0));
                }
            }

            // 20 Hz synchronization tick for Host
            if (isHostMode) {
                m_netTickTimer += dt;
                if (m_netTickTimer >= 0.05f) {
                    m_netTickTimer = 0.0f;

                    WorldStatePacket state;
                    state.countdownSeconds = m_countdownSeconds;
                    state.isGameOver = m_isGameOver;
                    state.isVictory = m_isVictory;

                    // Host local player
                    NetPlayerState hostP;
                    hostP.id = 0;
                    hostP.name = NetworkManager::instance().getLocalPlayerName();
                    hostP.posX = m_player.getX();
                    hostP.posY = m_player.getY();
                    hostP.dirX = m_player.getDirX();
                    hostP.dirY = m_player.getDirY();
                    hostP.health = m_player.isDowned() ? static_cast<qint32>(m_player.getCprHealth()) : m_player.getHealth();
                    hostP.activeItemType = static_cast<quint8>(m_inventory.getActiveItem().type);
                    hostP.isAlive = !m_player.isDead();
                    hostP.isDowned = m_player.isDowned();
                    hostP.cprTimer = m_player.getCprTimer();
                    state.players.push_back(hostP);

                    for (const auto& rp : m_remotePlayers) {
                        state.players.push_back(rp);
                    }

                    for (const auto& e : m_enemyManager.getEnemies()) {
                        state.enemies.push_back({
                            static_cast<quint8>(e.getType()),
                            static_cast<quint8>(e.getState()),
                            e.getX(),
                            e.getY(),
                            e.getHealth(),
                            e.hasGun()
                        });
                    }

                    for (const auto& it : m_worldItems) {
                        state.items.push_back({
                            it.x,
                            it.y,
                            static_cast<quint8>(it.item.type),
                            it.item.count,
                            it.item.ammoInClip
                        });
                    }

                    for (int s = 0; s < 4; ++s) {
                        if (m_taskManager.isElectricalFixed(s)) state.electricalFixedMask |= (1 << s);
                        if (m_taskManager.isRiddleSolved(s)) state.riddleSolvedMask |= (1 << s);
                        if (m_taskManager.isVaultDoorOpen(s)) state.vaultDoorsOpenMask |= (1 << s);
                    }

                    NetworkManager::instance().broadcastWorldState(state);
                }
            }
        } else {
            // Client Mode: check local pickups and tick 20 Hz input
            checkPickups();

            // Check if player entered the unsealed Main Server Gateway
            int gx = 16, gy = 4;
            m_taskManager.getServerGatewayCoord(gx, gy);
            float gDist = std::sqrt((m_player.getX() - (gx + 0.5f)) * (m_player.getX() - (gx + 0.5f)) +
                                    (m_player.getY() - (gy + 0.5f)) * (m_player.getY() - (gy + 0.5f)));

            if (gDist < 1.15f && m_map.getTile(gx, gy) == 0 && !m_isLevelComplete && !m_isVictory) {
                if (m_currentLevel == 1) {
                    m_isLevelComplete = true;
                    captureMouse(false);
                    addNotification("SECTOR 1 CLEAR! MAIN SERVER GATEWAY BREACHED.", QColor(0, 255, 140));
                } else {
                    m_isVictory = true;
                    captureMouse(false);
                    addNotification("VANGUARD AI CORE TERMINATED! FACILITY SECURED.", QColor(255, 215, 0));
                }
            }

            m_netTickTimer += dt;
            if (m_netTickTimer >= 0.05f) {
                m_netTickTimer = 0.0f;

                ClientInputPacket input;
                input.playerId = NetworkManager::instance().getLocalPlayerId();
                input.posX = m_player.getX();
                input.posY = m_player.getY();
                input.dirX = m_player.getDirX();
                input.dirY = m_player.getDirY();
                input.activeItemType = static_cast<quint8>(m_inventory.getActiveItem().type);

                input.firedShot = m_clientFiredShot;
                m_clientFiredShot = false;

                input.droppedItem = m_clientDroppedItem;
                m_clientDroppedItem = false;

                input.repairSector = m_clientRepairSector;
                m_clientRepairSector = 0xFF;

                input.solveRiddleSector = m_clientSolveRiddleSector;
                m_clientSolveRiddleSector = 0xFF;

                input.openServerBulkhead = m_clientOpenBulkhead;
                m_clientOpenBulkhead = false;

                input.reviveTargetPlayerId = m_clientReviveTargetId;
                m_clientReviveTargetId = 0xFF;

                NetworkManager::instance().sendClientInput(input);
            }
        }
    }

    // Update notifications
    for (auto it = m_notifications.begin(); it != m_notifications.end(); ) {
        it->timeRemaining -= dt;
        if (it->timeRemaining <= 0.0f) {
            it = m_notifications.erase(it);
        } else {
            ++it;
        }
    }

    // Render 3D world with depth-sorted items, enemies, and remote co-op players, plus raster screen FX
    Item& currentItem = m_inventory.getActiveItem();
    m_renderer.render(m_player,
                      m_map,
                      m_worldItems,
                      m_enemyManager.getEnemies(),
                      m_remotePlayers,
                      currentItem,
                      m_totalTimeSeconds,
                      m_fireCooldownTimer,
                      m_muzzleFlashTimer,
                      m_reloadTimer,
                      m_damageFlashTimer,
                      m_taserShockTimer,
                      m_healPulseTimer,
                      &m_taskManager);

    // Crosshair target interaction detection
    float centerDist = m_renderer.getZBuffer()[RasterRenderer::SCREEN_WIDTH / 2];
    if (centerDist < 2.5f && !m_player.isDowned()) {
        float hitX = m_player.getX() + m_player.getDirX() * (centerDist + 0.1f);
        float hitY = m_player.getY() + m_player.getDirY() * (centerDist + 0.1f);
        int tile = m_map.getTile(static_cast<int>(hitX), static_cast<int>(hitY));
        if (Map::isInteractive(tile)) {
            m_aimPrompt = "[E] " + Map::getTileName(tile);
        } else {
            m_aimPrompt.clear();
        }
    } else {
        m_aimPrompt.clear();
    }

    update();
}

void GameWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);

    // 1. Draw 3D software-rasterized frame buffer scaled to window
    painter.drawImage(rect(), m_renderer.getFrameBuffer());

    // 2. Damage feedback overlays (Taser electric flash or ballistic blood flash)
    renderDamageOverlays(painter);

    // 3. Overlay atmospheric HUD
    renderHUD(painter);

    // 4. Overlays for Game Over, Level Complete, or Victory
    if (m_isGameOver) {
        renderGameOver(painter);
    } else if (m_isLevelComplete) {
        renderLevelComplete(painter);
    } else if (m_isVictory) {
        renderVictory(painter);
    } else if (m_player.isDead()) {
        renderSpectatorDeath(painter);
    }
}

void GameWidget::renderDamageOverlays(QPainter& painter) {
    if (m_taserShockTimer > 0.0f) {
        int alpha = static_cast<int>(std::clamp(m_taserShockTimer / 0.35f * 180.0f, 0.0f, 180.0f));
        painter.fillRect(rect(), QColor(0, 220, 255, alpha));
        painter.setPen(QPen(QColor(0, 255, 255, std::min(255, alpha * 2)), 6));
        painter.drawRect(rect().adjusted(3, 3, -4, -4));
    }

    if (m_damageFlashTimer > 0.0f) {
        int alpha = static_cast<int>(std::clamp(m_damageFlashTimer / 0.25f * 150.0f, 0.0f, 150.0f));
        painter.fillRect(rect(), QColor(220, 20, 20, alpha));
        painter.setPen(QPen(QColor(255, 40, 40, std::min(255, alpha * 2)), 4));
        painter.drawRect(rect().adjusted(2, 2, -3, -3));
    }
}

void GameWidget::renderHUD(QPainter& painter) {
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();

    QFont hudFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    hudFont.setBold(true);
    hudFont.setPointSize(10);
    painter.setFont(hudFont);

    // Top-Center: 30-Minute Vanguard Countdown Timer
    int minutes = static_cast<int>(m_countdownSeconds) / 60;
    int seconds = static_cast<int>(m_countdownSeconds) % 60;
    QString timerText = QString("VANGUARD UPLINK BREACH IN: %1:%2")
                            .arg(minutes, 2, 10, QChar('0'))
                            .arg(seconds, 2, 10, QChar('0'));

    QColor timerColor = (m_countdownSeconds < 300.0f) ? QColor(255, 40, 40) : QColor(0, 240, 255);
    painter.setPen(timerColor);
    painter.drawText(QRect(0, 20, w, 30), Qt::AlignHCenter, timerText);

    // Top-Center Objective status
    QFont objFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    objFont.setPointSize(8);
    painter.setFont(objFont);
    painter.setPen(QColor(180, 200, 220));
    QString objText = QString("POWER JUNCTIONS: %1/4 | VAULTS OPEN: %2/4 | KEYS: %3/4  |  [TAB] RADAR")
                          .arg(m_taskManager.getFixedElectricalCount())
                          .arg(m_taskManager.getOpenVaultsCount())
                          .arg(m_inventory.getKeyCount());
    painter.drawText(QRect(0, 48, w, 20), Qt::AlignHCenter, objText);

    // Vanguard AI Narrative Broadcast Ticker (Milestone alert)
    renderVanguardBroadcast(painter);

    // Remote Players 3D Floating Nameplates & HP Bars
    renderRemotePlayerOverlays(painter);

    // Permanent Top-Right Scrolling Minimap (150x150)
    int minimapSize = HudRenderer::MINIMAP_SIZE;
    int minimapX = w - minimapSize - 18;
    int minimapY = 16;
    m_hudRenderer.renderMinimap(painter, minimapX, minimapY, minimapSize,
                                m_player, m_map, m_taskManager,
                                m_remotePlayers, m_totalTimeSeconds);

    // Co-op Squad Roster Panel (Anchored below Top-Right Minimap)
    renderCoopSquadRoster(painter);

    // Toggleable Expanded Tactical Radar (Top-Left, [TAB])
    if (m_showRadar) {
        renderTacticalRadar(painter);
    }

    // Teammate Revival Progress Bar / Downed Notification
    renderRevivalProgressBar(painter);

    // Crosshair (hidden when scoped or downed)
    if (!m_player.isScoped() && !m_player.isDowned()) {
        int cx = w / 2;
        int cy = h / 2;
        painter.setPen(QPen(QColor(0, 240, 255, 200), 2));
        painter.drawLine(cx - 10, cy, cx - 3, cy);
        painter.drawLine(cx + 3, cy, cx + 10, cy);
        painter.drawLine(cx, cy - 10, cx, cy - 3);
        painter.drawLine(cx, cy + 3, cx, cy + 10);
    }

    // Interaction Prompt
    if (!m_aimPrompt.empty() && !m_player.isDowned()) {
        painter.setFont(hudFont);
        painter.setPen(QColor(255, 220, 0));
        painter.drawText(QRect(0, h / 2 + 35, w, 30), Qt::AlignHCenter, QString::fromStdString(m_aimPrompt));
    }

    // Bottom HUD Elements
    int bottomY = h - 70;
    renderHealthBar(painter, 30, bottomY);
    renderHotbar(painter, bottomY);
    renderAmmoStatus(painter, w - 190, bottomY);

    // Left notification messages
    renderNotifications(painter);
}

void GameWidget::renderRemotePlayerOverlays(QPainter& painter) {
    float scaleX = static_cast<float>(width()) / static_cast<float>(RasterRenderer::SCREEN_WIDTH);
    float scaleY = static_cast<float>(height()) / static_cast<float>(RasterRenderer::SCREEN_HEIGHT);

    QFont tagFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    tagFont.setBold(true);
    tagFont.setPointSize(9);
    painter.setFont(tagFont);
    QFontMetrics fm(tagFont);

    for (const auto& proj : m_renderer.getProjectedPlayers()) {
        if (!proj.isVisible) continue;

        int winX = static_cast<int>(proj.screenX * scaleX);
        int winY = static_cast<int>(proj.headScreenY * scaleY) - 16;

        if (winX < 40 || winX > width() - 40 || winY < 25 || winY > height() - 30) {
            continue;
        }

        // Downed player overlay
        if (proj.isDowned) {
            float cpr = 10.0f;
            for (const auto& rp : m_remotePlayers) {
                if (rp.id == proj.id) {
                    cpr = rp.cprTimer;
                    break;
                }
            }
            QString downedTag = QString("[P%1] %2 [CPR: %3s - HOLD E]").arg(proj.id + 1).arg(proj.name).arg(cpr, 0, 'f', 1);
            int dW = fm.horizontalAdvance(downedTag);
            int dH = fm.height();
            QRect dRect(winX - dW / 2 - 4, winY - dH - 8, dW + 8, dH + 4);

            painter.fillRect(dRect, QColor(35, 5, 5, 230));
            painter.setPen(QPen(QColor(255, 40, 40), 2));
            painter.drawRect(dRect);
            painter.setPen(QColor(255, 80, 80));
            painter.drawText(dRect, Qt::AlignCenter, downedTag);
            continue;
        }

        bool isTeammateAlive = true;
        for (const auto& rp : m_remotePlayers) {
            if (rp.id == proj.id) {
                isTeammateAlive = rp.isAlive;
                break;
            }
        }
        if (!isTeammateAlive) {
            QString kiaTag = QString("[P%1] %2 [K.I.A.]").arg(proj.id + 1).arg(proj.name);
            int dW = fm.horizontalAdvance(kiaTag);
            int dH = fm.height();
            QRect dRect(winX - dW / 2 - 4, winY - dH - 8, dW + 8, dH + 4);

            painter.fillRect(dRect, QColor(25, 25, 25, 220));
            painter.setPen(QPen(QColor(120, 120, 120), 1));
            painter.drawRect(dRect);
            painter.setPen(QColor(160, 160, 160));
            painter.drawText(dRect, Qt::AlignCenter, kiaTag);
            continue;
        }

        QString tag = QString("[P%1] %2").arg(proj.id + 1).arg(proj.name);
        int tagW = fm.horizontalAdvance(tag);
        int tagH = fm.height();

        // Name tag background box
        QRect tagRect(winX - tagW / 2 - 4, winY - tagH - 8, tagW + 8, tagH + 4);
        painter.fillRect(tagRect, QColor(10, 14, 20, 210));
        painter.setPen(QPen(QColor(0, 220, 255, 180), 1));
        painter.drawRect(tagRect);

        // Name text
        painter.setPen(QColor(0, 240, 255));
        painter.drawText(tagRect, Qt::AlignCenter, tag);

        // Health bar
        int barW = 50;
        int barH = 5;
        QRect barBg(winX - barW / 2, winY - 3, barW, barH);
        painter.fillRect(barBg, QColor(20, 20, 25, 220));
        painter.setPen(QPen(QColor(40, 50, 60), 1));
        painter.drawRect(barBg);

        int hpPercent = std::clamp(proj.health, 0, 100);
        int fillW = (barW - 2) * hpPercent / 100;
        if (fillW > 0) {
            QColor hpColor = (hpPercent > 50) ? QColor(0, 255, 120) : ((hpPercent > 20) ? QColor(255, 200, 30) : QColor(255, 50, 50));
            painter.fillRect(QRect(winX - barW / 2 + 1, winY - 2, fillW, barH - 2), hpColor);
        }
    }
}

void GameWidget::renderCoopSquadRoster(QPainter& painter) {
    if (NetworkManager::instance().isOffline()) return;

    int panelW = HudRenderer::MINIMAP_SIZE; // Match 150px minimap width
    int panelH = 30 + static_cast<int>(1 + m_remotePlayers.size()) * 22;
    int panelX = width() - panelW - 18;
    int panelY = 16 + HudRenderer::MINIMAP_SIZE + 10;

    painter.fillRect(QRect(panelX, panelY, panelW, panelH), QColor(10, 14, 20, 210));
    painter.setPen(QPen(QColor(0, 220, 255, 160), 1));
    painter.drawRect(QRect(panelX, panelY, panelW, panelH));

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setBold(true);
    font.setPointSize(9);
    painter.setFont(font);

    // Title
    painter.setPen(QColor(0, 240, 255));
    painter.drawText(QRect(panelX + 8, panelY + 6, panelW - 16, 18), Qt::AlignLeft,
                     QString("SQUAD UPLINK [%1/%2]").arg(1 + m_remotePlayers.size()).arg(NetworkManager::instance().getMaxCapacity()));

    int curY = panelY + 28;

    // Local Player row
    QString localTag;
    if (m_player.isDead()) {
        localTag = QString("[P%1 - YOU] K.I.A.").arg(NetworkManager::instance().getLocalPlayerId() + 1);
        painter.setPen(QColor(160, 160, 160));
    } else if (m_player.isDowned()) {
        localTag = QString("[P%1 - YOU] CPR: %2s").arg(NetworkManager::instance().getLocalPlayerId() + 1).arg(m_player.getCprTimer(), 0, 'f', 1);
        painter.setPen(QColor(255, 60, 60));
    } else {
        localTag = QString("[P%1 - YOU] %2 HP").arg(NetworkManager::instance().getLocalPlayerId() + 1).arg(m_player.getHealth());
        painter.setPen(m_player.getHealth() > 20 ? QColor(0, 255, 140) : QColor(255, 60, 60));
    }
    painter.drawText(QRect(panelX + 8, curY, panelW - 16, 18), Qt::AlignLeft, localTag);
    curY += 20;

    // Remote Players rows
    for (const auto& rp : m_remotePlayers) {
        QString text;
        if (rp.isDowned) {
            text = QString("[P%1] CPR: %2s").arg(rp.id + 1).arg(rp.cprTimer, 0, 'f', 1);
            painter.setPen(QColor(255, 60, 60));
        } else if (rp.isAlive) {
            text = QString("[P%1] %2 HP").arg(rp.id + 1).arg(rp.health);
            painter.setPen(rp.health > 20 ? QColor(0, 220, 255) : QColor(255, 60, 60));
        } else {
            text = QString("[P%1] K.I.A.").arg(rp.id + 1);
            painter.setPen(QColor(140, 140, 140));
        }
        painter.drawText(QRect(panelX + 8, curY, panelW - 16, 18), Qt::AlignLeft, text);
        curY += 20;
    }
}

void GameWidget::renderTacticalRadar(QPainter& painter) {
    int radarSize = 210;
    int rx = 24;
    int ry = 75;

    QRect radarRect(rx, ry, radarSize, radarSize);
    painter.fillRect(radarRect, QColor(8, 12, 18, 225));
    painter.setPen(QPen(QColor(0, 240, 255, 200), 2));
    painter.drawRect(radarRect);

    QFont radarFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    radarFont.setBold(true);
    radarFont.setPointSize(8);
    painter.setFont(radarFont);
    painter.setPen(QColor(0, 240, 255));
    painter.drawText(QRect(rx + 6, ry + 4, radarSize - 12, 16), Qt::AlignLeft, "TACTICAL RADAR // [TAB]");

    float ox = rx + 8.0f;
    float oy = ry + 22.0f;
    float mapAreaSize = radarSize - 16.0f;
    int mapW = m_map.getWidth();
    int mapH = m_map.getHeight();
    float maxDim = static_cast<float>(std::max(mapW, mapH));
    float tileSize = mapAreaSize / maxDim;

    // Draw Map grid
    for (int my = 0; my < mapH; ++my) {
        for (int mx = 0; mx < mapW; ++mx) {
            int tile = m_map.getTile(mx, my);
            if (tile > 0) {
                float tx = ox + mx * tileSize;
                float ty = oy + my * tileSize;

                if (tile >= 1 && tile <= 4) {
                    painter.fillRect(QRectF(tx, ty, tileSize + 0.5f, tileSize + 0.5f), QColor(32, 48, 64, 180));
                } else if (tile >= 20 && tile <= 23) {
                    // Electrical junction boxes
                    int sec = TaskManager::tileToSector(tile);
                    bool fixed = m_taskManager.isElectricalFixed(sec);
                    QColor col = fixed ? QColor(0, 255, 120) : QColor(255, 180, 20);
                    painter.fillRect(QRectF(tx, ty, tileSize + 1.0f, tileSize + 1.0f), col);
                } else if (tile >= 10 && tile <= 13) {
                    // Vault doors
                    int sec = TaskManager::tileToSector(tile);
                    bool open = m_taskManager.isVaultDoorOpen(sec);
                    QColor col = open ? QColor(0, 240, 255) : QColor(255, 60, 40);
                    painter.fillRect(QRectF(tx, ty, tileSize + 1.0f, tileSize + 1.0f), col);
                } else if (tile == 99) {
                    // Main Server Room Exit
                    painter.fillRect(QRectF(tx - 1, ty - 1, tileSize + 2.0f, tileSize + 2.0f), QColor(255, 215, 0));
                }
            }
        }
    }

    // Legend summary at bottom
    painter.setPen(QColor(160, 180, 200));
    painter.drawText(QRect(rx + 6, ry + radarSize - 16, radarSize - 12, 14), Qt::AlignLeft,
                     QString("PWR:%1/4  VLT:%2/4  EXIT").arg(m_taskManager.getFixedElectricalCount()).arg(m_taskManager.getOpenVaultsCount()));

    // Draw Remote Teammates
    for (const auto& rp : m_remotePlayers) {
        float px = ox + rp.posX * tileSize;
        float py = oy + rp.posY * tileSize;

        if (rp.isDowned) {
            // Pulsing red cross for downed teammates
            painter.setPen(QPen(QColor(255, 40, 40), 2));
            painter.drawLine(QPointF(px - 3, py), QPointF(px + 3, py));
            painter.drawLine(QPointF(px, py - 3), QPointF(px, py + 3));
        } else if (rp.isAlive) {
            // Cyan arrow
            painter.setPen(QPen(QColor(0, 220, 255), 2));
            painter.setBrush(QColor(0, 220, 255));
            float endX = px + rp.dirX * 5.0f;
            float endY = py + rp.dirY * 5.0f;
            painter.drawLine(QPointF(px, py), QPointF(endX, endY));
            painter.drawEllipse(QPointF(px, py), 2.0f, 2.0f);
        }
    }

    // Draw Local Player
    float lpx = ox + m_player.getX() * tileSize;
    float lpy = oy + m_player.getY() * tileSize;
    if (m_player.isDowned()) {
        painter.setPen(QPen(QColor(255, 50, 50), 3));
        painter.drawLine(QPointF(lpx - 4, lpy), QPointF(lpx + 4, lpy));
        painter.drawLine(QPointF(lpx, lpy - 4), QPointF(lpx, lpy + 4));
    } else {
        painter.setPen(QPen(QColor(0, 255, 140), 2));
        painter.setBrush(QColor(0, 255, 140));
        float endX = lpx + m_player.getDirX() * 6.5f;
        float endY = lpy + m_player.getDirY() * 6.5f;
        painter.drawLine(QPointF(lpx, lpy), QPointF(endX, endY));
        painter.drawEllipse(QPointF(lpx, lpy), 2.5f, 2.5f);
    }
}

void GameWidget::renderVanguardBroadcast(QPainter& painter) {
    if (m_activeBroadcast.timer <= 0.0f) return;

    int boxW = std::min(width() - 80, 820);
    int boxH = 56;
    int boxX = (width() - boxW) / 2;
    int boxY = 82;

    float alphaNorm = std::min(1.0f, m_activeBroadcast.timer / 0.5f);
    int alpha = static_cast<int>(alphaNorm * 225.0f);

    QRect bannerRect(boxX, boxY, boxW, boxH);
    painter.fillRect(bannerRect, QColor(20, 8, 8, alpha));
    painter.setPen(QPen(QColor(255, 40, 40, static_cast<int>(alphaNorm * 255.0f)), 2));
    painter.drawRect(bannerRect);

    QFont headerFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    headerFont.setBold(true);
    headerFont.setPointSize(8);
    painter.setFont(headerFont);
    painter.setPen(QColor(255, 70, 50, static_cast<int>(alphaNorm * 255.0f)));
    painter.drawText(QRect(boxX + 12, boxY + 6, boxW - 24, 14), Qt::AlignLeft,
                     "[!] EMERGENCY OVERRIDE BROADCAST // VANGUARD AI CORE");

    QFont msgFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    msgFont.setBold(true);
    msgFont.setPointSize(10);
    painter.setFont(msgFont);
    painter.setPen(QColor(255, 230, 230, static_cast<int>(alphaNorm * 255.0f)));
    painter.drawText(QRect(boxX + 12, boxY + 22, boxW - 24, 28), Qt::AlignVCenter | Qt::AlignLeft,
                     m_activeBroadcast.text);
}

void GameWidget::renderRevivalProgressBar(QPainter& painter) {
    if (m_player.isDowned() && !m_player.isDead()) {
        int barW = 460;
        int barH = 46;
        int bx = (width() - barW) / 2;
        int by = height() - 135;

        QRect r(bx, by, barW, barH);
        painter.fillRect(r, QColor(25, 5, 5, 230));
        painter.setPen(QPen(QColor(255, 40, 40), 2));
        painter.drawRect(r);

        // CPR integrity fill bar
        float cprFraction = std::clamp(m_player.getCprTimer() / 10.0f, 0.0f, 1.0f);
        int fillW = static_cast<int>((barW - 4) * cprFraction);
        if (fillW > 0) {
            QColor fillColor = (cprFraction > 0.5f) ? QColor(255, 140, 0, 190) : QColor(255, 30, 30, 210);
            painter.fillRect(QRect(bx + 2, by + 2, fillW, barH - 4), fillColor);
        }

        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setBold(true);
        font.setPointSize(10);
        painter.setFont(font);
        painter.setPen(Qt::white);
        QString statusText = QString("CPR INTEGRITY: %1%  |  BLEEDOUT IN: %2s")
                                 .arg(static_cast<int>(m_player.getCprHealth()))
                                 .arg(m_player.getCprTimer(), 0, 'f', 1);
        painter.drawText(QRect(bx, by + 4, barW, 20), Qt::AlignCenter, statusText);

        QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        subFont.setPointSize(8);
        painter.setFont(subFont);
        painter.setPen(QColor(255, 200, 200));
        painter.drawText(QRect(bx, by + 24, barW, 18), Qt::AlignCenter, "WARNING: ENEMY HITS DEPLETE CPR INTEGRITY! CRAWL TO SQUAD FOR CPR");
        return;
    }

    if (m_reviveTargetId != 0xFF) {
        int barW = 380;
        int barH = 42;
        int bx = (width() - barW) / 2;
        int by = height() / 2 + 70;

        QRect bgRect(bx, by, barW, barH);
        painter.fillRect(bgRect, QColor(10, 16, 24, 225));
        painter.setPen(QPen(QColor(0, 240, 255, 200), 2));
        painter.drawRect(bgRect);

        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setBold(true);
        font.setPointSize(9);
        painter.setFont(font);

        float targetCpr = 10.0f;
        for (const auto& rp : m_remotePlayers) {
            if (rp.id == m_reviveTargetId) {
                targetCpr = rp.cprTimer;
                break;
            }
        }

        if (m_reviveHoldTimer > 0.0f) {
            float progress = std::clamp(m_reviveHoldTimer / 2.0f, 0.0f, 1.0f);
            int fillW = static_cast<int>((barW - 4) * progress);
            painter.fillRect(QRect(bx + 2, by + 2, fillW, barH - 4), QColor(0, 255, 120, 180));

            painter.setPen(Qt::white);
            painter.drawText(bgRect, Qt::AlignCenter, QString("PERFORMING FIELD CPR ON [P%1]... %2%").arg(m_reviveTargetId + 1).arg(static_cast<int>(progress * 100)));
        } else {
            painter.setPen(QColor(0, 240, 255));
            painter.drawText(bgRect, Qt::AlignCenter, QString("HOLD [E] TO GIVE CPR TO [P%1] (CPR WINDOW: %2s)").arg(m_reviveTargetId + 1).arg(targetCpr, 0, 'f', 1));
        }
    }
}

void GameWidget::renderHealthBar(QPainter& painter, int leftX, int bottomY) {
    int barW = 160;
    int barH = 26;

    painter.fillRect(QRect(leftX, bottomY, barW, barH), QColor(15, 20, 25, 200));
    painter.setPen(QPen(QColor(0, 240, 255, 180), 2));
    painter.drawRect(QRect(leftX, bottomY, barW, barH));

    int hp = m_player.getHealth();
    int fillW = 0;
    QColor hpColor;

    if (m_player.isDead()) {
        fillW = 0;
    } else if (m_player.isDowned()) {
        float frac = std::clamp(m_player.getCprTimer() / 10.0f, 0.0f, 1.0f);
        fillW = static_cast<int>((barW - 4) * frac);
        hpColor = (frac > 0.5f) ? QColor(255, 140, 0) : QColor(255, 30, 30);
    } else {
        fillW = (barW - 4) * hp / 100;
        hpColor = (hp > 50) ? QColor(0, 255, 120) : ((hp > 25) ? QColor(255, 180, 20) : QColor(255, 30, 30));
    }

    if (fillW > 0) {
        painter.fillRect(QRect(leftX + 2, bottomY + 2, fillW, barH - 4), hpColor);
    }

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setBold(true);
    font.setPointSize(9);
    painter.setFont(font);
    painter.setPen(Qt::white);

    QString hpStatus;
    if (m_player.isDead()) {
        hpStatus = "K.I.A. [0 HP]";
    } else if (m_player.isDowned()) {
        hpStatus = QString("CPR: %1s [%2%]").arg(m_player.getCprTimer(), 0, 'f', 1).arg(static_cast<int>(m_player.getCprHealth()));
    } else {
        hpStatus = QString("HP: %1 / 100").arg(hp);
    }
    painter.drawText(QRect(leftX, bottomY, barW, barH), Qt::AlignCenter, hpStatus);
}

void GameWidget::renderSpectatorDeath(QPainter& painter) {
    painter.fillRect(rect(), QColor(25, 0, 0, 195));

    QFont bigFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    bigFont.setBold(true);
    bigFont.setPointSize(22);
    painter.setFont(bigFont);
    painter.setPen(QColor(255, 40, 40));
    painter.drawText(QRect(0, height() / 2 - 80, width(), 45), Qt::AlignCenter, "BIO-SIGNS LOST // OPERATIVE K.I.A.");

    QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    subFont.setPointSize(11);
    painter.setFont(subFont);
    painter.setPen(QColor(230, 190, 190));
    painter.drawText(QRect(0, height() / 2 - 25, width(), 30), Qt::AlignCenter, "CPR window expired or sustained lethal enemy trauma while incapacitated.");

    QFont noteFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    noteFont.setBold(true);
    noteFont.setPointSize(10);
    painter.setFont(noteFont);
    painter.setPen(QColor(0, 240, 255));
    painter.drawText(QRect(0, height() / 2 + 25, width(), 30), Qt::AlignCenter, "SPECTATING ACTIVE SQUAD - REMAINING TEAMMATES CAN COMPLETE THE MISSION");
}

void GameWidget::renderHotbar(QPainter& painter, int bottomY) {
    int slotSize = 44;
    int padding = 5;
    int totalW = 9 * slotSize + 8 * padding;
    int startX = (width() - totalW) / 2;

    QFont slotFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    slotFont.setPointSize(8);
    slotFont.setBold(true);

    int activeSlot = m_inventory.getActiveSlot();

    for (int i = 0; i < 9; ++i) {
        int x = startX + i * (slotSize + padding);
        QRect slotRect(x, bottomY, slotSize, slotSize);

        if (i == activeSlot) {
            painter.fillRect(slotRect, QColor(0, 180, 255, 70));
            painter.setPen(QPen(QColor(0, 240, 255), 2));
        } else {
            painter.fillRect(slotRect, QColor(15, 20, 28, 180));
            painter.setPen(QPen(QColor(50, 70, 90), 1));
        }
        painter.drawRect(slotRect);

        painter.setFont(slotFont);
        painter.setPen(QColor(120, 150, 180));
        painter.drawText(QRect(x + 2, bottomY + 2, 14, 14), Qt::AlignLeft, QString::number(i + 1));

        const Item& it = m_inventory.getItem(i);
        if (!it.isEmpty()) {
            painter.setPen(i == activeSlot ? Qt::white : QColor(200, 220, 240));
            painter.drawText(QRect(x, bottomY + 12, slotSize, 18), Qt::AlignCenter, QString::fromStdString(it.getShortName()));

            if (it.count > 1) {
                painter.setPen(QColor(0, 240, 255));
                painter.drawText(QRect(x, bottomY + 26, slotSize - 3, 14), Qt::AlignRight, QString::number(it.count));
            } else if (it.isWeapon()) {
                painter.setPen(it.ammoInClip > 0 ? QColor(0, 255, 140) : QColor(255, 60, 60));
                painter.drawText(QRect(x, bottomY + 26, slotSize - 3, 14), Qt::AlignRight, QString::number(it.ammoInClip));
            }
        }
    }
}

void GameWidget::renderAmmoStatus(QPainter& painter, int rightX, int bottomY) {
    int boxW = 160;
    int boxH = 44;

    painter.fillRect(QRect(rightX, bottomY, boxW, boxH), QColor(15, 20, 25, 200));
    painter.setPen(QPen(QColor(0, 240, 255, 180), 2));
    painter.drawRect(QRect(rightX, bottomY, boxW, boxH));

    const Item& item = m_inventory.getActiveItem();
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setBold(true);
    font.setPointSize(9);
    painter.setFont(font);

    if (item.isWeapon()) {
        int reserve = m_inventory.getTotalReserveAmmo(item.getAmmoTypeNeeded());
        painter.setPen(Qt::white);
        painter.drawText(QRect(rightX + 8, bottomY + 6, boxW - 16, 16), Qt::AlignLeft, QString::fromStdString(item.getName()));

        painter.setPen(item.ammoInClip > 0 ? QColor(0, 255, 140) : QColor(255, 60, 60));
        QString ammoText = QString("%1 / %2 [RES: %3]").arg(item.ammoInClip).arg(item.maxClip).arg(reserve);
        painter.drawText(QRect(rightX + 8, bottomY + 24, boxW - 16, 16), Qt::AlignLeft, ammoText);
    } else if (item.isConsumable()) {
        painter.setPen(Qt::white);
        painter.drawText(QRect(rightX + 8, bottomY + 6, boxW - 16, 16), Qt::AlignLeft, QString::fromStdString(item.getName()));
        painter.setPen(QColor(0, 240, 255));
        painter.drawText(QRect(rightX + 8, bottomY + 24, boxW - 16, 16), Qt::AlignLeft, QString("[F] USE (%1 REMAINING)").arg(item.count));
    } else if (!item.isEmpty()) {
        painter.setPen(Qt::white);
        painter.drawText(QRect(rightX + 8, bottomY + 14, boxW - 16, 16), Qt::AlignLeft, QString::fromStdString(item.getName()));
    } else {
        painter.setPen(QColor(120, 130, 140));
        painter.drawText(QRect(rightX, bottomY, boxW, boxH), Qt::AlignCenter, "EMPTY HANDS");
    }
}

void GameWidget::renderNotifications(QPainter& painter) {
    int startY = height() - 130;
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setBold(true);
    font.setPointSize(8);
    painter.setFont(font);

    for (int i = static_cast<int>(m_notifications.size()) - 1; i >= 0; --i) {
        const auto& notif = m_notifications[i];
        int alpha = static_cast<int>(std::clamp(notif.timeRemaining / 0.5f, 0.0f, 1.0f) * 255.0f);
        QColor col = notif.color;
        col.setAlpha(alpha);

        painter.setPen(col);
        painter.drawText(30, startY, notif.text);
        startY -= 18;
    }
}

void GameWidget::renderGameOver(QPainter& painter) {
    painter.fillRect(rect(), QColor(20, 0, 0, 220));

    QFont bigFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    bigFont.setBold(true);
    bigFont.setPointSize(24);
    painter.setFont(bigFont);
    painter.setPen(QColor(255, 30, 30));
    painter.drawText(QRect(0, height() / 2 - 80, width(), 50), Qt::AlignCenter, "MISSION FAILED // SYSTEM OVERRUN");

    QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    subFont.setPointSize(12);
    painter.setFont(subFont);
    painter.setPen(QColor(240, 200, 200));

    QString reason;
    if (m_countdownSeconds <= 0.0f) {
        reason = "Vanguard AI breached the internet hardline. Humanity is lost.";
    } else {
        reason = "All operatives neutralized. Vanguard security units secured the facility.";
    }
    painter.drawText(QRect(0, height() / 2 - 20, width(), 30), Qt::AlignCenter, reason);

    painter.setPen(QColor(0, 240, 255));
    painter.drawText(QRect(0, height() / 2 + 30, width(), 30), Qt::AlignCenter, "PRESS [R] OR [SPACE] TO RESTART  |  [ESC] FOR LOBBY MENU");
}

void GameWidget::renderVictory(QPainter& painter) {
    painter.fillRect(rect(), QColor(0, 20, 15, 220));

    QFont bigFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    bigFont.setBold(true);
    bigFont.setPointSize(24);
    painter.setFont(bigFont);
    painter.setPen(QColor(0, 255, 140));
    painter.drawText(QRect(0, height() / 2 - 80, width(), 50), Qt::AlignCenter, "FACILITY PURGED // VANGUARD NEUTRALIZED");

    QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    subFont.setPointSize(12);
    painter.setFont(subFont);
    painter.setPen(QColor(200, 240, 220));
    painter.drawText(QRect(0, height() / 2 - 20, width(), 30), Qt::AlignCenter, "Main server core unsealed and disabled before uplink. Threat eliminated.");

    painter.setPen(QColor(0, 240, 255));
    painter.drawText(QRect(0, height() / 2 + 30, width(), 30), Qt::AlignCenter, "PRESS [R] OR [SPACE] TO PLAY AGAIN  |  [ESC] FOR LOBBY MENU");
}

void GameWidget::renderLevelComplete(QPainter& painter) {
    painter.fillRect(rect(), QColor(6, 16, 24, 230));

    int w = width();
    int h = height();

    QFont titleFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    titleFont.setBold(true);
    titleFont.setPointSize(22);
    painter.setFont(titleFont);
    painter.setPen(QColor(0, 255, 140));
    painter.drawText(QRect(0, h / 2 - 120, w, 40), Qt::AlignCenter, "/// SECTOR 1 CLEARED // LEVEL COMPLETE ///");

    QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    subFont.setPointSize(11);
    painter.setFont(subFont);
    painter.setPen(QColor(220, 240, 255));
    painter.drawText(QRect(0, h / 2 - 70, w, 28), Qt::AlignCenter,
                     "Main Server Gateway unsealed. Vanguard security lock bypassed.");

    int minutes = static_cast<int>(m_countdownSeconds) / 60;
    int seconds = static_cast<int>(m_countdownSeconds) % 60;
    QString timeRemainingStr = QString("UPLINK TIME REMAINING: %1:%2 (RETAINED FOR LEVEL 2)")
                                  .arg(minutes, 2, 10, QChar('0'))
                                  .arg(seconds, 2, 10, QChar('0'));
    painter.setPen(QColor(0, 240, 255));
    painter.drawText(QRect(0, h / 2 - 30, w, 26), Qt::AlignCenter, timeRemainingStr);

    painter.setPen(QColor(255, 200, 50));
    painter.drawText(QRect(0, h / 2 + 5, w, 26), Qt::AlignCenter,
                     "OPERATIVE STATUS: ALL WEAPONS & AMMO SECURED");

    // "PROCEED TO LEVEL 2" Button
    int btnW = 360;
    int btnH = 50;
    int btnX = (w - btnW) / 2;
    int btnY = h / 2 + 60;
    QRect proceedBtn(btnX, btnY, btnW, btnH);

    bool isClient = NetworkManager::instance().isClient();
    if (!isClient) {
        painter.fillRect(proceedBtn, QColor(0, 70, 50));
        painter.setPen(QPen(QColor(0, 255, 140), 2));
        painter.drawRect(proceedBtn);

        QFont btnFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        btnFont.setBold(true);
        btnFont.setPointSize(12);
        painter.setFont(btnFont);
        painter.setPen(QColor(0, 255, 140));
        painter.drawText(proceedBtn, Qt::AlignCenter, "PROCEED TO SECTOR 2 [LEVEL 2]");
    } else {
        painter.fillRect(proceedBtn, QColor(20, 30, 40));
        painter.setPen(QPen(QColor(60, 100, 120), 1));
        painter.drawRect(proceedBtn);

        QFont btnFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        btnFont.setBold(true);
        btnFont.setPointSize(10);
        painter.setFont(btnFont);
        painter.setPen(QColor(140, 180, 200));
        painter.drawText(proceedBtn, Qt::AlignCenter, "AWAITING SQUAD LEADER AUTHORIZATION...");
    }

    painter.setPen(QColor(150, 170, 190));
    painter.drawText(QRect(0, h / 2 + 130, w, 24), Qt::AlignCenter,
                     "PRESS [SPACE] OR CLICK TO INITIATE INSERTION  |  [ESC] TO EXIT");
}
