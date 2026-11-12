#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <algorithm>

#include <QObject>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QFile>

enum class SoundType {
    WEAPON_FIRE_PISTOL = 0,
    WEAPON_FIRE_RIFLE = 1,
    WEAPON_FIRE_SNIPER = 2,
    WEAPON_RELOAD = 3,
    TASER_ZAP = 4,
    TERMINATOR_ALERT = 5,
    TERMINATOR_MELEE = 6,
    UI_WIRE_CLICK = 7,
    TERMINAL_TYPING = 8,
    VAULT_DOOR_OPEN = 9
};

// Spatialized SoundEffect wrapper adding 2.5D stereo pan tracking
class SoundEffect : public QSoundEffect {
    Q_OBJECT

public:
    explicit SoundEffect(QObject *parent = nullptr)
        : QSoundEffect(parent), m_pan(0.0f) {}

    void setPan(float pan) {
        m_pan = std::clamp(pan, -1.0f, 1.0f);
    }

    float pan() const { return m_pan; }

private:
    float m_pan = 0.0f;
};

class AudioManager : public QObject {
    Q_OBJECT

public:
    static constexpr float MAX_HEARING_DISTANCE = 15.0f; // Max audio dropoff distance (tiles)
    static constexpr int POOL_SIZE_PER_SOUND = 4;        // Concurrent channel pool size

    static AudioManager& instance();

    // Listener (local player) state update
    void setListener(float x, float y, float dirX, float dirY);
    float getListenerX() const { return m_listenerX; }
    float getListenerY() const { return m_listenerY; }

    // Direct (non-spatial) sound playback for 2D UI or local actions
    void playSound(SoundType type, float volume = 1.0f, float pan = 0.0f);

    // 2.5D Spatial sound playback using current listener state
    void playSoundAt(SoundType type, float sourceX, float sourceY);

    // 2.5D Spatial sound playback with explicit listener coordinates
    void playSoundAt(SoundType type, float sourceX, float sourceY,
                     float listenerX, float listenerY,
                     float listenerDirX, float listenerDirY);

    // Background Ambience (QMediaPlayer + QAudioOutput loop)
    void startAmbience();
    void pauseAmbience();
    void stopAmbience();
    void setAmbienceVolume(float volume);

    // Master Volume
    void setMasterVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }

private:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager() override = default;

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void initializeSounds();
    void loadSoundEffectPool(SoundType type, const QString& resourceSubpath);

    struct SoundChannelPool {
        std::vector<std::unique_ptr<SoundEffect>> channels;
        int nextIndex = 0;
        bool available = false;

        SoundEffect* getNext() {
            if (channels.empty() || !available) return nullptr;
            SoundEffect* effect = channels[nextIndex].get();
            nextIndex = (nextIndex + 1) % static_cast<int>(channels.size());
            return effect;
        }
    };

    std::unordered_map<SoundType, SoundChannelPool> m_soundPools;

    // Background ambience
    QMediaPlayer* m_mediaPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    bool m_ambienceLoaded = false;

    // Listener position and direction
    float m_listenerX = 16.5f;
    float m_listenerY = 16.5f;
    float m_listenerDirX = 0.0f;
    float m_listenerDirY = -1.0f;

    float m_masterVolume = 1.0f;
};

#endif // AUDIO_MANAGER_H

