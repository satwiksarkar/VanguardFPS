#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <QString>
#include <QByteArray>
#include <QDataStream>

enum class PacketType : quint8 {
    LOBBY_BEACON = 1,
    JOIN_REQUEST = 2,
    JOIN_RESPONSE = 3,
    LOBBY_UPDATE = 4,
    GAME_START = 5,
    BEGIN_PROTOCOL = 6,
    CLIENT_INPUT = 10,
    WORLD_STATE = 20
};

struct NetPlayerState {
    quint8 id = 0;
    QString name;
    float posX = 0.0f;
    float posY = 0.0f;
    float dirX = 0.0f;
    float dirY = -1.0f;
    qint32 health = 100;
    quint8 activeItemType = 0;
    bool isAlive = true;
    bool isDowned = false;
    float cprTimer = 10.0f; // 10-second countdown for CPR / bleedout
};

struct NetEnemyState {
    quint8 type = 0;
    quint8 state = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    qint32 health = 0;
    bool hasGun = false;
};

struct NetItemState {
    float posX = 0.0f;
    float posY = 0.0f;
    quint8 itemType = 0;
    qint32 count = 0;
    qint32 ammoInClip = 0;
};

struct ClientInputPacket {
    quint8 playerId = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    float dirX = 0.0f;
    float dirY = -1.0f;
    quint8 activeItemType = 0;

    bool firedShot = false;
    bool droppedItem = false;
    quint8 repairSector = 0xFF;      // 0xFF = none, 0..3 = sector index
    quint8 solveRiddleSector = 0xFF;  // 0xFF = none, 0..3 = sector index
    bool openServerBulkhead = false;
    quint8 reviveTargetPlayerId = 0xFF; // 0xFF = none, 0..3 = target player ID to revive
};

struct WorldStatePacket {
    float countdownSeconds = 1800.0f;
    bool isGameOver = false;
    bool isVictory = false;

    std::vector<NetPlayerState> players;
    std::vector<NetEnemyState> enemies;
    std::vector<NetItemState> items;

    quint8 electricalFixedMask = 0;
    quint8 riddleSolvedMask = 0;
    quint8 vaultDoorsOpenMask = 0;
    quint8 vaultKeysCollectedMask = 0;
};

struct LobbyBeacon {
    QString lobbyCode;
    QString hostName;
    quint16 tcpPort = 45450;
    quint8 currentPlayers = 1;
    quint8 maxPlayers = 4;
    bool inGame = false;
};

class NetProtocol {
public:
    static constexpr quint16 DEFAULT_TCP_PORT = 45450;
    static constexpr quint16 DISCOVERY_UDP_PORT = 45455;

    // Helper: Wraps payload in length-prefixed TCP frame (quint32 payloadSize + quint8 packetType + payload)
    static QByteArray createTcpFrame(PacketType type, const QByteArray& payload);

    // Serialization & Deserialization
    static QByteArray serializeBeacon(const LobbyBeacon& beacon);
    static bool deserializeBeacon(const QByteArray& data, LobbyBeacon& outBeacon);

    static QByteArray serializeJoinRequest(const QString& playerName, const QString& lobbyCode);
    static bool deserializeJoinRequest(const QByteArray& data, QString& outPlayerName, QString& outLobbyCode);

    static QByteArray serializeJoinResponse(bool success, const QString& message, quint8 assignedId, quint8 maxPlayers);
    static bool deserializeJoinResponse(const QByteArray& data, bool& outSuccess, QString& outMessage, quint8& outAssignedId, quint8& outMaxPlayers);

    static QByteArray serializeLobbyUpdate(const QString& lobbyCode, quint8 maxPlayers, const QStringList& playerNames, bool isHost);
    static bool deserializeLobbyUpdate(const QByteArray& data, QString& outLobbyCode, quint8& outMaxPlayers, QStringList& outPlayerNames, bool& outIsHost);

    static QByteArray serializeGameStart();
    static QByteArray serializeBeginProtocol();

    static QByteArray serializeClientInput(const ClientInputPacket& input);
    static bool deserializeClientInput(const QByteArray& data, ClientInputPacket& outInput);

    static QByteArray serializeWorldState(const WorldStatePacket& state);
    static bool deserializeWorldState(const QByteArray& data, WorldStatePacket& outState);

    // Standard QDataStream setup helper
    static void configureStream(QDataStream& stream);
};

#endif // NET_PROTOCOL_H

