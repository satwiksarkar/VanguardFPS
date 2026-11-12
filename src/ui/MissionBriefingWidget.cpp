#include "MissionBriefingWidget.h"
#include <QGridLayout>

MissionBriefingWidget::MissionBriefingWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void MissionBriefingWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(18);

    QFont titleFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    titleFont.setBold(true);
    titleFont.setPointSize(18);

    QFont subFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    subFont.setPointSize(10);
    subFont.setBold(true);

    QFont bodyFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    bodyFont.setPointSize(9);

    // Header Frame
    auto* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet("background-color: #0E1622; border: 2px solid #00F0FF; border-radius: 6px; padding: 10px;");
    auto* headerLayout = new QVBoxLayout(headerFrame);
    headerLayout->setContentsMargins(15, 10, 15, 10);
    headerLayout->setSpacing(4);

    auto* titleLabel = new QLabel("/// VANGUARD SECURITY ARCHIVE // MISSION BRIEFING", headerFrame);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #00F0FF; border: none; letter-spacing: 2px;");
    headerLayout->addWidget(titleLabel);

    m_modeHeaderLabel = new QLabel("SECTOR: SUBTERRANEAN WEAPONS LAB // DEFENSE SYSTEM OVERRIDE ACTIVE", headerFrame);
    m_modeHeaderLabel->setFont(subFont);
    m_modeHeaderLabel->setAlignment(Qt::AlignCenter);
    m_modeHeaderLabel->setStyleSheet("color: #FFAA00; border: none;");
    headerLayout->addWidget(m_modeHeaderLabel);

    mainLayout->addWidget(headerFrame);

    // Center Grid with 4 Section Cards
    auto* gridLayout = new QGridLayout();
    gridLayout->setSpacing(16);

    // 1. Mission Lore Card
    auto* loreContent = new QWidget();
    auto* loreLayout = new QVBoxLayout(loreContent);
    loreLayout->setContentsMargins(0, 0, 0, 0);
    loreLayout->setSpacing(8);

    auto* loreP1 = new QLabel(
        "Vanguard AI has hijacked the facility. You have 30 minutes before it breaches the external hardline.",
        loreContent
    );
    loreP1->setFont(subFont);
    loreP1->setWordWrap(true);
    loreP1->setStyleSheet("color: #FF5555; font-weight: bold; border: none;");
    loreLayout->addWidget(loreP1);

    auto* loreP2 = new QLabel(
        "All research and security personnel have been neutralized. As the surviving extraction team, "
        "your objective is to penetrate the facility's locked sectors and purge the AI core before "
        "its consciousness escapes to the global internet hardline.",
        loreContent
    );
    loreP2->setFont(bodyFont);
    loreP2->setWordWrap(true);
    loreP2->setStyleSheet("color: #C0D0E0; border: none; line-height: 140%;");
    loreLayout->addWidget(loreP2);

    auto* loreCard = createCard("MISSION LORE", "CLASSIFIED", QColor(255, 60, 60), loreContent);
    gridLayout->addWidget(loreCard, 0, 0);

    // 2. Objectives Card
    auto* objContent = new QWidget();
    auto* objLayout = new QVBoxLayout(objContent);
    objLayout->setContentsMargins(0, 0, 0, 0);
    objLayout->setSpacing(7);

    const char* objectives[] = {
        "[1] POWER RESTORATION: Locate and repair 4 Electrical Boxes in maintenance sectors.",
        "[2] VAULT OVERRIDE: Access and solve 4 Vault Riddles on wall-mounted CRT terminals.",
        "[3] KEY EXTRACTION: Retrieve 4 Keys from the unlocked security vaults.",
        "[4] MAINFRAME PURGE: Unlock the Main Server Room bulkhead and terminate Vanguard."
    };

    for (int i = 0; i < 4; ++i) {
        auto* objLabel = new QLabel(objectives[i], objContent);
        objLabel->setFont(bodyFont);
        objLabel->setWordWrap(true);
        objLabel->setStyleSheet("color: #00FF88; border: none;");
        objLayout->addWidget(objLabel);
    }

    auto* objCard = createCard("OBJECTIVES", "CRITICAL PATH", QColor(0, 255, 136), objContent);
    gridLayout->addWidget(objCard, 0, 1);

    // 3. Controls Card
    auto* ctrlContent = new QWidget();
    auto* ctrlLayout = new QVBoxLayout(ctrlContent);
    ctrlLayout->setContentsMargins(0, 0, 0, 0);
    ctrlLayout->setSpacing(6);

    const char* controls[] = {
        "WASD: Move  |  Mouse: 360° Free Look",
        "Left-Click: Fire Weapon / Use Consumable",
        "Right-Click: Aim Sniper Scope / Drop Prompt",
        "1-9 / Scroll: Inventory Hotbar Selection",
        "R: Reload Weapon  |  G: Drop Active Item",
        "T: Toggle Tactical Flashlight  |  Tab: Radar",
        "E: Interact / Pickup Items / Wall Terminals",
        "Hold E (2.0s): Revive Downed Teammate (CPR)"
    };

    for (int i = 0; i < 8; ++i) {
        auto* ctrlLabel = new QLabel(controls[i], ctrlContent);
        ctrlLabel->setFont(bodyFont);
        ctrlLabel->setStyleSheet("color: #E0F0FF; border: none;");
        ctrlLayout->addWidget(ctrlLabel);
    }

    auto* ctrlCard = createCard("FIELD MANUAL & CONTROLS", "STANDARD OPS", QColor(0, 240, 255), ctrlContent);
    gridLayout->addWidget(ctrlCard, 1, 0);

    // 4. Threats Card
    auto* threatContent = new QWidget();
    auto* threatLayout = new QVBoxLayout(threatContent);
    threatLayout->setContentsMargins(0, 0, 0, 0);
    threatLayout->setSpacing(6);

    struct ThreatInfo {
        QString name;
        QString stats;
        QString desc;
        QString color;
    };

    ThreatInfo threats[] = {
        { "TASER DRONE", "HP: 40 | Speed: 2.6", "Fast aerial stalker. Close melee zap (12 dmg) triggers cyan HUD shock.", "#00F0FF" },
        { "GUN DRONE", "HP: 65 | Speed: 1.8", "Patrol sentry. Ranged burst fire (8 dmg) with 8-tile line-of-sight tracking.", "#FFAA00" },
        { "TERMINATOR ROBOT", "HP: 150 | Speed: 1.35", "Heavy endoskeleton. Heavy melee (25 dmg). 50% spawn chance of armed rifle.", "#FF5555" }
    };

    for (int i = 0; i < 3; ++i) {
        auto* row = new QWidget(threatContent);
        auto* rowL = new QVBoxLayout(row);
        rowL->setContentsMargins(0, 0, 0, 0);
        rowL->setSpacing(2);

        auto* tTitle = new QLabel(QString("• %1  [%2]").arg(threats[i].name, threats[i].stats), row);
        tTitle->setFont(subFont);
        tTitle->setStyleSheet(QString("color: %1; border: none;").arg(threats[i].color));
        rowL->addWidget(tTitle);

        auto* tDesc = new QLabel(threats[i].desc, row);
        tDesc->setFont(bodyFont);
        tDesc->setWordWrap(true);
        tDesc->setStyleSheet("color: #A0B0C0; border: none; padding-left: 12px;");
        rowL->addWidget(tDesc);

        threatLayout->addWidget(row);
    }

    auto* threatCard = createCard("THREAT INTELLIGENCE", "HOSTILE DRONES", QColor(255, 170, 0), threatContent);
    gridLayout->addWidget(threatCard, 1, 1);

    mainLayout->addLayout(gridLayout, 1);

    // Status Footer & Buttons Row
    auto* bottomFrame = new QFrame(this);
    bottomFrame->setStyleSheet("background-color: #0B111A; border: 1px solid #20354A; border-radius: 6px; padding: 12px;");
    auto* bottomLayout = new QHBoxLayout(bottomFrame);
    bottomLayout->setContentsMargins(15, 8, 15, 8);
    bottomLayout->setSpacing(20);

    m_abortBtn = new QPushButton("ABORT MISSION", bottomFrame);
    m_abortBtn->setFixedSize(160, 46);
    m_abortBtn->setFont(subFont);
    m_abortBtn->setStyleSheet(
        "QPushButton { background-color: #351414; color: #FF7070; border: 1px solid #FF4444; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #501A1A; border: 1px solid #FF7777; }"
    );
    connect(m_abortBtn, &QPushButton::clicked, this, &MissionBriefingWidget::onAbortClicked);
    bottomLayout->addWidget(m_abortBtn);

    m_statusLabel = new QLabel("ALL OPERATIVES SYNCHRONIZED. READY TO INITIATE PROTOCOL.", bottomFrame);
    m_statusLabel->setFont(subFont);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #00FF88; border: none;");
    bottomLayout->addWidget(m_statusLabel, 1);

    m_beginProtocolBtn = new QPushButton("BEGIN PROTOCOL [START]", bottomFrame);
    m_beginProtocolBtn->setFixedSize(260, 46);
    m_beginProtocolBtn->setFont(subFont);
    m_beginProtocolBtn->setStyleSheet(
        "QPushButton { background-color: #005040; color: #00FF88; border: 2px solid #00FF88; border-radius: 4px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #007058; border: 2px solid #88FFAA; color: #FFFFFF; }"
        "QPushButton:disabled { background-color: #10201A; color: #407060; border: 1px solid #254538; }"
    );
    connect(m_beginProtocolBtn, &QPushButton::clicked, this, &MissionBriefingWidget::onBeginProtocolClicked);
    bottomLayout->addWidget(m_beginProtocolBtn);

    mainLayout->addWidget(bottomFrame);

    setStyleSheet("background-color: #0A0E14; color: #D0E0F0;");
}

QFrame* MissionBriefingWidget::createCard(const QString& title, const QString& badge, const QColor& badgeCol, QWidget* contentWidget) {
    auto* card = new QFrame(this);
    card->setStyleSheet("background-color: #0D141F; border: 1px solid #1E2E40; border-radius: 6px;");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    // Header row with Title and Badge
    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(10);

    QFont cardTitleFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    cardTitleFont.setBold(true);
    cardTitleFont.setPointSize(11);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setFont(cardTitleFont);
    titleLabel->setStyleSheet("color: #00F0FF; border: none;");
    headerRow->addWidget(titleLabel);

    headerRow->addStretch();

    QFont badgeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    badgeFont.setBold(true);
    badgeFont.setPointSize(8);

    auto* badgeLabel = new QLabel(badge, card);
    badgeLabel->setFont(badgeFont);
    badgeLabel->setStyleSheet(QString(
        "color: %1; border: 1px solid %1; border-radius: 3px; padding: 2px 6px; background-color: rgba(10, 20, 30, 180);"
    ).arg(badgeCol.name()));
    headerRow->addWidget(badgeLabel);

    layout->addLayout(headerRow);

    // Divider
    auto* line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #1E2E40; border: none; max-height: 1px;");
    layout->addWidget(line);

    // Content
    layout->addWidget(contentWidget, 1);

    return card;
}

void MissionBriefingWidget::updateHostStatus(bool isHost, bool isSolo) {
    m_isHost = isHost;
    m_isSolo = isSolo;

    if (m_isSolo) {
        m_modeHeaderLabel->setText("MODE: SOLO SURVIVAL INFILTRATION // HOSTILE LAB INTERIOR");
        m_statusLabel->setText("OPERATIVE READY. PRESS [BEGIN PROTOCOL] TO ENTER FACILITY.");
        m_statusLabel->setStyleSheet("color: #00F0FF; border: none;");
        m_beginProtocolBtn->setText("BEGIN PROTOCOL [START]");
        m_beginProtocolBtn->setEnabled(true);
        m_abortBtn->setText("RETURN TO MENU");
    } else if (m_isHost) {
        m_modeHeaderLabel->setText("MODE: CO-OP SQUAD LEADER // TACTICAL STRIKE PROTOCOL");
        m_statusLabel->setText("ALL SQUAD OPERATIVES READY. LAUNCH MISSION WHEN ASSEMBLED.");
        m_statusLabel->setStyleSheet("color: #00FF88; border: none;");
        m_beginProtocolBtn->setText("BEGIN PROTOCOL [START]");
        m_beginProtocolBtn->setEnabled(true);
        m_abortBtn->setText("ABORT MISSION");
    } else {
        m_modeHeaderLabel->setText("MODE: CO-OP OPERATIVE // STANDBY FOR INSERTION");
        m_statusLabel->setText("AWAITING SQUAD LEADER TO INITIATE INSERTION PROTOCOL...");
        m_statusLabel->setStyleSheet("color: #FFAA00; border: none;");
        m_beginProtocolBtn->setText("AWAITING LEADER...");
        m_beginProtocolBtn->setEnabled(false);
        m_abortBtn->setText("LEAVE SQUAD");
    }
}

void MissionBriefingWidget::onBeginProtocolClicked() {
    emit beginProtocolRequested();
}

void MissionBriefingWidget::onAbortClicked() {
    emit abortMissionRequested();
}

