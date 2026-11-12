#include "NetProtocol.h"
#include <QIODevice>

void NetProtocol::configureStream(QDataStream& stream) {
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
}

QByteArray NetProtocol::createTcpFrame(PacketType type, const QByteArray& payload) {
    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    configureStream(stream);

    // Payload size = 1 byte (packetType) + payload.size()
    quint32 payloadSize = static_cast<quint32>(sizeof(quint8) + payload.size());

    stream << payloadSize;
    stream << static_cast<quint8>(type);

    if (!payload.isEmpty()) {
        stream.writeRawData(payload.constData(), payload.size());
    }

    return frame;
}

QByteArray NetProtocol::serializeBeacon(const LobbyBeacon& beacon) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    configureStream(stream);

    stream << static_cast<quint8>(PacketType::LOBBY_BEACON);
    stream << beacon.lobbyCode;
    stream << beacon.hostName;
    stream << beacon.tcpPort;
    stream << beacon.currentPlayers;
    stream << beacon.maxPlayers;
    stream << beacon.inGame;

    return data;
}

bool NetProtocol::deserializeBeacon(const QByteArray& data, LobbyBeacon& outBeacon) {
    QDataStream stream(data);
    configureStream(stream);

    quint8 typeVal;
    stream >> typeVal;
    if (static_cast<PacketType>(typeVal) != PacketType::LOBBY_BEACON) {
        return false;
    }

    stream >> outBeacon.lobbyCode;
    stream >> outBeacon.hostName;
    stream >> outBeacon.tcpPort;
    stream >> outBeacon.currentPlayers;
    stream >> outBeacon.maxPlayers;
    stream >> outBeacon.inGame;

    return stream.status() == QDataStream::Ok;
}

QByteArray NetProtocol::serializeJoinRequest(const QString& playerName, const QString& lobbyCode) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    configureStream(stream);

    stream << playerName;
    stream << lobbyCode;

    return data;
}

bool NetProtocol::deserializeJoinRequest(const QByteArray& data, QString& outPlayerName, QString& outLobbyCode) {
    QDataStream stream(data);
    configureStream(stream);

    stream >> outPlayerName;
    stream >> outLobbyCode;

    return stream.status() == QDataStream::Ok;
}

QByteArray NetProtocol::serializeJoinResponse(bool success, const QString& message, quint8 assignedId, quint8 maxPlayers) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    configureStream(stream);

    stream << success;
    stream << message;
    stream << assignedId;
    stream << maxPlayers;

    return data;
}

bool NetProtocol::deserializeJoinResponse(const QByteArray& data, bool& outSuccess, QString& outMessage, quint8& outAssignedId, quint8& outMaxPlayers) {
    QDataStream stream(data);
    configureStream(stream);

    stream >> outSuccess;
    stream >> outMessage;
    stream >> outAssignedId;
    stream >> outMaxPlayers;

    return stream.status() == QDataStream::Ok;
}

QByteArray NetProtocol::serializeLobbyUpdate(const QString& lobbyCode, quint8 maxPlayers, const QStringList& playerNames, bool isHost) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    configureStream(stream);

    stream << lobbyCode;
    stream << maxPlayers;
    stream << static_cast<quint8>(playerNames.size());
    for (const auto& name : playerNames) {
        stream << name;
    }
    stream << isHost;

    return data;
}

bool NetProtocol::deserializeLobbyUpdate(const QByteArray& data, QString& outLobbyCode, quint8& outMaxPlayers, QStringList& outPlayerNames, bool& outIsHost) {
    QDataStream stream(data);
    configureStream(stream);

    stream >> outLobbyCode;
    stream >> outMaxPlayers;

    quint8 count = 0;
    stream >> count;
    outPlayerNames.clear();
    for (quint8 i = 0; i < count; ++i) {
        QString name;
        stream >> name;
        outPlayerNames.append(name);
    }
    stream >> outIsHost;

    return stream.status() == QDataStream::Ok;
}

QByteArray NetProtocol::serializeGameStart() {
    return QByteArray();
}

QByteArray NetProtocol::serializeBeginProtocol() {
    return QByteArray();
}

QByteArray NetProtocol::serializeClientInput(const ClientInputPacket& input) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    configureStream(stream);

    stream << input.playerId;
    stream << input.posX;
    stream << input.posY;
    stream << input.dirX;
    stream << input.dirY;
    stream << input.activeItemType;

    stream << input.firedShot;
    stream << input.droppedItem;
    stream << input.repairSector;
    stream << input.solveRiddleSector;
    stream << input.openServerBulkhead;
    stream << input.reviveTargetPlayerId;

    return data;
}

bool NetProtocol::deserializeClientInput(const QByteArray& data, ClientInputPacket& outInput) {
    QDataStream stream(data);
    configureStream(stream);

    stream >> outInput.playerId;
    stream >> outInput.posX;
    stream >> outInput.posY;
    stream >> outInput.dirX;
    stream >> outInput.dirY;
    stream >> outInput.activeItemType;

    stream >> outInput.firedShot;
    stream >> outInput.droppedItem;
    stream >> outInput.repairSector;
    stream >> outInput.solveRiddleSector;
    stream >> outInput.openServerBulkhead;
    stream >> outInput.reviveTargetPlayerId;

    return stream.status() == QDataStream::Ok;
}

QByteArray NetProtocol::serializeWorldState(const WorldStatePacket& state) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    configureStream(stream);

    stream << state.countdownSeconds;
    stream << state.isGameOver;
    stream << state.isVictory;

    // Players
    quint8 pCount = static_cast<quint8>(state.players.size());
    stream << pCount;
    for (const auto& p : state.players) {
        stream << p.id;
        stream << p.name;
        stream << p.posX;
        stream << p.posY;
        stream << p.dirX;
        stream << p.dirY;
        stream << p.health;
        stream << p.activeItemType;
        stream << p.isAlive;
        stream << p.isDowned;
        stream << p.cprTimer;
    }

    // Enemies
    quint16 eCount = static_cast<quint16>(state.enemies.size());
    stream << eCount;
    for (const auto& e : state.enemies) {
        stream << e.type;
        stream << e.state;
        stream << e.posX;
        stream << e.posY;
        stream << e.health;
        stream << e.hasGun;
    }

    // World Items
    quint16 iCount = static_cast<quint16>(state.items.size());
    stream << iCount;
    for (const auto& item : state.items) {
        stream << item.posX;
        stream << item.posY;
        stream << item.itemType;
        stream << item.count;
        stream << item.ammoInClip;
    }

    // Task progression
    stream << state.electricalFixedMask;
    stream << state.riddleSolvedMask;
    stream << state.vaultDoorsOpenMask;
    stream << state.vaultKeysCollectedMask;

    return data;
}

bool NetProtocol::deserializeWorldState(const QByteArray& data, WorldStatePacket& outState) {
    QDataStream stream(data);
    configureStream(stream);

    stream >> outState.countdownSeconds;
    stream >> outState.isGameOver;
    stream >> outState.isVictory;

    // Players
    quint8 pCount = 0;
    stream >> pCount;
    outState.players.resize(pCount);
    for (quint8 i = 0; i < pCount; ++i) {
        auto& p = outState.players[i];
        stream >> p.id;
        stream >> p.name;
        stream >> p.posX;
        stream >> p.posY;
        stream >> p.dirX;
        stream >> p.dirY;
        stream >> p.health;
        stream >> p.activeItemType;
        stream >> p.isAlive;
        stream >> p.isDowned;
        stream >> p.cprTimer;
    }

    // Enemies
    quint16 eCount = 0;
    stream >> eCount;
    outState.enemies.resize(eCount);
    for (quint16 i = 0; i < eCount; ++i) {
        auto& e = outState.enemies[i];
        stream >> e.type;
        stream >> e.state;
        stream >> e.posX;
        stream >> e.posY;
        stream >> e.health;
        stream >> e.hasGun;
    }

    // World Items
    quint16 iCount = 0;
    stream >> iCount;
    outState.items.resize(iCount);
    for (quint16 i = 0; i < iCount; ++i) {
        auto& item = outState.items[i];
        stream >> item.posX;
        stream >> item.posY;
        stream >> item.itemType;
        stream >> item.count;
        stream >> item.ammoInClip;
    }

    // Task progression
    stream >> outState.electricalFixedMask;
    stream >> outState.riddleSolvedMask;
    stream >> outState.vaultDoorsOpenMask;
    stream >> outState.vaultKeysCollectedMask;

    return stream.status() == QDataStream::Ok;
}

