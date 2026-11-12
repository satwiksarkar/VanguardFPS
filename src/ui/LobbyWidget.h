#ifndef LOBBY_WIDGET_H
#define LOBBY_WIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>

class GameWidget;
class MissionBriefingWidget;
class StoryboardWidget;

class LobbyWidget : public QWidget {
    Q_OBJECT

public:
    explicit LobbyWidget(QWidget *parent = nullptr);
    ~LobbyWidget() override = default;

private slots:
    void onSoloClicked();
    void onHostClicked();
    void onJoinClicked();
    void onQuickJoinLobbyItem(QListWidgetItem *item);
    void onCopyCodeClicked();
    void onLaunchMissionClicked();
    void onLeaveLobbyClicked();

    // NetworkManager hooks
    void onDiscoveredLobbiesChanged();
    void onLobbyCreated(const QString& lobbyCode, quint8 maxCapacity);
    void onJoinedLobby(const QString& lobbyCode, quint8 assignedId, quint8 maxCapacity);
    void onLobbyUpdated(const QStringList& playerNames);
    void onConnectionFailed(const QString& reason);
    void onDisconnectedFromLobby();
    void onGameStarted();
    void onProtocolStarted();
    void onBriefingBeginProtocol();
    void onBriefingAbort();

private:
    void setupUI();
    QWidget* createMainMenuPage();
    QWidget* createWaitingRoomPage();
    void applyTheme();

    QStackedWidget* m_stack = nullptr;

    // Main Menu Controls
    QLineEdit* m_callsignInput = nullptr;
    QSpinBox* m_maxCapacitySpinBox = nullptr;
    QLineEdit* m_joinTargetInput = nullptr;
    QListWidget* m_discoveredList = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Waiting Room Controls
    QLabel* m_roomTitleLabel = nullptr;
    QLabel* m_lobbyCodeLabel = nullptr;
    QPushButton* m_copyCodeBtn = nullptr;
    QLabel* m_rosterHeaderLabel = nullptr;
    QListWidget* m_rosterList = nullptr;
    QPushButton* m_launchMissionBtn = nullptr;
    QPushButton* m_leaveLobbyBtn = nullptr;
    QLabel* m_waitingStatusLabel = nullptr;

    // Briefing Screen
    MissionBriefingWidget* m_briefingWidget = nullptr;

    // Cinematic Storyboard
    StoryboardWidget* m_storyboardWidget = nullptr;

    // In-Game Viewport
    GameWidget* m_gameWidget = nullptr;
};

#endif // LOBBY_WIDGET_H

