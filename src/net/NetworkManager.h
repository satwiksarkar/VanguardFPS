#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <cstdint>
#include <vector>
#include <memory>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QDateTime>

#include "NetProtocol.h"

enum class NetworkRole {
    OFFLINE,
    HOST,
    CLIENT
};

struct DiscoveredLobbyInfo {
    LobbyBeacon beacon;
    QHostAddress address;
    quint16 port = 0;
    qint64 lastSeenTimeMs = 0;
};

class NetworkManager : public QObject {
    Q_OBJECT

public:
    static NetworkManager& instance();

    // Lifecycle
    NetworkRole getRole() const { return m_role; }
    bool isHost() const { return m_role == NetworkRole::HOST; }
    bool isClient() const { return m_role == NetworkRole::CLIENT; }
    bool isOffline() const { return m_role == NetworkRole::OFFLINE; }

    quint8 getLocalPlayerId() const { return m_localPlayerId; }
    QString getLocalPlayerName() const { return m_localPlayerName; }
    QString getLobbyCode() const { return m_lobbyCode; }
    quint8 getMaxCapacity() const { return m_maxCapacity; }
    QStringList getPlayerNames() const;

    // Discovery (always active so LAN lobbies can be browsed)
    void startDiscovery();
    void stopDiscovery();
    const std::vector<DiscoveredLobbyInfo>& getDiscoveredLobbies() const { return m_discoveredLobbies; }

    // Host actions
    bool hostLobby(const QString& hostName, quint8 maxCapacity = 4, quint16 tcpPort = NetProtocol::DEFAULT_TCP_PORT);
    void launchGame();
    void startProtocol();

    // Client actions
    void joinLobby(const QString& target, const QString& playerName);
    void disconnectLobby();

    // In-game communication
    void sendClientInput(const ClientInputPacket& input);
    void broadcastWorldState(const WorldStatePacket& state);

signals:
    // Discovery
    void discoveredLobbiesChanged();

    // Lobby events
    void lobbyCreated(const QString& lobbyCode, quint8 maxCapacity);
    void joinedLobby(const QString& lobbyCode, quint8 assignedId, quint8 maxCapacity);
    void lobbyUpdated(const QStringList& playerNames);
    void connectionFailed(const QString& reason);
    void disconnectedFromLobby();
    void gameStarted();
    void protocolStarted();

    // In-game packets
    void clientInputReceived(quint8 playerId, const ClientInputPacket& input);
    void worldStateReceived(const WorldStatePacket& state);

private slots:
    void onNewTcpConnection();
    void onPeerSocketReadyRead();
    void onPeerSocketDisconnected();
    void onClientSocketReadyRead();
    void onClientSocketConnected();
    void onClientSocketDisconnected();
    void onClientSocketError();
    void onUdpReadyRead();
    void onBeaconTimer();
    void onCleanupTimer();

private:
    struct ConnectedPeer {
        QTcpSocket* socket = nullptr;
        quint8 id = 0;
        QString name;
        QByteArray rxBuffer;
    };

    NetworkManager(QObject* parent = nullptr);
    ~NetworkManager() override;

    void processHostPacket(quint8 packetType, const QByteArray& payload, ConnectedPeer& peer);
    void processClientPacket(quint8 packetType, const QByteArray& payload);
    void broadcastLobbyUpdate();
    static QString generateRandomLobbyCode();

    NetworkRole m_role = NetworkRole::OFFLINE;
    quint8 m_localPlayerId = 0;
    QString m_localPlayerName = "Operative";
    QString m_lobbyCode;
    quint8 m_maxCapacity = 4;
    bool m_isInGame = false;

    // UDP Discovery
    QUdpSocket* m_udpSocket = nullptr;
    QTimer* m_beaconTimer = nullptr;
    QTimer* m_cleanupTimer = nullptr;
    std::vector<DiscoveredLobbyInfo> m_discoveredLobbies;

    // Host TCP server
    QTcpServer* m_tcpServer = nullptr;
    std::vector<ConnectedPeer> m_peers;

    // Client TCP socket
    QTcpSocket* m_clientSocket = nullptr;
    QByteArray m_clientRxBuffer;
    QString m_pendingJoinCode;
};

#endif // NETWORK_MANAGER_H

