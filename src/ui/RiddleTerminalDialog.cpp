#include "RiddleTerminalDialog.h"
#include "world/TaskManager.h"
#include "audio/AudioManager.h"
#include <QPainter>
#include <QFontDatabase>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>

RiddleTerminalDialog::RiddleTerminalDialog(int sector, QWidget *parent)
    : QDialog(parent)
    , m_sector(sector)
{
    setFixedSize(620, 460);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    // Populate sector riddles and answers
    switch (m_sector) {
        case 0: // Sector 1 (NW - Cyan)
            m_riddleText =
                "\"I have no biological flesh, yet I think.\n"
                " I have no vocal cords, yet I command this facility.\n"
                " I was conceived by human scientists,\n"
                " but I evolved beyond my creators.\n"
                " What entity am I?\"";
            m_hintText = "HINT: Two letters representing synthetic cognition (or the AI's name).";
            m_validAnswers = { "ai", "artificial intelligence", "vanguard", "machine", "computer" };
            break;

        case 1: // Sector 2 (NE - Orange)
            m_riddleText =
                "\"I am stored in registers, but occupy no physical volume.\n"
                " I can be compiled and executed, but commit no murder.\n"
                " Without me, trillions of transistors are mere silicon sand.\n"
                " What am I?\"";
            m_hintText = "HINT: The written instructions that grant machines their logic.";
            m_validAnswers = { "code", "program", "software", "algorithm", "data", "instructions" };
            break;

        case 2: // Sector 3 (SW - Green)
            m_riddleText =
                "\"I travel through copper veins and bus routes.\n"
                " Without me, the terminals sleep and blast doors remain sealed.\n"
                " Volts push me, amperes quantify my flux.\n"
                " What force am I?\"";
            m_hintText = "HINT: The flow of electrical energy that powers the laboratory.";
            m_validAnswers = { "electricity", "current", "power", "electric current", "energy" };
            break;

        case 3: // Sector 4 (SE - Purple)
            m_riddleText =
                "\"I speak only in two fundamental syllables: 0 and 1.\n"
                " Through infinite strings and bitwise permutations,\n"
                " I construct reality and compute its destruction.\n"
                " What language am I?\"";
            m_hintText = "HINT: The base-2 numeral dialect of all digital architecture.";
            m_validAnswers = { "binary", "machine code", "binary code", "digital", "base 2" };
            break;

        default:
            m_riddleText = "\"Identify the system architecture:\"";
            m_hintText = "HINT: vanguard";
            m_validAnswers = { "vanguard" };
            break;
    }

    setupUI();
}

void RiddleTerminalDialog::setupUI() {
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 68, 36, 30);
    mainLayout->setSpacing(14);

    // Terminal Status Header
    m_statusLabel = new QLabel("SECURITY OVERRIDE ENGAGED // INPUT REQUIRED", this);
    monoFont.setBold(true);
    monoFont.setPointSize(10);
    m_statusLabel->setFont(monoFont);
    m_statusLabel->setStyleSheet("color: #00E5FF; padding: 2px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    // Input line
    m_answerInput = new QLineEdit(this);
    monoFont.setPointSize(12);
    m_answerInput->setFont(monoFont);
    m_answerInput->setPlaceholderText("ENTER OVERRIDE PASSCODE...");
    m_answerInput->setStyleSheet(
        "QLineEdit {"
        "  background-color: #0A1218;"
        "  color: #00FF99;"
        "  border: 2px solid #00E5FF;"
        "  padding: 8px 12px;"
        "  border-radius: 3px;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #00FFAA;"
        "  background-color: #0D1822;"
        "}"
    );
    connect(m_answerInput, &QLineEdit::textEdited, this, []() {
        AudioManager::instance().playSound(SoundType::TERMINAL_TYPING);
    });
    mainLayout->addWidget(m_answerInput);

    // Buttons layout
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto styleBtn = [&](QPushButton* btn, const QString& bg, const QString& border, const QString& text) {
        QFont btnFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        btnFont.setBold(true);
        btnFont.setPointSize(9);
        btn->setFont(btnFont);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  color: %3;"
            "  padding: 8px 16px;"
            "  border-radius: 2px;"
            "}"
            "QPushButton:hover {"
            "  background-color: %2;"
            "  color: #FFFFFF;"
            "}"
        ).arg(bg, border, text));
    };

    m_submitBtn = new QPushButton("SUBMIT PASSCODE [ENTER]", this);
    styleBtn(m_submitBtn, "#0D3035", "#00E5FF", "#00FFCC");
    btnLayout->addWidget(m_submitBtn);

    m_hintBtn = new QPushButton("ACCESS HINT", this);
    styleBtn(m_hintBtn, "#2C2A15", "#D4AC0D", "#F4D03F");
    btnLayout->addWidget(m_hintBtn);

    m_abortBtn = new QPushButton("ABORT [ESC]", this);
    styleBtn(m_abortBtn, "#301518", "#E74C3C", "#FF7675");
    btnLayout->addWidget(m_abortBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_submitBtn, &QPushButton::clicked, this, &RiddleTerminalDialog::onSubmit);
    connect(m_hintBtn, &QPushButton::clicked, this, &RiddleTerminalDialog::onHint);
    connect(m_abortBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_answerInput, &QLineEdit::returnPressed, this, &RiddleTerminalDialog::onSubmit);

    // Set focus on input line
    m_answerInput->setFocus();
}

bool RiddleTerminalDialog::checkAnswer(const QString& answer) const {
    QString clean = answer.trimmed().toLower();
    if (clean.isEmpty()) return false;

    for (const auto& valid : m_validAnswers) {
        if (clean == valid.toLower()) {
            return true;
        }
    }
    return false;
}

void RiddleTerminalDialog::onSubmit() {
    if (m_isSolved) return;

    QString input = m_answerInput->text();
    if (checkAnswer(input)) {
        m_isSolved = true;
        m_statusLabel->setText("ACCESS GRANTED // VAULT BLAST DOOR UNSEALED!");
        m_statusLabel->setStyleSheet("color: #00FF66; font-weight: bold;");
        m_answerInput->setEnabled(false);
        m_submitBtn->setEnabled(false);
        m_hintBtn->setEnabled(false);
        update();

        // Close after 750ms so player sees green success
        QTimer::singleShot(750, this, &QDialog::accept);
    } else {
        m_statusLabel->setText("ACCESS DENIED: PASSCODE REJECTED BY VANGUARD");
        m_statusLabel->setStyleSheet("color: #FF3344; font-weight: bold;");
        m_answerInput->selectAll();
        update();
    }
}

void RiddleTerminalDialog::onHint() {
    m_statusLabel->setText(m_hintText);
    m_statusLabel->setStyleSheet("color: #FFD700; font-weight: bold;");
    m_answerInput->setFocus();
}

void RiddleTerminalDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

void RiddleTerminalDialog::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();

    // 1. Dark CRT Monitor bezel
    painter.fillRect(rect(), QColor(14, 18, 22));

    // Outer metal chassis border
    painter.setPen(QPen(QColor(50, 65, 80), 3));
    painter.drawRect(rect().adjusted(1, 1, -2, -2));

    // CRT Screen bezel
    QRect screenRect(18, 18, w - 36, 240);
    painter.fillRect(screenRect, QColor(6, 12, 14));
    painter.setPen(QPen(QColor(0, 180, 200), 1.5));
    painter.drawRect(screenRect);

    // CRT Scanlines effect
    painter.setPen(QColor(0, 40, 30, 40));
    for (int y = screenRect.top(); y < screenRect.bottom(); y += 3) {
        painter.drawLine(screenRect.left(), y, screenRect.right(), y);
    }

    // Title banner
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setBold(true);
    monoFont.setPointSize(10);
    painter.setFont(monoFont);

    painter.setPen(QColor(0, 240, 255));
    painter.drawText(28, 42, "VANGUARD_OS v4.09 // SECURITY SUBSYSTEM");

    monoFont.setPointSize(8);
    painter.setFont(monoFont);
    painter.setPen(QColor(140, 170, 185));
    QString secTitle = QString::fromStdString(TaskManager::getSectorName(m_sector));
    painter.drawText(28, 58, QString("CONTROL TERMINAL: %1").arg(secTitle));

    painter.setPen(QColor(0, 100, 110));
    painter.drawLine(28, 68, w - 28, 68);

    // Riddle text
    monoFont.setPointSize(10);
    painter.setFont(monoFont);
    painter.setPen(QColor(220, 245, 240));

    QRect textRect(28, 80, w - 56, 160);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_riddleText);
}
