#include "LobbyWidget.h"
#include "MissionBriefingWidget.h"
#include "StoryboardWidget.h"
#include "game/GameWidget.h"
#include "net/NetworkManager.h"
#include "audio/AudioManager.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QGroupBox>
#include <QFrame>

LobbyWidget::LobbyWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("VanguardFPS - Underground Weapons Facility [Phase 5: Co-op Multiplayer]");
    resize(1280, 720);
    setMinimumSize(960, 540);

    setupUI();
    applyTheme();

    // Hook NetworkManager events
    NetworkManager& net = NetworkManager::instance();
    connect(&net, &NetworkManager::discoveredLobbiesChanged, this, &LobbyWidget::onDiscoveredLobbiesChanged);
    connect(&net, &NetworkManager::lobbyCreated, this, &LobbyWidget::onLobbyCreated);
    connect(&net, &NetworkManager::joinedLobby, this, &LobbyWidget::onJoinedLobby);
    connect(&net, &NetworkManager::lobbyUpdated, this, &LobbyWidget::onLobbyUpdated);
    connect(&net, &NetworkManager::connectionFailed, this, &LobbyWidget::onConnectionFailed);
    connect(&net, &NetworkManager::disconnectedFromLobby, this, &LobbyWidget::onDisconnectedFromLobby);
    connect(&net, &NetworkManager::gameStarted, this, &LobbyWidget::onGameStarted);
    connect(&net, &NetworkManager::protocolStarted, this, &LobbyWidget::onProtocolStarted);

    // Hook MissionBriefingWidget events
    connect(m_briefingWidget, &MissionBriefingWidget::beginProtocolRequested, this, &LobbyWidget::onBriefingBeginProtocol);
    connect(m_briefingWidget, &MissionBriefingWidget::abortMissionRequested, this, &LobbyWidget::onBriefingAbort);

    // Hook GameWidget return signal
    connect(m_gameWidget, &GameWidget::exitToLobbyRequested, this, [this]() {
        if (!NetworkManager::instance().isOffline()) {
            m_stack->setCurrentIndex(1); // Return to briefing room
        } else {
            m_stack->setCurrentIndex(0); // Return to main menu
        }
    });

    onDiscoveredLobbiesChanged();
}

void LobbyWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);

    // Page 0: Main Menu
    m_stack->addWidget(createMainMenuPage());

    // Page 1: Waiting Room
    m_stack->addWidget(createWaitingRoomPage());

    // Page 2: Mission Briefing
    m_briefingWidget = new MissionBriefingWidget(this);
    m_stack->addWidget(m_briefingWidget);

    // Page 3: In-Game Viewport
    m_gameWidget = new GameWidget(this);
    m_stack->addWidget(m_gameWidget);

    // Page 4: Cinematic Storyboard
    m_storyboardWidget = new StoryboardWidget(this);
    m_stack->addWidget(m_storyboardWidget);

    connect(m_storyboardWidget, &StoryboardWidget::storyFinished, this, [this]() {
        m_stack->setCurrentIndex(0); // Return to Main Menu after intro
    });

    mainLayout->addWidget(m_stack);
    // Boot directly into the cinematic storyboard
    m_stack->setCurrentIndex(4);
}

QWidget* LobbyWidget::createMainMenuPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);
    layout->setSpacing(20);

    // Header Title
    QFont titleFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    titleFont.setBold(true);
    titleFont.setPointSize(22);

    auto* titleLabel = new QLabel("VANGUARD : FACILITY BREACH", page);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #00F0FF; letter-spacing: 2px;");
    layout->addWidget(titleLabel);

    QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    subFont.setPointSize(10);
    auto* subLabel = new QLabel("UNDERGROUND WEAPONS RESEARCH LAB // TACTICAL CO-OP TERMINAL", page);
    subLabel->setFont(subFont);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setStyleSheet("color: #7090A8;");
    layout->addWidget(subLabel);

    layout->addSpacing(10);

    // Profile Box
    auto* profileBox = new QGroupBox("OPERATIVE IDENTIFICATION", page);
    auto* profileLayout = new QHBoxLayout(profileBox);

    auto* callsignLabel = new QLabel("OPERATIVE CALLSIGN:", profileBox);
    callsignLabel->setFont(subFont);
    callsignLabel->setStyleSheet("color: #D0E0F0;");

    m_callsignInput = new QLineEdit("Agent_Miller", profileBox);
    m_callsignInput->setFont(subFont);
    m_callsignInput->setMaxLength(16);

    profileLayout->addWidget(callsignLabel);
    profileLayout->addWidget(m_callsignInput, 1);
    layout->addWidget(profileBox);

    // Center 2-Column Grid: Single Player / Host vs Join / LAN Lobbies
    auto* centerLayout = new QHBoxLayout();
    centerLayout->setSpacing(25);

    // Left Column: Solo & Host
    auto* leftCol = new QVBoxLayout();

    // Solo Mode Box
    auto* soloBox = new QGroupBox("SINGLE-PLAYER MISSION", page);
    auto* soloLayout = new QVBoxLayout(soloBox);

    auto* soloDesc = new QLabel("Infiltrate the facility solo. 30:00 to retrieve 4 Vault Keys and disable Vanguard before hardline uplink.", soloBox);
    soloDesc->setWordWrap(true);
    soloDesc->setStyleSheet("color: #8A9EAF; font-size: 11px;");

    auto* soloBtn = new QPushButton("SOLO BREACH [OFFLINE]", soloBox);
    soloBtn->setFixedHeight(42);
    connect(soloBtn, &QPushButton::clicked, this, &LobbyWidget::onSoloClicked);

    soloLayout->addWidget(soloDesc);
    soloLayout->addSpacing(10);
    soloLayout->addWidget(soloBtn);
    leftCol->addWidget(soloBox);

    // Host Mode Box
    auto* hostBox = new QGroupBox("HOST CO-OP LOBBY", page);
    auto* hostLayout = new QVBoxLayout(hostBox);

    auto* capRow = new QHBoxLayout();
    auto* capLabel = new QLabel("MAX SQUAD OPERATIVES (2 - 4):", hostBox);
    capLabel->setStyleSheet("color: #D0E0F0; font-size: 11px;");
    m_maxCapacitySpinBox = new QSpinBox(hostBox);
    m_maxCapacitySpinBox->setRange(2, 4);
    m_maxCapacitySpinBox->setValue(4);
    m_maxCapacitySpinBox->setFixedWidth(60);
    capRow->addWidget(capLabel);
    capRow->addWidget(m_maxCapacitySpinBox);

    auto* hostBtn = new QPushButton("HOST CO-OP SQUAD LOBBY", hostBox);
    hostBtn->setFixedHeight(42);
    connect(hostBtn, &QPushButton::clicked, this, &LobbyWidget::onHostClicked);

    hostLayout->addLayout(capRow);
    hostLayout->addSpacing(10);
    hostLayout->addWidget(hostBtn);
    leftCol->addWidget(hostBox);

    centerLayout->addLayout(leftCol, 1);

    // Right Column: Join by Code & Discovered LAN Lobbies
    auto* rightCol = new QVBoxLayout();

    // Join Box
    auto* joinBox = new QGroupBox("JOIN SQUAD LOBBY", page);
    auto* joinLayout = new QVBoxLayout(joinBox);

    auto* joinInputRow = new QHBoxLayout();
    m_joinTargetInput = new QLineEdit(joinBox);
    m_joinTargetInput->setPlaceholderText("Enter 6-char code (e.g. VNG42X) or IP:CODE");
    m_joinTargetInput->setFont(subFont);
    m_joinTargetInput->setFixedHeight(36);

    auto* joinBtn = new QPushButton("JOIN", joinBox);
    joinBtn->setFixedSize(80, 36);
    connect(joinBtn, &QPushButton::clicked, this, &LobbyWidget::onJoinClicked);

    joinInputRow->addWidget(m_joinTargetInput);
    joinInputRow->addWidget(joinBtn);
    joinLayout->addLayout(joinInputRow);

    // Discovered Lobbies
    auto* lanLabel = new QLabel("DISCOVERED LAN & LOCALHOST LOBBIES (UDP 45455):", joinBox);
    lanLabel->setStyleSheet("color: #00F0FF; font-size: 11px; margin-top: 10px;");
    joinLayout->addWidget(lanLabel);

    m_discoveredList = new QListWidget(joinBox);
    m_discoveredList->setFont(subFont);
    connect(m_discoveredList, &QListWidget::itemDoubleClicked, this, &LobbyWidget::onQuickJoinLobbyItem);
    joinLayout->addWidget(m_discoveredList);

    auto* quickJoinBtn = new QPushButton("QUICK JOIN SELECTED LOBBY", joinBox);
    quickJoinBtn->setFixedHeight(34);
    connect(quickJoinBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem* item = m_discoveredList->currentItem();
        if (item) {
            onQuickJoinLobbyItem(item);
        } else {
            m_statusLabel->setText("Please select a discovered lobby from the list above.");
        }
    });
    joinLayout->addWidget(quickJoinBtn);

    rightCol->addWidget(joinBox);
    centerLayout->addLayout(rightCol, 1);

    layout->addLayout(centerLayout);

    // Status Footer
    m_statusLabel = new QLabel("Ready. Select an infiltration mode.", page);
    m_statusLabel->setFont(subFont);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #FFB020; min-height: 24px;");
    layout->addWidget(m_statusLabel);

    // Replay Storyboard Button
    auto* bottomBtnRow = new QHBoxLayout();
    bottomBtnRow->addStretch(1);
    auto* replayStoryBtn = new QPushButton("REPLAY CINEMATIC STORYBOARD", page);
    replayStoryBtn->setFixedHeight(32);
    replayStoryBtn->setStyleSheet("background-color: #162432; color: #00F0FF; border: 1px solid #00F0FF; padding: 0 16px; font-weight: bold;");
    connect(replayStoryBtn, &QPushButton::clicked, this, [this]() {
        m_storyboardWidget->resetStory();
        m_stack->setCurrentWidget(m_storyboardWidget);
    });
    bottomBtnRow->addWidget(replayStoryBtn);
    bottomBtnRow->addStretch(1);
    layout->addLayout(bottomBtnRow);

    return page;
}

QWidget* LobbyWidget::createWaitingRoomPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(50, 40, 50, 40);
    layout->setSpacing(25);

    // Header
    QFont titleFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    titleFont.setBold(true);
    titleFont.setPointSize(20);

    m_roomTitleLabel = new QLabel("VANGUARD CO-OP BRIEFING ROOM", page);
    m_roomTitleLabel->setFont(titleFont);
    m_roomTitleLabel->setAlignment(Qt::AlignCenter);
    m_roomTitleLabel->setStyleSheet("color: #00F0FF;");
    layout->addWidget(m_roomTitleLabel);

    // Big Code Display Box
    auto* codeBox = new QFrame(page);
    codeBox->setFrameShape(QFrame::StyledPanel);
    codeBox->setStyleSheet("background-color: #0D141E; border: 2px solid #00F0FF; border-radius: 8px; padding: 15px;");
    auto* codeLayout = new QHBoxLayout(codeBox);

    QFont codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    codeFont.setBold(true);
    codeFont.setPointSize(28);

    m_lobbyCodeLabel = new QLabel("LOBBY CODE: [ ------ ]", codeBox);
    m_lobbyCodeLabel->setFont(codeFont);
    m_lobbyCodeLabel->setAlignment(Qt::AlignCenter);
    m_lobbyCodeLabel->setStyleSheet("color: #00FF88; border: none; letter-spacing: 3px;");

    m_copyCodeBtn = new QPushButton("COPY CODE", codeBox);
    m_copyCodeBtn->setFixedSize(130, 44);
    m_copyCodeBtn->setStyleSheet("background-color: #1A2838; color: #00F0FF; border: 1px solid #00F0FF; font-weight: bold; border-radius: 4px;");
    connect(m_copyCodeBtn, &QPushButton::clicked, this, &LobbyWidget::onCopyCodeClicked);

    codeLayout->addWidget(m_lobbyCodeLabel, 1);
    codeLayout->addWidget(m_copyCodeBtn);
    layout->addWidget(codeBox);

    // Squad Roster
    QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    subFont.setBold(true);
    subFont.setPointSize(11);

    m_rosterHeaderLabel = new QLabel("SURVIVORS ASSEMBLED ( 1 / 4 ):", page);
    m_rosterHeaderLabel->setFont(subFont);
    m_rosterHeaderLabel->setStyleSheet("color: #D0E0F0;");
    layout->addWidget(m_rosterHeaderLabel);

    m_rosterList = new QListWidget(page);
    m_rosterList->setFont(subFont);
    m_rosterList->setStyleSheet("background-color: #0E1622; border: 1px solid #304860; color: #E0F0FF; padding: 10px; font-size: 14px;");
    layout->addWidget(m_rosterList, 1);

    // Waiting Status Label
    m_waitingStatusLabel = new QLabel("Awaiting operatives to join squad...", page);
    m_waitingStatusLabel->setFont(subFont);
    m_waitingStatusLabel->setAlignment(Qt::AlignCenter);
    m_waitingStatusLabel->setStyleSheet("color: #FFAA00;");
    layout->addWidget(m_waitingStatusLabel);

    // Action Buttons Row
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(20);

    m_leaveLobbyBtn = new QPushButton("LEAVE LOBBY", page);
    m_leaveLobbyBtn->setFixedHeight(46);
    m_leaveLobbyBtn->setStyleSheet("background-color: #3A1818; color: #FF7070; border: 1px solid #FF4040; border-radius: 4px; font-weight: bold;");
    connect(m_leaveLobbyBtn, &QPushButton::clicked, this, &LobbyWidget::onLeaveLobbyClicked);

    m_launchMissionBtn = new QPushButton("LAUNCH MISSION", page);
    m_launchMissionBtn->setFixedHeight(46);
    m_launchMissionBtn->setStyleSheet("background-color: #005040; color: #00FF88; border: 2px solid #00FF88; border-radius: 4px; font-size: 14px; font-weight: bold;");
    connect(m_launchMissionBtn, &QPushButton::clicked, this, &LobbyWidget::onLaunchMissionClicked);

    btnRow->addWidget(m_leaveLobbyBtn, 1);
    btnRow->addWidget(m_launchMissionBtn, 2);
    layout->addLayout(btnRow);

    return page;
}

void LobbyWidget::applyTheme() {
    setStyleSheet(R"(
        QWidget {
            background-color: #0A0E14;
            color: #C0D0E0;
        }
        QGroupBox {
            border: 1px solid #203448;
            border-radius: 6px;
            margin-top: 14px;
            padding-top: 18px;
            font-weight: bold;
            color: #00F0FF;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 14px;
            padding: 0 4px;
            color: #00F0FF;
        }
        QLineEdit {
            background-color: #0E1620;
            border: 1px solid #284058;
            border-radius: 4px;
            padding: 6px 10px;
            color: #00F0FF;
            selection-background-color: #0080A0;
        }
        QLineEdit:focus {
            border: 1px solid #00F0FF;
        }
        QPushButton {
            background-color: #122030;
            color: #D0E8F8;
            border: 1px solid #305070;
            border-radius: 4px;
            padding: 6px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1A3048;
            border: 1px solid #00F0FF;
            color: #00F0FF;
        }
        QPushButton:pressed {
            background-color: #006080;
            color: #FFFFFF;
        }
        QListWidget {
            background-color: #0C121A;
            border: 1px solid #24384C;
            border-radius: 4px;
            color: #D0E0F0;
        }
        QListWidget::item {
            padding: 8px 10px;
            border-bottom: 1px solid #182432;
        }
        QListWidget::item:selected {
            background-color: #143048;
            color: #00F0FF;
            border-left: 3px solid #00F0FF;
        }
        QSpinBox {
            background-color: #0E1620;
            border: 1px solid #284058;
            border-radius: 4px;
            color: #00F0FF;
            padding: 4px;
            font-weight: bold;
        }
    )");
}

void LobbyWidget::onSoloClicked() {
    NetworkManager::instance().disconnectLobby();
    m_briefingWidget->updateHostStatus(true, true);
    m_stack->setCurrentIndex(2); // Mission Briefing Screen
}

void LobbyWidget::onHostClicked() {
    QString name = m_callsignInput->text().trimmed();
    if (name.isEmpty()) name = "Operative_Alpha";

    quint8 cap = static_cast<quint8>(m_maxCapacitySpinBox->value());
    if (NetworkManager::instance().hostLobby(name, cap)) {
        m_stack->setCurrentIndex(1); // Waiting Room
    }
}

void LobbyWidget::onJoinClicked() {
    QString target = m_joinTargetInput->text().trimmed();
    if (target.isEmpty()) {
        m_statusLabel->setText("Please enter a 6-character Lobby Code (e.g. VNG42X) or IP:CODE.");
        return;
    }

    QString name = m_callsignInput->text().trimmed();
    if (name.isEmpty()) name = "Operative_Bravo";

    m_statusLabel->setText("Connecting to host [" + target + "]...");
    NetworkManager::instance().joinLobby(target, name);
}

void LobbyWidget::onQuickJoinLobbyItem(QListWidgetItem *item) {
    if (!item) return;

    QString code = item->data(Qt::UserRole).toString();
    if (code.isEmpty()) return;

    QString name = m_callsignInput->text().trimmed();
    if (name.isEmpty()) name = "Operative_Echo";

    m_statusLabel->setText("Joining lobby " + code + "...");
    NetworkManager::instance().joinLobby(code, name);
}

void LobbyWidget::onCopyCodeClicked() {
    QString code = NetworkManager::instance().getLobbyCode();
    if (!code.isEmpty()) {
        QClipboard* cb = QGuiApplication::clipboard();
        cb->setText(code);
        m_copyCodeBtn->setText("COPIED!");
        QTimer::singleShot(1500, this, [this]() {
            m_copyCodeBtn->setText("COPY CODE");
        });
    }
}

void LobbyWidget::onLaunchMissionClicked() {
    if (NetworkManager::instance().isHost()) {
        NetworkManager::instance().launchGame();
    }
}

void LobbyWidget::onLeaveLobbyClicked() {
    NetworkManager::instance().disconnectLobby();
    m_stack->setCurrentIndex(0);
    m_statusLabel->setText("Returned to Main Menu.");
}

void LobbyWidget::onDiscoveredLobbiesChanged() {
    m_discoveredList->clear();
    const auto& lobbies = NetworkManager::instance().getDiscoveredLobbies();

    if (lobbies.empty()) {
        auto* emptyItem = new QListWidgetItem("No active lobbies detected on LAN (Listening on port 45455)...", m_discoveredList);
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setForeground(QColor(100, 120, 140));
        return;
    }

    for (const auto& l : lobbies) {
        QString status = l.beacon.inGame ? "[IN PROGRESS]" : "[JOINABLE]";
        QString line = QString("%1 | Code: %2 | Players: %3/%4 | Host: %5 %6")
                           .arg(status)
                           .arg(l.beacon.lobbyCode)
                           .arg(l.beacon.currentPlayers)
                           .arg(l.beacon.maxPlayers)
                           .arg(l.beacon.hostName)
                           .arg(l.address.toString());

        auto* item = new QListWidgetItem(line, m_discoveredList);
        item->setData(Qt::UserRole, l.beacon.lobbyCode);
        if (l.beacon.inGame || l.beacon.currentPlayers >= l.beacon.maxPlayers) {
            item->setForeground(QColor(140, 150, 160));
        } else {
            item->setForeground(QColor(0, 240, 255));
        }
    }
}

void LobbyWidget::onLobbyCreated(const QString& lobbyCode, quint8 maxCapacity) {
    m_lobbyCodeLabel->setText(QString("LOBBY CODE: [ %1 ]").arg(lobbyCode));
    m_rosterHeaderLabel->setText(QString("SURVIVORS ASSEMBLED ( 1 / %1 ):").arg(maxCapacity));
    m_launchMissionBtn->setVisible(true);
    m_launchMissionBtn->setEnabled(true);
    m_waitingStatusLabel->setText("You are the Squad Leader. Press LAUNCH MISSION when ready.");
    m_stack->setCurrentIndex(1);
}

void LobbyWidget::onJoinedLobby(const QString& lobbyCode, quint8 assignedId, quint8 maxCapacity) {
    Q_UNUSED(assignedId);
    m_lobbyCodeLabel->setText(QString("LOBBY CODE: [ %1 ]").arg(lobbyCode));
    m_rosterHeaderLabel->setText(QString("SURVIVORS ASSEMBLED ( -- / %1 ):").arg(maxCapacity));
    m_launchMissionBtn->setVisible(true);
    m_launchMissionBtn->setEnabled(false);
    m_waitingStatusLabel->setText("Squad joined. Awaiting Squad Leader to launch mission...");
    m_stack->setCurrentIndex(1);
}

void LobbyWidget::onLobbyUpdated(const QStringList& playerNames) {
    m_rosterList->clear();
    quint8 maxCap = NetworkManager::instance().getMaxCapacity();
    m_rosterHeaderLabel->setText(QString("SURVIVORS ASSEMBLED ( %1 / %2 ):").arg(playerNames.size()).arg(maxCap));

    for (int i = 0; i < playerNames.size(); ++i) {
        QString role = (i == 0) ? "[SQUAD LEADER]" : "[OPERATIVE]";
        QString entry = QString("%1  Operative #%2: %3  -  READY").arg(role).arg(i + 1).arg(playerNames[i]);
        auto* item = new QListWidgetItem(entry, m_rosterList);
        if (i == 0) {
            item->setForeground(QColor(0, 255, 140));
        } else {
            item->setForeground(QColor(0, 220, 255));
        }
    }

    if (!NetworkManager::instance().isHost()) {
        m_waitingStatusLabel->setText("Squad joined. Awaiting Squad Leader to launch mission...");
    }
}

void LobbyWidget::onConnectionFailed(const QString& reason) {
    m_statusLabel->setText("Failed: " + reason);
    m_stack->setCurrentIndex(0);
}

void LobbyWidget::onDisconnectedFromLobby() {
    m_statusLabel->setText("Disconnected from lobby.");
    m_stack->setCurrentIndex(0);
}

void LobbyWidget::onGameStarted() {
    bool isHost = NetworkManager::instance().isHost();
    m_briefingWidget->updateHostStatus(isHost, false);
    m_stack->setCurrentIndex(2); // Mission Briefing Screen
}

void LobbyWidget::onBriefingBeginProtocol() {
    if (NetworkManager::instance().isOffline()) {
        m_stack->setCurrentIndex(3); // In-Game Viewport
        m_gameWidget->startMission();
    } else if (NetworkManager::instance().isHost()) {
        NetworkManager::instance().startProtocol();
    }
}

void LobbyWidget::onProtocolStarted() {
    m_stack->setCurrentIndex(3); // In-Game Viewport
    m_gameWidget->startMission();
}

void LobbyWidget::onBriefingAbort() {
    AudioManager::instance().pauseAmbience();
    if (!NetworkManager::instance().isOffline()) {
        NetworkManager::instance().disconnectLobby();
    }
    m_stack->setCurrentIndex(0);
    m_statusLabel->setText("Mission aborted. Returned to Main Menu.");
}

