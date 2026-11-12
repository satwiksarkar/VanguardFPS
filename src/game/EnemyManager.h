#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <QRandomGenerator>

#include "Enemy.h"
#include "net/NetProtocol.h"

class Map;
class Player;
struct WorldItem;

struct EnemyAttackEvent {
    EnemyType type;
    quint8 targetPlayerId;
    int damage;
    bool isElectricZap;
};

class EnemyManager {
public:
    EnemyManager();

    void populateLevel1();
    void reset();

    // Frame-rate independent update with multi-target squad awareness
    void update(float dt,
                Player& player,
                const std::vector<NetPlayerState>& remotePlayers,
                const Map& map,
                std::vector<WorldItem>& worldItems,
                std::vector<EnemyAttackEvent>& attackEvents);

    // Audio shockwave alert when player fires unsuppressed weapon
    void alertNearbyEnemies(float soundSourceX, float soundSourceY, float hearingRadius = 16.0f);
    void broadcastAlert(float alertX, float alertY, float alertRadius = 14.0f);

    // Hitscan bullet test from player weapon
    bool testHitscan(float px, float py, float dirX, float dirY, float maxRange, int baseDamage, bool isSniper, std::vector<WorldItem>& worldItems, std::string& outHitMsg);

    std::vector<Enemy>& getEnemies() { return m_enemies; }
    const std::vector<Enemy>& getEnemies() const { return m_enemies; }

    static bool hasLineOfSight(float x0, float y0, float x1, float y1, const Map& map);
    static bool findSteeringDirection(float ex, float ey, float targetX, float targetY, float radius, const Map& map, float& outDirX, float& outDirY);

private:
    void updateEnemyAI(Enemy& enemy,
                       float dt,
                       const Player& player,
                       const std::vector<NetPlayerState>& remotePlayers,
                       const Map& map,
                       std::vector<EnemyAttackEvent>& attackEvents);
    void spawnDeathDrop(const Enemy& enemy, std::vector<WorldItem>& worldItems);

    std::vector<Enemy> m_enemies;
};

#endif // ENEMY_MANAGER_H

