#ifndef MAP_H
#define MAP_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

constexpr double kPi = 3.14159265358979323846;

enum class LightState : uint8_t {
    OFF = 0,
    LOW_GLOW = 1,
    FLICKERING = 2,
    BRIGHT_RESTORED = 3
};

class Map {
public:
    static constexpr int DEFAULT_WIDTH = 32;
    static constexpr int DEFAULT_HEIGHT = 32;
    static constexpr int WIDTH = DEFAULT_WIDTH;   // Legacy fallback alias
    static constexpr int HEIGHT = DEFAULT_HEIGHT; // Legacy fallback alias

    Map();
    Map(int width, int height);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    void resize(int width, int height);
    void clear(int fillTile = 1);

    // Tile access
    int getTile(int x, int y) const;
    void setTile(int x, int y, int tile);
    bool isWall(int x, int y) const;
    bool isWalkable(float x, float y, float radius = 0.25f) const;

    // Light map
    LightState getLightState(int x, int y) const;
    void setLightState(int x, int y, LightState state);
    float getLightIntensity(int x, int y, float timeSeconds) const;
    void restoreSectorLighting(int sector);

    // Descriptive name of tile (for HUD interaction prompts)
    static std::string getTileName(int tileId);
    static bool isInteractive(int tileId);

private:
    void initializeMap();
    void initializeLights();

    int m_width = DEFAULT_WIDTH;
    int m_height = DEFAULT_HEIGHT;
    std::vector<std::vector<int>> m_tiles;
    std::vector<std::vector<LightState>> m_lightStates;
    std::vector<std::vector<float>> m_flickerPhase;
};

#endif // MAP_H
