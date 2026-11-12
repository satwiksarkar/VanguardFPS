#include "EnemyManager.h"
#include "game/Player.h"
#include "world/Map.h"
#include "world/WorldItem.h"
#include "audio/AudioManager.h"

EnemyManager::EnemyManager() {
    populateLevel1();
}

void EnemyManager::reset() {
    populateLevel1();
}

void EnemyManager::populateLevel1() {
    m_enemies.clear();

    // =========================================================================
    // 1. Central Hub & Server Gateway Assault Group (6 Enemies)
    // =========================================================================
    m_enemies.emplace_back(EnemyType::TASER_DRONE, 16.0f, 13.0f);     // Central corridor North scout
    m_enemies.emplace_back(EnemyType::TASER_DRONE, 16.0f, 11.5f);     // Central corridor assault drone
    m_enemies.emplace_back(EnemyType::GUN_DRONE, 14.5f, 8.5f);        // Flanking Server Gateway hallway
    m_enemies.emplace_back(EnemyType::GUN_DRONE, 17.5f, 8.5f);        // Flanking Server Gateway hallway
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 15.0f, 5.5f); // Heavy Bulkhead Sentry [Rotary Gun]
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 17.0f, 5.5f); // Heavy Bulkhead Sentry [Melee Brute]

    // =========================================================================
    // 2. Sector 1 (NW - Maintenance 1 & Vault 1 Challenge Group) (5 Enemies)
    // =========================================================================
    m_enemies.emplace_back(EnemyType::TASER_DRONE, 8.0f, 10.0f);      // Junction NW vanguard
    m_enemies.emplace_back(EnemyType::TASER_DRONE, 6.0f, 10.0f);      // Corridor flanker
    m_enemies.emplace_back(EnemyType::GUN_DRONE, 3.5f, 8.5f);         // Maintenance 1 room sentry
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 7.5f, 7.5f);  // Guarding Vault 1 Door
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 9.0f, 7.5f);  // Guarding Vault 1 Panel

    // =========================================================================
    // 3. Sector 2 (NE - Maintenance 2 & Vault 2 Challenge Group) (5 Enemies)
    // =========================================================================
    m_enemies.emplace_back(EnemyType::GUN_DRONE, 24.0f, 10.0f);       // Junction NE crossfire
    m_enemies.emplace_back(EnemyType::TASER_DRONE, 26.0f, 10.0f);     // Junction NE fast interceptor
    m_enemies.emplace_back(EnemyType::GUN_DRONE, 28.5f, 8.5f);        // Maintenance 2 sentry
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 24.5f, 7.5f); // Guarding Vault 2 Door
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 26.5f, 7.5f); // Guarding Vault 2 Panel

    // =========================================================================
    // 4. Sector 3 (SW - Maintenance 3 & Vault 3 Challenge Group) (4 Enemies)
    // =========================================================================
    m_enemies.emplace_back(EnemyType::TASER_DRONE, 8.0f, 22.0f);      // Junction SW vanguard
    m_enemies.emplace_back(EnemyType::GUN_DRONE, 3.5f, 23.5f);        // Maintenance 3 guardian
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 7.5f, 25.0f); // Guarding Vault 3 Door
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 9.0f, 25.0f); // Guarding Vault 3 Panel

    // =========================================================================
    // 5. Sector 4 (SE - Maintenance 4 & Vault 4 Challenge Group) (4 Enemies)
    // =========================================================================
    m_enemies.emplace_back(EnemyType::GUN_DRONE, 24.0f, 22.0f);       // Junction SE crossfire
    m_enemies.emplace_back(EnemyType::TASER_DRONE, 26.0f, 22.0f);     // Junction SE fast interceptor
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 24.5f, 25.0f);// Guarding Vault 4 Door
    m_enemies.emplace_back(EnemyType::TERMINATOR_ROBOT, 26.5f, 25.0f);// Guarding Vault 4 Panel
}

bool EnemyManager::hasLineOfSight(float x0, float y0, float x1, float y1, const Map& map) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 0.1f) return true;

    // Subdivide ray in 0.25-tile increments
    int steps = static_cast<int>(dist * 4.0f) + 1;
    float stepX = dx / static_cast<float>(steps);
    float stepY = dy / static_cast<float>(steps);

    float cx = x0;
    float cy = y0;
    for (int i = 0; i < steps; ++i) {
        cx += stepX;
        cy += stepY;
        if (map.isWall(static_cast<int>(cx), static_cast<int>(cy))) {
            return false;
        }
    }
    return true;
}

void EnemyManager::broadcastAlert(float alertX, float alertY, float alertRadius) {
    for (auto& other : m_enemies) {
        if (!other.isAlive()) continue;
        if (other.distanceTo(alertX, alertY) <= alertRadius) {
            other.setState(EnemyState::ALERT_CHASE);
            other.alertTimer() = std::max(other.alertTimer(), 12.0f);
            other.setLastKnownTarget(alertX, alertY);
        }
    }
}

bool EnemyManager::findSteeringDirection(float ex, float ey, float targetX, float targetY, float radius, const Map& map, float& outDirX, float& outDirY) {
    float toTargetX = targetX - ex;
    float toTargetY = targetY - ey;
    float dist = std::sqrt(toTargetX * toTargetX + toTargetY * toTargetY);
    if (dist < 0.05f) {
        outDirX = 0.0f;
        outDirY = 0.0f;
        return false;
    }

    float baseAngle = std::atan2(toTargetY, toTargetX);
    // Multi-directional probes: allows mob swarms to wrap around corners, doorway pillars, and wall junctions
    static const float testAngles[] = {
        0.0f,
        0.42f, -0.42f,  // ~24 deg
        0.84f, -0.84f,  // ~48 deg
        1.30f, -1.30f,  // ~75 deg
        1.75f, -1.75f,  // ~100 deg
        2.20f, -2.20f   // ~126 deg
    };

    float bestScore = 1e9f;
    float bestDirX = toTargetX / dist;
    float bestDirY = toTargetY / dist;
    bool found = false;

    for (float delta : testAngles) {
        float angle = baseAngle + delta;
        float probeDirX = std::cos(angle);
        float probeDirY = std::sin(angle);

        // Probe forward step distance
        float stepDist = 0.40f;
        float testX = ex + probeDirX * stepDist;
        float testY = ey + probeDirY * stepDist;

        if (map.isWalkable(testX, testY, radius)) {
            // Distance from probe position to target
            float pdx = targetX - testX;
            float pdy = targetY - testY;
            float pDist = std::sqrt(pdx * pdx + pdy * pdy);
            float score = pDist + std::abs(delta) * 0.40f;

            if (score < bestScore) {
                bestScore = score;
                bestDirX = probeDirX;
                bestDirY = probeDirY;
                found = true;
            }
        }
    }

    outDirX = bestDirX;
    outDirY = bestDirY;
    return found;
}

void EnemyManager::alertNearbyEnemies(float soundSourceX, float soundSourceY, float hearingRadius) {
    for (auto& enemy : m_enemies) {
        if (!enemy.isAlive()) continue;

        float dist = enemy.distanceTo(soundSourceX, soundSourceY);
        if (dist <= hearingRadius) {
            enemy.setState(EnemyState::ALERT_CHASE);
            enemy.alertTimer() = 12.0f;
            enemy.setLastKnownTarget(soundSourceX, soundSourceY);
            enemy.setDirection(soundSourceX - enemy.getX(), soundSourceY - enemy.getY());
        }
    }
}

void EnemyManager::update(float dt,
                          Player& player,
                          const std::vector<NetPlayerState>& remotePlayers,
                          const Map& map,
                          std::vector<WorldItem>& worldItems,
                          std::vector<EnemyAttackEvent>& attackEvents)
{
    (void)worldItems;
    // Frame-rate independent clamp
    float clampedDt = std::min(dt, 0.05f);

    for (auto& enemy : m_enemies) {
        // Timers update (decrements attackCooldown, hurtTimer, attackAnimTimer)
        enemy.updateTimers(clampedDt);

        if (!enemy.isAlive()) {
            enemy.deathTimer() += clampedDt;
            continue;
        }

        updateEnemyAI(enemy, clampedDt, player, remotePlayers, map, attackEvents);
    }
}

void EnemyManager::updateEnemyAI(Enemy& enemy,
                                float dt,
                                const Player& player,
                                const std::vector<NetPlayerState>& remotePlayers,
                                const Map& map,
                                std::vector<EnemyAttackEvent>& attackEvents)
{
    float ex = enemy.getX();
    float ey = enemy.getY();

    struct TargetCandidate {
        quint8 id;
        float x;
        float y;
        bool isDowned;
        float dist;
        bool hasLoS;
        float priorityScore;
    };

    std::vector<TargetCandidate> candidates;

    // Check Host player
    if (!player.isDead()) {
        float d = enemy.distanceTo(player.getX(), player.getY());
        bool los = hasLineOfSight(ex, ey, player.getX(), player.getY(), map);
        // Prioritize conscious standing players; downed/CPR players get +3.5 distance weighting
        float score = d + (player.isDowned() ? 3.5f : 0.0f);
        candidates.push_back({ 0, player.getX(), player.getY(), player.isDowned(), d, los, score });
    }

    // Check Remote players
    for (const auto& rp : remotePlayers) {
        if (rp.isAlive) {
            float d = enemy.distanceTo(rp.posX, rp.posY);
            bool los = hasLineOfSight(ex, ey, rp.posX, rp.posY, map);
            float score = d + (rp.isDowned ? 3.5f : 0.0f);
            candidates.push_back({ rp.id, rp.posX, rp.posY, rp.isDowned, d, los, score });
        }
    }

    if (candidates.empty()) {
        // No living players anywhere: wander or stay idle
        if (enemy.getState() != EnemyState::PATROL) {
            enemy.setState(EnemyState::PATROL);
        }
        return;
    }

    // Sort to find best target (lowest priorityScore)
    std::sort(candidates.begin(), candidates.end(), [](const TargetCandidate& a, const TargetCandidate& b) {
        return a.priorityScore < b.priorityScore;
    });

    const auto& bestTarget = candidates.front();
    float px = bestTarget.x;
    float py = bestTarget.y;
    float dist = bestTarget.dist;
    bool hasLoS = bestTarget.hasLoS;
    quint8 targetId = bestTarget.id;

    // Face the target
    enemy.setDirection(px - ex, py - ey);

    // AI state machine: Relentless Active Hunt AI & Damage Mechanics
    switch (enemy.getState()) {
        case EnemyState::PATROL: {
            // Aggressive detection: vision within 14.0 tiles or proximity within 12.0 tiles
            if ((dist <= 12.0f) || (dist <= 14.0f && hasLoS)) {
                enemy.setState(EnemyState::ALERT_CHASE);
                enemy.alertTimer() = 12.0f;
                enemy.setLastKnownTarget(px, py);
                broadcastAlert(ex, ey, 14.0f); // Coordinate nearby pack swarms

                if (enemy.getType() == EnemyType::TERMINATOR_ROBOT) {
                    AudioManager::instance().playSoundAt(SoundType::TERMINATOR_ALERT, ex, ey);
                }
            } else {
                // Relentless advancement in patrol
                enemy.patrolTimer() -= dt;
                if (enemy.patrolTimer() <= 0.0f) {
                    enemy.patrolTimer() = 2.0f + QRandomGenerator::global()->generateDouble() * 2.0f;
                    float angle = QRandomGenerator::global()->generateDouble() * 6.2831853f;
                    enemy.setDirection(std::cos(angle), std::sin(angle));
                }

                float patrolSpeed = enemy.getSpeed() * 0.70f;
                float stepX = enemy.getDirX() * patrolSpeed * dt;
                float stepY = enemy.getDirY() * patrolSpeed * dt;

                bool movedX = false;
                bool movedY = false;

                if (map.isWalkable(ex + stepX, ey, enemy.getRadius())) {
                    enemy.setPosition(ex + stepX, ey);
                    movedX = true;
                }
                if (map.isWalkable(enemy.getX(), ey + stepY, enemy.getRadius())) {
                    enemy.setPosition(enemy.getX(), ey + stepY);
                    movedY = true;
                }

                if (!movedX && !movedY) {
                    float angle = QRandomGenerator::global()->generateDouble() * 6.2831853f;
                    enemy.setDirection(std::cos(angle), std::sin(angle));
                    enemy.patrolTimer() = 1.0f;
                }
                break;
            }
            // Fallthrough to hunt and attack immediately if detected
            [[fallthrough]];
        }

        case EnemyState::ALERT_CHASE:
        case EnemyState::ATTACK: {
            if (hasLoS || dist <= 12.0f) {
                enemy.alertTimer() = 12.0f; // Active sight / hearing lock
                broadcastAlert(ex, ey, 12.0f);
            } else {
                enemy.alertTimer() -= dt;
                if (enemy.alertTimer() <= 0.0f) {
                    enemy.setState(EnemyState::PATROL);
                    enemy.patrolTimer() = 1.5f;
                    break;
                }
            }

            // 1. Strike if in range and cooldown is ready
            EnemyAttackResult atkResult = enemy.tryAttack(dist, hasLoS);
            if (atkResult.didAttack) {
                attackEvents.push_back({ enemy.getType(), targetId, atkResult.damage, atkResult.isElectricZap });
                broadcastAlert(ex, ey, 14.0f);

                if (atkResult.isElectricZap) {
                    AudioManager::instance().playSoundAt(SoundType::TASER_ZAP, ex, ey);
                } else if (atkResult.damage == 25) {
                    AudioManager::instance().playSoundAt(SoundType::TERMINATOR_MELEE, ex, ey);
                } else {
                    AudioManager::instance().playSoundAt(SoundType::WEAPON_FIRE_RIFLE, ex, ey);
                }
            }

            // 2. Active hunt advance logic (never freeze during combat)
            bool shouldAdvance = false;
            if (enemy.getType() == EnemyType::TASER_DRONE) {
                shouldAdvance = (dist >= 0.65f);
            } else if (enemy.getType() == EnemyType::GUN_DRONE) {
                shouldAdvance = (!hasLoS || dist >= 4.5f);
            } else if (enemy.getType() == EnemyType::TERMINATOR_ROBOT) {
                if (enemy.hasGun()) {
                    shouldAdvance = (!hasLoS || dist >= 4.0f);
                } else {
                    shouldAdvance = (dist >= 0.70f);
                }
            }

            // Short pause during strike animation impact
            if (enemy.isAttacking() && enemy.attackAnimTimer() > 0.15f) {
                shouldAdvance = false;
            }

            if (shouldAdvance) {
                // Predictive tracking: lead player movement vector
                float huntTargetX = px;
                float huntTargetY = py;
                if (bestTarget.id == 0 && (player.isMovingForward() || player.isMovingBackward() || player.isStrafingLeft() || player.isStrafingRight())) {
                    float pDirX = player.getDirX();
                    float pDirY = player.getDirY();
                    float lead = std::min(1.0f, dist / std::max(1.0f, enemy.getSpeed()));
                    float leadX = px + pDirX * player.getSpeed() * lead * 0.40f;
                    float leadY = py + pDirY * player.getSpeed() * lead * 0.40f;
                    if (map.isWalkable(leadX, leadY, enemy.getRadius())) {
                        huntTargetX = leadX;
                        huntTargetY = leadY;
                    }
                }
                enemy.setLastKnownTarget(huntTargetX, huntTargetY);

                // DDA multi-directional corner navigation probe
                float steerX = 0.0f;
                float steerY = 0.0f;
                bool steered = findSteeringDirection(ex, ey, huntTargetX, huntTargetY, enemy.getRadius(), map, steerX, steerY);

                float chaseSpeed = enemy.getSpeed();
                float moveDirX = steered ? steerX : (huntTargetX - ex);
                float moveDirY = steered ? steerY : (huntTargetY - ey);
                enemy.setDirection(moveDirX, moveDirY);

                float stepX = enemy.getDirX() * chaseSpeed * dt;
                float stepY = enemy.getDirY() * chaseSpeed * dt;

                if (map.isWalkable(ex + stepX, ey, enemy.getRadius())) {
                    enemy.setPosition(ex + stepX, ey);
                }
                if (map.isWalkable(enemy.getX(), ey + stepY, enemy.getRadius())) {
                    enemy.setPosition(enemy.getX(), ey + stepY);
                }
            }
            break;
        }

        case EnemyState::HURT: {
            if (enemy.hurtTimer() <= 0.0f) {
                enemy.setState(EnemyState::ALERT_CHASE);
                enemy.alertTimer() = 12.0f;
            }
            break;
        }

        case EnemyState::DEAD: {
            break;
        }
    }
}

bool EnemyManager::testHitscan(float px,
                               float py,
                               float dirX,
                               float dirY,
                               float maxRange,
                               int baseDamage,
                               bool isSniper,
                               std::vector<WorldItem>& worldItems,
                               std::string& outHitMsg)
{
    float closestDist = maxRange;
    Enemy* hitEnemy = nullptr;

    for (auto& enemy : m_enemies) {
        if (!enemy.isAlive()) continue;

        float toEnemyX = enemy.getX() - px;
        float toEnemyY = enemy.getY() - py;

        // Project onto ray direction
        float projDist = toEnemyX * dirX + toEnemyY * dirY;
        if (projDist > 0.3f && projDist < closestDist) {
            // Perpendicular distance from ray to enemy center
            float perpDist = std::abs(toEnemyX * dirY - toEnemyY * dirX);
            if (perpDist <= enemy.getRadius()) {
                closestDist = projDist;
                hitEnemy = &enemy;
            }
        }
    }

    if (hitEnemy) {
        // Apply damage with sniper headshot multiplier
        int damage = baseDamage;
        if (isSniper) {
            damage = static_cast<int>(baseDamage * 1.6f); // 176 damage critical headshot
        }

        hitEnemy->takeDamage(damage);

        if (!hitEnemy->isAlive()) {
            outHitMsg = "TARGET TERMINATED: " + hitEnemy->getDisplayName();
            // Roll drop chance
            spawnDeathDrop(*hitEnemy, worldItems);
        } else {
            outHitMsg = "HIT " + hitEnemy->getDisplayName() + " [-" + std::to_string(damage) + " HP]";
        }
        return true;
    }

    return false;
}

void EnemyManager::spawnDeathDrop(const Enemy& enemy, std::vector<WorldItem>& worldItems) {
    // 55% chance to drop useful ammo or health
    int roll = QRandomGenerator::global()->bounded(100);
    if (roll >= 55) return;

    float dropX = enemy.getX();
    float dropY = enemy.getY();

    switch (enemy.getType()) {
        case EnemyType::TASER_DRONE: {
            if (QRandomGenerator::global()->bounded(2) == 0) {
                worldItems.emplace_back(dropX, dropY, Item::createPistolAmmo(24));
            } else {
                worldItems.emplace_back(dropX, dropY, Item::createStimpack(1));
            }
            break;
        }
        case EnemyType::GUN_DRONE: {
            if (QRandomGenerator::global()->bounded(2) == 0) {
                worldItems.emplace_back(dropX, dropY, Item::createRifleAmmo(30));
            } else {
                worldItems.emplace_back(dropX, dropY, Item::createStimpack(1));
            }
            break;
        }
        case EnemyType::TERMINATOR_ROBOT: {
            int r = QRandomGenerator::global()->bounded(3);
            if (r == 0) {
                worldItems.emplace_back(dropX, dropY, Item::createRifleAmmo(60));
            } else if (r == 1) {
                worldItems.emplace_back(dropX, dropY, Item::createSniperAmmo(5));
            } else {
                worldItems.emplace_back(dropX, dropY, Item::createMedkit(1));
            }
            break;
        }
    }
}
