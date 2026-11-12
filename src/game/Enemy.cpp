#include "Enemy.h"

Enemy::Enemy(EnemyType type, float startX, float startY)
    : m_type(type)
    , m_x(startX)
    , m_y(startY)
    , m_dirX(1.0f)
    , m_dirY(0.0f)
{
    switch (m_type) {
        case EnemyType::TASER_DRONE:
            m_maxHealth = 40;
            m_health = 40;
            m_speed = 2.6f;
            m_radius = 0.32f;
            m_hasGun = false;
            break;

        case EnemyType::GUN_DRONE:
            m_maxHealth = 65;
            m_health = 65;
            m_speed = 1.8f;
            m_radius = 0.36f;
            m_hasGun = true;
            break;

        case EnemyType::TERMINATOR_ROBOT:
            m_maxHealth = 150;
            m_health = 150;
            m_speed = 1.35f;
            m_radius = 0.42f;
            // Exact 50% gun spawn probability
            m_hasGun = (QRandomGenerator::global()->bounded(2) == 1);
            break;
    }

    // Initial random patrol direction
    float angle = QRandomGenerator::global()->generateDouble() * 6.2831853f;
    m_dirX = std::cos(angle);
    m_dirY = std::sin(angle);
}

void Enemy::setDirection(float dx, float dy) {
    float lenSq = dx * dx + dy * dy;
    if (lenSq > 0.0001f) {
        float len = std::sqrt(lenSq);
        m_dirX = dx / len;
        m_dirY = dy / len;
    }
}

int Enemy::getSpriteTextureId() const {
    if (m_state == EnemyState::DEAD) {
        return 124; // Debris / dead chassis sprite
    }

    switch (m_type) {
        case EnemyType::TASER_DRONE:
            return 120;
        case EnemyType::GUN_DRONE:
            return 121;
        case EnemyType::TERMINATOR_ROBOT:
            return m_hasGun ? 123 : 122;
        default:
            return 120;
    }
}

std::string Enemy::getDisplayName() const {
    switch (m_type) {
        case EnemyType::TASER_DRONE:
            return "Vanguard Taser Drone";
        case EnemyType::GUN_DRONE:
            return "Vanguard Gun Drone";
        case EnemyType::TERMINATOR_ROBOT:
            return m_hasGun ? "Heavy Terminator [Rotary Gatling]" : "Heavy Terminator [Melee Brute]";
        default:
            return "Vanguard Unit";
    }
}

void Enemy::takeDamage(int amount) {
    if (m_state == EnemyState::DEAD) return;

    m_health -= amount;
    m_hurtTimer = 0.16f; // Trigger hurt flash animation

    if (m_health <= 0) {
        m_health = 0;
        m_state = EnemyState::DEAD;
        m_deathTimer = 0.0f;
    } else {
        // Shooting an enemy alerts it and forces aggressive chase
        m_state = EnemyState::ALERT_CHASE;
        m_alertTimer = 12.0f; // Extended chase memory
    }
}

void Enemy::updateTimers(float dt) {
    if (m_attackCooldown > 0.0f) {
        m_attackCooldown = std::max(0.0f, m_attackCooldown - dt);
    }
    if (m_hurtTimer > 0.0f) {
        m_hurtTimer = std::max(0.0f, m_hurtTimer - dt);
    }
    if (m_attackAnimTimer > 0.0f) {
        m_attackAnimTimer = std::max(0.0f, m_attackAnimTimer - dt);
        if (m_attackAnimTimer == 0.0f && m_state == EnemyState::ATTACK) {
            m_state = EnemyState::ALERT_CHASE;
        }
    }
}

EnemyAttackResult Enemy::tryAttack(float dist, bool hasLoS) {
    EnemyAttackResult result;
    if (!isAlive() || m_attackCooldown > 0.0f) {
        return result;
    }

    switch (m_type) {
        case EnemyType::TASER_DRONE: {
            // Taser Drone: If distance < 1.2 tiles and attackCooldown <= 0, deal 12 damage to the player and reset cooldown to 1.2s.
            if (dist < 1.2f) {
                result.didAttack = true;
                result.damage = 12;
                result.isElectricZap = true;
                m_attackCooldown = 1.2f;
                m_attackAnimTimer = 0.25f;
                m_state = EnemyState::ATTACK;
            }
            break;
        }

        case EnemyType::GUN_DRONE: {
            // Gun Drone: If distance < 8.0 tiles with clear line-of-sight and attackCooldown <= 0, deal 8 damage and reset cooldown to 1.5s.
            if (dist < 8.0f && hasLoS) {
                result.didAttack = true;
                result.damage = 8;
                result.isElectricZap = false;
                m_attackCooldown = 1.5f;
                m_attackAnimTimer = 0.25f;
                m_state = EnemyState::ATTACK;
            }
            break;
        }

        case EnemyType::TERMINATOR_ROBOT: {
            // Terminator (Melee): If distance < 1.3 tiles and attackCooldown <= 0, deal 25 damage and reset cooldown to 1.5s.
            // Terminator (Armed): Can attack via melee if close (< 1.3 tiles, 25 dmg), OR fire a projectile/hitscan dealing 15 damage if within 9.0 tiles with LoS, resetting cooldown to 1.2s.
            if (dist < 1.3f) {
                result.didAttack = true;
                result.damage = 25;
                result.isElectricZap = false;
                m_attackCooldown = m_hasGun ? 1.2f : 1.5f;
                m_attackAnimTimer = 0.30f;
                m_state = EnemyState::ATTACK;
            } else if (m_hasGun && dist <= 9.0f && hasLoS) {
                result.didAttack = true;
                result.damage = 15;
                result.isElectricZap = false;
                m_attackCooldown = 1.2f;
                m_attackAnimTimer = 0.25f;
                m_state = EnemyState::ATTACK;
            }
            break;
        }
    }

    return result;
}

