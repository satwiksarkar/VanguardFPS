#include "Player.h"
#include "world/Map.h"
#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>

Player::Player(float startX, float startY)
    : m_posX(startX)
    , m_posY(startY)
    , m_dirX(0.0f)
    , m_dirY(-1.0f)   // Facing North
    , m_planeX(0.66f) // Perpendicular camera plane (~66° FOV)
    , m_planeY(0.0f)
    , m_radius(0.28f)
    , m_health(100)
    , m_maxHealth(100)
    , m_isScoped(false)
{
}

float Player::getPlaneX() const {
    return m_isScoped ? (m_planeX * 0.28f) : m_planeX;
}

float Player::getPlaneY() const {
    return m_isScoped ? (m_planeY * 0.28f) : m_planeY;
}

void Player::rotate(float angleRad) {
    if (m_isDead) return;

    float oldDirX = m_dirX;
    m_dirX = m_dirX * std::cos(angleRad) - m_dirY * std::sin(angleRad);
    m_dirY = oldDirX * std::sin(angleRad) + m_dirY * std::cos(angleRad);

    float oldPlaneX = m_planeX;
    m_planeX = m_planeX * std::cos(angleRad) - m_planeY * std::sin(angleRad);
    m_planeY = oldPlaneX * std::sin(angleRad) + m_planeY * std::cos(angleRad);
}

void Player::update(float dt, const Map& map) {
    if (m_isDead) return;

    // Keyboard rotation
    float rotMultiplier = m_isScoped ? 0.4f : 1.0f;
    if (m_rotLeft) {
        rotate(-KEY_ROT_SPEED * rotMultiplier * dt);
    }
    if (m_rotRight) {
        rotate(KEY_ROT_SPEED * rotMultiplier * dt);
    }

    // Directional velocity calculation
    float speed = BASE_MOVE_SPEED;
    if (m_isDowned) {
        speed *= 0.20f; // Crawl at 20% speed when downed
    } else {
        if (m_sprinting && !m_isScoped) {
            speed *= SPRINT_MULTIPLIER;
        }
        if (m_isScoped) {
            speed *= SCOPED_MOVE_MULTIPLIER;
        }
    }

    float moveVectorX = 0.0f;
    float moveVectorY = 0.0f;

    // Forward / backward
    if (m_moveForward) {
        moveVectorX += m_dirX;
        moveVectorY += m_dirY;
    }
    if (m_moveBackward) {
        moveVectorX -= m_dirX;
        moveVectorY -= m_dirY;
    }

    // Strafe left / right (perpendicular to direction)
    if (m_strafeRight) {
        moveVectorX += -m_dirY;
        moveVectorY += m_dirX;
    }
    if (m_strafeLeft) {
        moveVectorX -= -m_dirY;
        moveVectorY -= m_dirX;
    }

    // Normalize movement vector to avoid faster diagonal speed
    float lengthSq = moveVectorX * moveVectorX + moveVectorY * moveVectorY;
    if (lengthSq > 0.0001f) {
        float length = std::sqrt(lengthSq);
        float stepX = (moveVectorX / length) * speed * dt;
        float stepY = (moveVectorY / length) * speed * dt;

        // Collision Sliding: check X axis first, then Y axis independently
        if (map.isWalkable(m_posX + stepX, m_posY, m_radius)) {
            m_posX += stepX;
        }

        if (map.isWalkable(m_posX, m_posY + stepY, m_radius)) {
            m_posY += stepY;
        }
    }
}
