#ifndef ENEMY_H
#define ENEMY_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <QRandomGenerator>

enum class EnemyType : uint8_t {
    TASER_DRONE = 0,
    GUN_DRONE = 1,
    TERMINATOR_ROBOT = 2
};

enum class EnemyState : uint8_t {
    PATROL = 0,
    ALERT_CHASE = 1,
    HUNT = 1, // Active relentless hunt
    ATTACK = 2,
    HURT = 3,
    DEAD = 4
};

struct EnemyAttackResult {
    bool didAttack = false;
    int damage = 0;
    bool isElectricZap = false;
};

class Enemy {
public:
    Enemy(EnemyType type, float startX, float startY);

    EnemyType getType() const { return m_type; }
    EnemyState getState() const { return m_state; }
    void setState(EnemyState state) { m_state = state; }

    float getX() const { return m_x; }
    float getY() const { return m_y; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    float getDirX() const { return m_dirX; }
    float getDirY() const { return m_dirY; }
    void setDirection(float dx, float dy);

    int getHealth() const { return m_health; }
    int getMaxHealth() const { return m_maxHealth; }
    void setHealth(int hp) { m_health = hp; }
    bool isAlive() const { return m_health > 0 && m_state != EnemyState::DEAD; }
    bool isHurt() const { return m_hurtTimer > 0.0f; }
    bool isAttacking() const { return m_attackAnimTimer > 0.0f; }
    bool hasGun() const { return m_hasGun; }
    void setHasGun(bool hasGun) { m_hasGun = hasGun; }

    float getRadius() const { return m_radius; }
    float getSpeed() const { return m_speed; }
    int getSpriteTextureId() const;
    std::string getDisplayName() const;

    float distanceTo(float px, float py) const {
        float dx = m_x - px;
        float dy = m_y - py;
        return std::sqrt(dx * dx + dy * dy);
    }

    void takeDamage(int amount);

    // Target memory & hunt tracking
    float getLastKnownTargetX() const { return m_lastTargetX; }
    float getLastKnownTargetY() const { return m_lastTargetY; }
    void setLastKnownTarget(float x, float y) { m_lastTargetX = x; m_lastTargetY = y; m_hasTarget = true; }
    bool hasTarget() const { return m_hasTarget; }
    void clearTarget() { m_hasTarget = false; }

    // Timers & AI state accessors
    float& attackCooldown() { return m_attackCooldown; }
    float getAttackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(float cd) { m_attackCooldown = cd; }
    float& hurtTimer() { return m_hurtTimer; }
    float& deathTimer() { return m_deathTimer; }
    float& patrolTimer() { return m_patrolTimer; }
    float& alertTimer() { return m_alertTimer; }
    float& attackAnimTimer() { return m_attackAnimTimer; }

    // Frame-rate independent cooldown update & attack logic
    void updateTimers(float dt);
    EnemyAttackResult tryAttack(float dist, bool hasLoS);

private:
    EnemyType m_type;
    EnemyState m_state = EnemyState::ALERT_CHASE; // Spawn ready to hunt

    float m_x;
    float m_y;
    float m_dirX;
    float m_dirY;

    int m_health;
    int m_maxHealth;
    float m_speed;
    float m_radius;
    bool m_hasGun = false;

    // Last known hunt target position
    float m_lastTargetX = 16.0f;
    float m_lastTargetY = 16.0f;
    bool m_hasTarget = false;

    // AI & combat timers
    float m_attackCooldown = 0.0f;
    float m_hurtTimer = 0.0f;
    float m_deathTimer = 0.0f;
    float m_patrolTimer = 0.0f;
    float m_alertTimer = 0.0f;
    float m_attackAnimTimer = 0.0f;
};

#endif // ENEMY_H

