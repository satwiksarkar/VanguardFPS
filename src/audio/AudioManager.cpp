#include "AudioManager.h"
#include <QFileInfo>
#include <QDir>

AudioManager& AudioManager::instance() {
    static AudioManager s_instance;
    return s_instance;
}

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
{
    initializeSounds();
}

void AudioManager::initializeSounds() {
    // 1. Initialize Sound Effects Pools
    loadSoundEffectPool(SoundType::WEAPON_FIRE_PISTOL, "assets/audio/pistol_fire.wav");
    loadSoundEffectPool(SoundType::WEAPON_FIRE_RIFLE, "assets/audio/rifle_fire.wav");
    loadSoundEffectPool(SoundType::WEAPON_FIRE_SNIPER, "assets/audio/sniper_fire.wav");
    loadSoundEffectPool(SoundType::WEAPON_RELOAD, "assets/audio/reload.wav");
    loadSoundEffectPool(SoundType::TASER_ZAP, "assets/audio/taser_zap.wav");
    loadSoundEffectPool(SoundType::TERMINATOR_ALERT, "assets/audio/terminator_alert.wav");
    loadSoundEffectPool(SoundType::TERMINATOR_MELEE, "assets/audio/terminator_melee.wav");
    loadSoundEffectPool(SoundType::UI_WIRE_CLICK, "assets/audio/ui_wire_click.wav");
    loadSoundEffectPool(SoundType::TERMINAL_TYPING, "assets/audio/terminal_typing.wav");
    loadSoundEffectPool(SoundType::VAULT_DOOR_OPEN, "assets/audio/vault_door.wav");

    // 2. Initialize Background Ambience via QMediaPlayer + QAudioOutput
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.20f); // Low-volume ominous industrial drone
    m_mediaPlayer->setLoops(QMediaPlayer::Infinite);

    QString ambienceQrc = ":/assets/audio/ambience_drone.wav";
    QString ambienceDisk = "assets/audio/ambience_drone.wav";

    if (QFile::exists(ambienceQrc)) {
        m_mediaPlayer->setSource(QUrl("qrc" + ambienceQrc));
        m_ambienceLoaded = true;
    } else if (QFile::exists(ambienceDisk)) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(ambienceDisk));
        m_ambienceLoaded = true;
    } else {
        // Silently handle missing audio file without crashing
        m_ambienceLoaded = false;
    }
}

void AudioManager::loadSoundEffectPool(SoundType type, const QString& resourceSubpath) {
    SoundChannelPool pool;
    pool.available = false;

    QString qrcPath = ":/" + resourceSubpath;
    QString diskPath = resourceSubpath;
    QUrl sourceUrl;

    if (QFile::exists(qrcPath)) {
        sourceUrl = QUrl("qrc" + qrcPath);
        pool.available = true;
    } else if (QFile::exists(diskPath)) {
        sourceUrl = QUrl::fromLocalFile(diskPath);
        pool.available = true;
    } else {
        // Silently catch missing file error without crashing game
        pool.available = false;
        m_soundPools[type] = std::move(pool);
        return;
    }

    pool.channels.reserve(POOL_SIZE_PER_SOUND);
    for (int i = 0; i < POOL_SIZE_PER_SOUND; ++i) {
        auto effect = std::make_unique<SoundEffect>(this);
        effect->setSource(sourceUrl);
        effect->setLoopCount(1);
        effect->setVolume(m_masterVolume);
        pool.channels.push_back(std::move(effect));
    }

    m_soundPools[type] = std::move(pool);
}

void AudioManager::setListener(float x, float y, float dirX, float dirY) {
    m_listenerX = x;
    m_listenerY = y;
    float len = std::sqrt(dirX * dirX + dirY * dirY);
    if (len > 0.0001f) {
        m_listenerDirX = dirX / len;
        m_listenerDirY = dirY / len;
    }
}

void AudioManager::playSound(SoundType type, float volume, float pan) {
    auto it = m_soundPools.find(type);
    if (it == m_soundPools.end() || !it->second.available) {
        return; // Silently skip missing or uninitialized sounds
    }

    SoundEffect* effect = it->second.getNext();
    if (!effect) return;

    // Set 2.5D spatial pan and master-scaled volume
    effect->setPan(pan);
    float finalVolume = std::clamp(volume * m_masterVolume, 0.0f, 1.0f);
    effect->setVolume(finalVolume);
    effect->play();
}

void AudioManager::playSoundAt(SoundType type, float sourceX, float sourceY) {
    playSoundAt(type, sourceX, sourceY, m_listenerX, m_listenerY, m_listenerDirX, m_listenerDirY);
}

void AudioManager::playSoundAt(SoundType type, float sourceX, float sourceY,
                               float listenerX, float listenerY,
                               float listenerDirX, float listenerDirY)
{
    float dx = sourceX - listenerX;
    float dy = sourceY - listenerY;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Drop sound if beyond maximum hearing radius
    if (dist > MAX_HEARING_DISTANCE) {
        return;
    }

    // Inverse linear volume attenuation
    float volume = std::clamp(1.0f - (dist / MAX_HEARING_DISTANCE), 0.0f, 1.0f);

    // Calculate relative azimuth angle for stereo panning
    // Right vector is perpendicular to look direction: (-dirY, dirX)
    float rightX = -listenerDirY;
    float rightY = listenerDirX;

    float toSourceX = (dist > 0.001f) ? (dx / dist) : 0.0f;
    float toSourceY = (dist > 0.001f) ? (dy / dist) : 0.0f;

    // Dot product with right vector yields pan in [-1.0 (hard left), 1.0 (hard right)]
    float pan = toSourceX * rightX + toSourceY * rightY;
    pan = std::clamp(pan, -1.0f, 1.0f);

    playSound(type, volume, pan);
}

void AudioManager::startAmbience() {
    if (!m_ambienceLoaded || !m_mediaPlayer) return;
    if (m_mediaPlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_mediaPlayer->play();
    }
}

void AudioManager::pauseAmbience() {
    if (!m_ambienceLoaded || !m_mediaPlayer) return;
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
    }
}

void AudioManager::stopAmbience() {
    if (!m_ambienceLoaded || !m_mediaPlayer) return;
    m_mediaPlayer->stop();
}

void AudioManager::setAmbienceVolume(float volume) {
    if (m_audioOutput) {
        m_audioOutput->setVolume(std::clamp(volume, 0.0f, 1.0f));
    }
}

void AudioManager::setMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    if (m_audioOutput) {
        m_audioOutput->setVolume(0.20f * m_masterVolume);
    }
}

