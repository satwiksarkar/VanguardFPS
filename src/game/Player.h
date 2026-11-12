#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>

constexpr double kPlayerPi = 3.14159265358979323846;

class Map;

class Player {
public:
    Player(float startX = 16.5f, float startY = 16.5f);

    // Getters
    float getX() const { return m_posX; }
    float getY() const { return m_posY; }
    float getDirX() const { return m_dirX; }
    float getDirY() const { return m_dirY; }
    float getPlaneX() const;
    float getPlaneY() const;
    float getCollisionRadius() const { return m_radius; }
    bool isScoped() const { return m_isScoped; }
    bool isDowned() const { return m_isDowned; }
    bool isDead() const { return m_isDead; }
    bool isAlive() const { return !m_isDead; }
    bool hasFlashlight() const { return m_hasFlashlight; }
    void setFlashlight(bool on) { m_hasFlashlight = on; }
    void toggleFlashlight() { m_hasFlashlight = !m_hasFlashlight; }
    int getHealth() const { return m_health; }
    int getMaxHealth() const { return m_maxHealth; }
    float getCprTimer() const { return m_cprTimer; }
    float getCprHealth() const { return m_cprHealth; }

    // Setters
    void setPosition(float x, float y) { m_posX = x; m_posY = y; }
    void setDirection(float dx, float dy) {
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.0001f) {
            m_dirX = dx / len;
            m_dirY = dy / len;
            m_planeX = -m_dirY * 0.66f;
            m_planeY = m_dirX * 0.66f;
        }
    }
    void setScoped(bool scoped) { if (!m_isDowned && !m_isDead) m_isScoped = scoped; }
    void setDowned(bool downed) {
        m_isDowned = downed;
        if (downed) {
            m_sprinting = false;
            m_isScoped = false;
        }
    }
    void setDead(bool dead) {
        m_isDead = dead;
        if (dead) {
            m_isDowned = false;
            m_health = 0;
            m_cprTimer = 0.0f;
            m_cprHealth = 0.0f;
            m_sprinting = false;
            m_isScoped = false;
        }
    }
    void resetCpr() {
        m_cprTimer = 10.0f;
        m_cprHealth = 100.0f;
        m_isDead = false;
    }
    void setCprTimer(float t) {
        m_cprTimer = std::clamp(t, 0.0f, 10.0f);
        m_cprHealth = m_cprTimer * 10.0f;
        if (m_cprTimer <= 0.0f && m_isDowned) {
            setDead(true);
        }
    }
    void updateCpr(float dt) {
        if (m_isDowned && !m_isDead) {
            m_cprTimer = std::max(0.0f, m_cprTimer - dt);
            m_cprHealth = m_cprTimer * 10.0f;
            if (m_cprTimer <= 0.0f) {
                setDead(true);
            }
        }
    }
    void damageCpr(float amount) {
        if (m_isDowned && !m_isDead) {
            m_cprHealth = std::max(0.0f, m_cprHealth - amount);
            m_cprTimer = m_cprHealth / 10.0f;
            if (m_cprHealth <= 0.0f || m_cprTimer <= 0.0f) {
                setDead(true);
            }
        }
    }
    void setHealth(int hp) { m_health = std::clamp(hp, 0, m_maxHealth); }
    void heal(int amount) { m_health = std::min(m_maxHealth, m_health + amount); }
    void takeDamage(int amount) { m_health = std::max(0, m_health - amount); }

    // Controls
    void setMovingForward(bool state) { m_moveForward = state; }
    void setMovingBackward(bool state) { m_moveBackward = state; }
    void setStrafingLeft(bool state) { m_strafeLeft = state; }
    void setStrafingRight(bool state) { m_strafeRight = state; }
    void setRotatingLeft(bool state) { m_rotLeft = state; }
    void setRotatingRight(bool state) { m_rotRight = state; }
    void setSprinting(bool state) { m_sprinting = state; }

    bool isMovingForward() const { return m_moveForward; }
    bool isMovingBackward() const { return m_moveBackward; }
    bool isStrafingLeft() const { return m_strafeLeft; }
    bool isStrafingRight() const { return m_strafeRight; }
    bool isSprinting() const { return m_sprinting; }
    float getSpeed() const {
        float speed = BASE_MOVE_SPEED;
        if (m_sprinting) speed *= SPRINT_MULTIPLIER;
        if (m_isScoped) speed *= SCOPED_MOVE_MULTIPLIER;
        if (m_isDowned) speed *= 0.20f;
        return speed;
    }

    // Update with collision sliding
    void update(float dt, const Map& map);

    // Mouse look
    void rotate(float angleRad);

private:
    float m_posX;
    float m_posY;

    // Direction vector (normalized)
    float m_dirX;
    float m_dirY;

    // 2D raycaster camera plane (determines FOV, perpendicular to dir)
    float m_planeX;
    float m_planeY;

    float m_radius; // Collision radius

    // Player stats
    int m_health = 100;
    int m_maxHealth = 100;
    bool m_isScoped = false;
    bool m_isDowned = false;
    bool m_isDead = false;
    bool m_hasFlashlight = true;
    float m_cprTimer = 10.0f;
    float m_cprHealth = 100.0f;

    // Input states
    bool m_moveForward = false;
    bool m_moveBackward = false;
    bool m_strafeLeft = false;
    bool m_strafeRight = false;
    bool m_rotLeft = false;
    bool m_rotRight = false;
    bool m_sprinting = false;

    static constexpr float BASE_MOVE_SPEED = 3.2f;  // Tiles per second
    static constexpr float SPRINT_MULTIPLIER = 1.6f;
    static constexpr float SCOPED_MOVE_MULTIPLIER = 0.45f;
    static constexpr float KEY_ROT_SPEED = 2.4f;    // Radians per second
};

#endif // PLAYER_H
