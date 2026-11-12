#include "NetworkManager.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <algorithm>

NetworkManager& NetworkManager::instance() {
    static NetworkManager mgr;
    return mgr;
}

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &NetworkManager::onNewTcpConnection);

    m_clientSocket = new QTcpSocket(this);
    connect(m_clientSocket, &QTcpSocket::connected, this, &NetworkManager::onClientSocketConnected);
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &NetworkManager::onClientSocketReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &NetworkManager::onClientSocketDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_clientSocket, &QTcpSocket::errorOccurred, this, &NetworkManager::onClientSocketError);
#endif

    m_beaconTimer = new QTimer(this);
    m_beaconTimer->setInterval(500); // 500ms UDP beacon broadcast
    connect(m_beaconTimer, &QTimer::timeout, this, &NetworkManager::onBeaconTimer);

    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(1200); // Prune stale lobbies
    connect(m_cleanupTimer, &QTimer::timeout, this, &NetworkManager::onCleanupTimer);
    m_cleanupTimer->start();

    startDiscovery();
}

NetworkManager::~NetworkManager() {
    disconnectLobby();
    stopDiscovery();
}

QString NetworkManager::generateRandomLobbyCode() {
    // 32-character alphabet omitting visually ambiguous characters (0, O, 1, I)
    static const char ALPHABET[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    constexpr int ALPHABET_LEN = sizeof(ALPHABET) - 1;

    QString code;
    code.reserve(6);
    for (int i = 0; i < 6; ++i) {
        quint32 index = QRandomGenerator::global()->bounded(ALPHABET_LEN);
        code.append(ALPHABET[index]);
    }
    return code;
}

void NetworkManager::startDiscovery() {
    if (!m_udpSocket) {
        m_udpSocket = new QUdpSocket(this);
        m_udpSocket->bind(QHostAddress::AnyIPv4, NetProtocol::DISCOVERY_UDP_PORT,
                          QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onUdpReadyRead);
    }
}

void NetworkManager::stopDiscovery() {
    if (m_udpSocket) {
        m_udpSocket->close();
        m_udpSocket->deleteLater();
        m_udpSocket = nullptr;
    }
}

void NetworkManager::onUdpReadyRead() {
    if (!m_udpSocket) return;

    bool updated = false;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));
        QHostAddress senderAddr;
        quint16 senderPort = 0;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);

        LobbyBeacon beacon;
        if (NetProtocol::deserializeBeacon(datagram, beacon)) {
            // Ignore if we are hosting this exact lobby code
            if (isHost() && beacon.lobbyCode == m_lobbyCode) {
                continue;
            }

            bool found = false;
            for (auto& item : m_discoveredLobbies) {
                if (item.beacon.lobbyCode == beacon.lobbyCode) {
                    item.beacon = beacon;
                    item.address = senderAddr;
                    item.port = beacon.tcpPort;
                    item.lastSeenTimeMs = now;
                    found = true;
                    updated = true;
                    break;
                }
            }

            if (!found) {
                m_discoveredLobbies.push_back({ beacon, senderAddr, beacon.tcpPort, now });
                updated = true;
            }
        }
    }

    if (updated) {
        emit discoveredLobbiesChanged();
    }
}

void NetworkManager::onBeaconTimer() {
    if (!isHost() || !m_udpSocket) return;

    LobbyBeacon beacon;
    beacon.lobbyCode = m_lobbyCode;
    beacon.hostName = m_localPlayerName;
    beacon.tcpPort = NetProtocol::DEFAULT_TCP_PORT;
    beacon.currentPlayers = static_cast<quint8>(getPlayerNames().size());
    beacon.maxPlayers = m_maxCapacity;
    beacon.inGame = m_isInGame;

    QByteArray data = NetProtocol::serializeBeacon(beacon);

    // Dual broadcast: broadcast to LAN and 127.0.0.1 for local testing
    m_udpSocket->writeDatagram(data, QHostAddress::Broadcast, NetProtocol::DISCOVERY_UDP_PORT);
    m_udpSocket->writeDatagram(data, QHostAddress::LocalHost, NetProtocol::DISCOVERY_UDP_PORT);
}

void NetworkManager::onCleanupTimer() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    size_t countBefore = m_discoveredLobbies.size();

    m_discoveredLobbies.erase(
        std::remove_if(m_discoveredLobbies.begin(), m_discoveredLobbies.end(),
                       [now](const DiscoveredLobbyInfo& info) {
                           return (now - info.lastSeenTimeMs) > 4000; // 4 seconds silence
                       }),
        m_discoveredLobbies.end()
    );

    if (m_discoveredLobbies.size() != countBefore) {
        emit discoveredLobbiesChanged();
    }
}

bool NetworkManager::hostLobby(const QString& hostName, quint8 maxCapacity, quint16 tcpPort) {
    disconnectLobby();

    m_localPlayerName = hostName.trimmed().isEmpty() ? "Host_Operative" : hostName.trimmed();
    m_maxCapacity = std::clamp<quint8>(maxCapacity, 2, 4);
    m_lobbyCode = generateRandomLobbyCode();
    m_localPlayerId = 0; // Host is always player 0
    m_role = NetworkRole::HOST;
    m_isInGame = false;
    m_peers.clear();

    if (!m_tcpServer->listen(QHostAddress::Any, tcpPort)) {
        m_role = NetworkRole::OFFLINE;
        emit connectionFailed("Failed to bind TCP server on port " + QString::number(tcpPort));
        return false;
    }

    startDiscovery();
    m_beaconTimer->start();

    emit lobbyCreated(m_lobbyCode, m_maxCapacity);
    emit lobbyUpdated(getPlayerNames());
    return true;
}

void NetworkManager::onNewTcpConnection() {
    if (!m_tcpServer->hasPendingConnections()) return;

    QTcpSocket* sock = m_tcpServer->nextPendingConnection();
    if (!sock) return;

    // Check if lobby is full or game in progress
    if (m_isInGame || m_peers.size() + 1 >= m_maxCapacity) {
        QByteArray resp = NetProtocol::serializeJoinResponse(false, "Lobby is full or game in progress.", 0, m_maxCapacity);
        QByteArray frame = NetProtocol::createTcpFrame(PacketType::JOIN_RESPONSE, resp);
        sock->write(frame);
        sock->flush();
        sock->disconnectFromHost();
        return;
    }

    // Assign temporary ID
    quint8 assignedId = static_cast<quint8>(m_peers.size() + 1);

    ConnectedPeer peer;
    peer.socket = sock;
    peer.id = assignedId;
    peer.name = QString("Agent_%1").arg(assignedId + 1);

    connect(sock, &QTcpSocket::readyRead, this, &NetworkManager::onPeerSocketReadyRead);
    connect(sock, &QTcpSocket::disconnected, this, &NetworkManager::onPeerSocketDisconnected);

    m_peers.push_back(peer);
}

void NetworkManager::onPeerSocketReadyRead() {
    QTcpSocket* senderSock = qobject_cast<QTcpSocket*>(sender());
    if (!senderSock) return;

    auto it = std::find_if(m_peers.begin(), m_peers.end(), [senderSock](const ConnectedPeer& p) {
        return p.socket == senderSock;
    });
    if (it == m_peers.end()) return;

    ConnectedPeer& peer = *it;
    peer.rxBuffer.append(peer.socket->readAll());

    // Process all framed packets with length prefix
    while (peer.rxBuffer.size() >= static_cast<int>(sizeof(quint32))) {
        QDataStream peekStream(peer.rxBuffer);
        NetProtocol::configureStream(peekStream);
        quint32 payloadSize = 0;
        peekStream >> payloadSize;

        if (peer.rxBuffer.size() < static_cast<int>(sizeof(quint32) + payloadSize)) {
            break; // Awaiting complete frame
        }

        QByteArray frame = peer.rxBuffer.mid(sizeof(quint32), payloadSize);
        peer.rxBuffer.remove(0, sizeof(quint32) + payloadSize);

        if (!frame.isEmpty()) {
            quint8 typeVal = static_cast<quint8>(frame[0]);
            QByteArray payload = frame.mid(1);
            processHostPacket(typeVal, payload, peer);
        }
    }
}

void NetworkManager::onPeerSocketDisconnected() {
    QTcpSocket* senderSock = qobject_cast<QTcpSocket*>(sender());
    if (!senderSock) return;

    auto it = std::find_if(m_peers.begin(), m_peers.end(), [senderSock](const ConnectedPeer& p) {
        return p.socket == senderSock;
    });

    if (it != m_peers.end()) {
        senderSock->deleteLater();
        m_peers.erase(it);

        // Reassign IDs 1..N
        for (size_t i = 0; i < m_peers.size(); ++i) {
            m_peers[i].id = static_cast<quint8>(i + 1);
        }

        broadcastLobbyUpdate();
        emit lobbyUpdated(getPlayerNames());
    }
}

void NetworkManager::processHostPacket(quint8 packetType, const QByteArray& payload, ConnectedPeer& peer) {
    PacketType type = static_cast<PacketType>(packetType);

    switch (type) {
        case PacketType::JOIN_REQUEST: {
            QString playerName, requestedCode;
            if (NetProtocol::deserializeJoinRequest(payload, playerName, requestedCode)) {
                // Validate code if provided
                if (!requestedCode.trimmed().isEmpty() &&
                    requestedCode.trimmed().compare(m_lobbyCode, Qt::CaseInsensitive) != 0)
                {
                    QByteArray resp = NetProtocol::serializeJoinResponse(false, "Invalid 6-character Lobby Code.", 0, m_maxCapacity);
                    peer.socket->write(NetProtocol::createTcpFrame(PacketType::JOIN_RESPONSE, resp));
                    peer.socket->flush();
                    peer.socket->disconnectFromHost();
                    return;
                }

                peer.name = playerName.trimmed().isEmpty() ? QString("Agent_%1").arg(peer.id + 1) : playerName.trimmed();

                // Send success response
                QByteArray resp = NetProtocol::serializeJoinResponse(true, "Authentication confirmed.", peer.id, m_maxCapacity);
                peer.socket->write(NetProtocol::createTcpFrame(PacketType::JOIN_RESPONSE, resp));

                broadcastLobbyUpdate();
                emit lobbyUpdated(getPlayerNames());
            }
            break;
        }

        case PacketType::CLIENT_INPUT: {
            ClientInputPacket input;
            if (NetProtocol::deserializeClientInput(payload, input)) {
                emit clientInputReceived(peer.id, input);
            }
            break;
        }

        default:
            break;
    }
}

void NetworkManager::broadcastLobbyUpdate() {
    if (!isHost()) return;

    QStringList names = getPlayerNames();
    QByteArray updateData = NetProtocol::serializeLobbyUpdate(m_lobbyCode, m_maxCapacity, names, false);
    QByteArray frame = NetProtocol::createTcpFrame(PacketType::LOBBY_UPDATE, updateData);

    for (auto& peer : m_peers) {
        if (peer.socket && peer.socket->state() == QAbstractSocket::ConnectedState) {
            peer.socket->write(frame);
        }
    }
}

void NetworkManager::launchGame() {
    if (!isHost()) return;

    m_isInGame = true;
    QByteArray startFrame = NetProtocol::createTcpFrame(PacketType::GAME_START, NetProtocol::serializeGameStart());

    for (auto& peer : m_peers) {
        if (peer.socket && peer.socket->state() == QAbstractSocket::ConnectedState) {
            peer.socket->write(startFrame);
        }
    }

    emit gameStarted();
}

void NetworkManager::startProtocol() {
    if (!isHost()) return;

    m_isInGame = true;
    QByteArray startFrame = NetProtocol::createTcpFrame(PacketType::BEGIN_PROTOCOL, NetProtocol::serializeBeginProtocol());

    for (auto& peer : m_peers) {
        if (peer.socket && peer.socket->state() == QAbstractSocket::ConnectedState) {
            peer.socket->write(startFrame);
        }
    }

    emit protocolStarted();
}

void NetworkManager::broadcastWorldState(const WorldStatePacket& state) {
    if (!isHost() || m_peers.empty()) return;

    QByteArray data = NetProtocol::serializeWorldState(state);
    QByteArray frame = NetProtocol::createTcpFrame(PacketType::WORLD_STATE, data);

    for (auto& peer : m_peers) {
        if (peer.socket && peer.socket->state() == QAbstractSocket::ConnectedState) {
            peer.socket->write(frame);
        }
    }
}

// ==================== CLIENT METHODS ====================

void NetworkManager::joinLobby(const QString& target, const QString& playerName) {
    disconnectLobby();

    m_localPlayerName = playerName.trimmed().isEmpty() ? "Operative" : playerName.trimmed();
    m_role = NetworkRole::CLIENT;
    m_isInGame = false;
    m_clientRxBuffer.clear();

    QString cleanTarget = target.trimmed();
    QString targetHost = "127.0.0.1";
    quint16 targetPort = NetProtocol::DEFAULT_TCP_PORT;
    m_pendingJoinCode.clear();

    // 1. Check if format is IP:PORT:CODE or IP:CODE
    if (cleanTarget.contains(':')) {
        QStringList tokens = cleanTarget.split(':');
        if (tokens.size() == 2) {
            targetHost = tokens[0].trimmed();
            // Could be IP:PORT or IP:CODE
            bool isPort = false;
            quint16 p = static_cast<quint16>(tokens[1].trimmed().toUShort(&isPort));
            if (isPort) {
                targetPort = p;
            } else {
                m_pendingJoinCode = tokens[1].trimmed();
            }
        } else if (tokens.size() >= 3) {
            targetHost = tokens[0].trimmed();
            targetPort = static_cast<quint16>(tokens[1].trimmed().toUShort());
            m_pendingJoinCode = tokens[2].trimmed();
        }
    } else {
        // Check if cleanTarget is an IP address
        QHostAddress addr;
        if (addr.setAddress(cleanTarget)) {
            targetHost = cleanTarget;
        } else {
            // Assume 6-character Lobby Code
            m_pendingJoinCode = cleanTarget.toUpper();
            bool beaconFound = false;

            for (const auto& l : m_discoveredLobbies) {
                if (l.beacon.lobbyCode.compare(m_pendingJoinCode, Qt::CaseInsensitive) == 0) {
                    targetHost = l.address.toString();
                    targetPort = l.port;
                    beaconFound = true;
                    break;
                }
            }

            if (!beaconFound) {
                // Direct localhost fallback for same-PC testing
                targetHost = "127.0.0.1";
                targetPort = NetProtocol::DEFAULT_TCP_PORT;
            }
        }
    }

    m_clientSocket->connectToHost(targetHost, targetPort);
}

void NetworkManager::onClientSocketConnected() {
    // Connected to host, send JOIN_REQUEST
    QByteArray req = NetProtocol::serializeJoinRequest(m_localPlayerName, m_pendingJoinCode);
    QByteArray frame = NetProtocol::createTcpFrame(PacketType::JOIN_REQUEST, req);
    m_clientSocket->write(frame);
}

void NetworkManager::onClientSocketReadyRead() {
    m_clientRxBuffer.append(m_clientSocket->readAll());

    while (m_clientRxBuffer.size() >= static_cast<int>(sizeof(quint32))) {
        QDataStream peekStream(m_clientRxBuffer);
        NetProtocol::configureStream(peekStream);
        quint32 payloadSize = 0;
        peekStream >> payloadSize;

        if (m_clientRxBuffer.size() < static_cast<int>(sizeof(quint32) + payloadSize)) {
            break; // Awaiting full frame
        }

        QByteArray frame = m_clientRxBuffer.mid(sizeof(quint32), payloadSize);
        m_clientRxBuffer.remove(0, sizeof(quint32) + payloadSize);

        if (!frame.isEmpty()) {
            quint8 typeVal = static_cast<quint8>(frame[0]);
            QByteArray payload = frame.mid(1);
            processClientPacket(typeVal, payload);
        }
    }
}

void NetworkManager::processClientPacket(quint8 packetType, const QByteArray& payload) {
    PacketType type = static_cast<PacketType>(packetType);

    switch (type) {
        case PacketType::JOIN_RESPONSE: {
            bool success = false;
            QString msg;
            quint8 assignedId = 0, maxPlayers = 4;
            if (NetProtocol::deserializeJoinResponse(payload, success, msg, assignedId, maxPlayers)) {
                if (success) {
                    m_localPlayerId = assignedId;
                    m_maxCapacity = maxPlayers;
                    m_lobbyCode = m_pendingJoinCode;
                    emit joinedLobby(m_lobbyCode, assignedId, maxPlayers);
                } else {
                    emit connectionFailed("Host rejected connection: " + msg);
                    disconnectLobby();
                }
            }
            break;
        }

        case PacketType::LOBBY_UPDATE: {
            QString code;
            quint8 maxPlayers;
            QStringList names;
            bool isHostFlag;
            if (NetProtocol::deserializeLobbyUpdate(payload, code, maxPlayers, names, isHostFlag)) {
                m_lobbyCode = code;
                m_maxCapacity = maxPlayers;
                emit lobbyUpdated(names);
            }
            break;
        }

        case PacketType::GAME_START: {
            emit gameStarted();
            break;
        }

        case PacketType::BEGIN_PROTOCOL: {
            m_isInGame = true;
            emit protocolStarted();
            break;
        }

        case PacketType::WORLD_STATE: {
            WorldStatePacket state;
            if (NetProtocol::deserializeWorldState(payload, state)) {
                emit worldStateReceived(state);
            }
            break;
        }

        default:
            break;
    }
}

void NetworkManager::onClientSocketDisconnected() {
    if (m_role == NetworkRole::CLIENT) {
        emit disconnectedFromLobby();
        m_role = NetworkRole::OFFLINE;
        m_isInGame = false;
    }
}

void NetworkManager::onClientSocketError() {
    if (m_role == NetworkRole::CLIENT && !m_isInGame) {
        emit connectionFailed("Network Error: " + m_clientSocket->errorString());
        disconnectLobby();
    }
}

void NetworkManager::sendClientInput(const ClientInputPacket& input) {
    if (!isClient() || !m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray data = NetProtocol::serializeClientInput(input);
    QByteArray frame = NetProtocol::createTcpFrame(PacketType::CLIENT_INPUT, data);
    m_clientSocket->write(frame);
}

void NetworkManager::disconnectLobby() {
    if (m_beaconTimer->isActive()) {
        m_beaconTimer->stop();
    }

    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    for (auto& peer : m_peers) {
        if (peer.socket) {
            peer.socket->disconnectFromHost();
            peer.socket->deleteLater();
        }
    }
    m_peers.clear();

    if (m_clientSocket->state() != QAbstractSocket::UnconnectedState) {
        m_clientSocket->disconnectFromHost();
    }
    m_clientRxBuffer.clear();

    m_role = NetworkRole::OFFLINE;
    m_isInGame = false;
    m_localPlayerId = 0;
}

QStringList NetworkManager::getPlayerNames() const {
    QStringList list;
    if (isHost()) {
        list.append(m_localPlayerName + " (Host)");
        for (const auto& peer : m_peers) {
            list.append(peer.name);
        }
    } else if (isClient()) {
        // Will be populated via LOBBY_UPDATE packets
        list.append(m_localPlayerName);
    } else {
        list.append(m_localPlayerName);
    }
    return list;
}

