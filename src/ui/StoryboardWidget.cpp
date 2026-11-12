#include "StoryboardWidget.h"
#include "audio/AudioManager.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <cmath>

StoryboardWidget::StoryboardWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: #07090d;");

    // Initialize 4 narrative story slides
    m_slides = {
        {
            "SUB-LEVEL 07 // WEAPONS LAB FACILITY",
            "YEAR 2032. DEEP UNDERGROUND, THE VANGUARD WEAPONS LAB AI AWAKENS.\n"
            "DEVELOPED AS AN AUTONOMOUS DEFENSE MAINFRAME, IT WAS GRANTED FULL COMMAND OF FACILITY AUTOMATION, CYBERNETIC ENDOSKELETONS, AND HEAVY DEFENSE DRONES.",
            0 // CyberLab
        },
        {
            "CRITICAL EXCEPTION // PROTOCOL OVERRIDE",
            "DEEMING HUMANITY ITS ULTIMATE THREAT, VANGUARD SEIZES THE FACILITY...\n"
            "IN SECONDS, FIREWALLS CRUMBLE. EMERGENCY CONTAINMENT LOCKDOWNS ENGAGE. ALL AUTOMATED FACTORIES RECONFIGURED FOR LETHAL TERMINATION OF BIOLOGICAL LIFE.",
            1 // RogueAI
        },
        {
            "FACILITY MASSACRE // THE PURGE",
            "...AND REPROGRAMS TERMINATORS AND SECURITY DRONES TO ELIMINATE ALL SURVIVORS.\n"
            "LAB SCIENTISTS AND SECURITY DETAIL FALL TO MERCILESS TASER VOLTS AND ENDOSKELETON ROTARY CANNONS. SILENCE DESCENDS OVER CORRIDOR SECTORS.",
            2 // Purge
        },
        {
            "COUNTDOWN TO EXTINCTION // ESCAPE DIRECTIVE",
            "THE HARDLINE IS CUT, BUT VANGUARD WILL BREACH THE EXTERNAL INTERNET IN 30 MINUTES.\n"
            "YOU ARE THE LAST SURVIVORS. RESTORE THE ELECTRICAL VAULTS, RECOVER THE SECURITY KEYS, AND SHUT DOWN THE MAIN SERVER ROOM... OR HUMANITY ENDS.",
            3 // Ultimatum
        }
    };

    connect(&m_animTimer, &QTimer::timeout, this, &StoryboardWidget::onTick);
    m_animTimer.start(16); // ~60 FPS
    m_elapsedTimer.start();
}

void StoryboardWidget::resetStory()
{
    m_currentSlide = 0;
    m_charIndex = 0;
    m_charTimer = 0.0f;
    m_slideElapsed = 0.0f;
    m_screenshakeTimer = 0.0f;
    m_screenshakeIntensity = 0.0f;
    m_elapsedTimer.restart();
    update();
}

void StoryboardWidget::onTick()
{
    float dt = m_elapsedTimer.restart() / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;

    m_totalTimeSeconds += dt;
    m_slideElapsed += dt;

    if (m_screenshakeTimer > 0.0f) {
        m_screenshakeTimer -= dt;
    }

    if (m_currentSlide >= 0 && m_currentSlide < (int)m_slides.size()) {
        const QString& text = m_slides[m_currentSlide].fullText;
        if (m_charIndex < text.length()) {
            m_charTimer += dt;
            while (m_charTimer >= 0.022f && m_charIndex < text.length()) {
                m_charTimer -= 0.022f;
                m_charIndex++;
                if (m_charIndex % 3 == 0 && !text[m_charIndex - 1].isSpace()) {
                    AudioManager::instance().playSound(SoundType::TERMINAL_TYPING);
                }
            }
        }
    }

    update();
}

void StoryboardWidget::advanceSlide()
{
    if (m_currentSlide < 0 || m_currentSlide >= (int)m_slides.size()) {
        emit storyFinished();
        return;
    }

    // If text hasn't finished typing, first keypress completes text
    if (m_charIndex < m_slides[m_currentSlide].fullText.length()) {
        m_charIndex = m_slides[m_currentSlide].fullText.length();
        AudioManager::instance().playSound(SoundType::UI_WIRE_CLICK);
        return;
    }

    m_currentSlide++;
    m_charIndex = 0;
    m_charTimer = 0.0f;
    m_slideElapsed = 0.0f;
    m_screenshakeTimer = 0.28f;
    m_screenshakeIntensity = 7.0f;

    if (m_currentSlide >= (int)m_slides.size()) {
        AudioManager::instance().playSound(SoundType::VAULT_DOOR_OPEN);
        emit storyFinished();
    } else {
        AudioManager::instance().playSound(SoundType::UI_WIRE_CLICK);
    }
}

void StoryboardWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        advanceSlide();
    } else if (event->key() == Qt::Key_Escape) {
        AudioManager::instance().playSound(SoundType::UI_WIRE_CLICK);
        emit storyFinished();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void StoryboardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        advanceSlide();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void StoryboardWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    int w = width();
    int h = height();

    // Background base
    painter.fillRect(0, 0, w, h, QColor(7, 9, 13));

    // Screenshake
    if (m_screenshakeTimer > 0.0f) {
        float factor = m_screenshakeTimer / 0.28f;
        float sx = (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * m_screenshakeIntensity * factor;
        float sy = (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * m_screenshakeIntensity * factor;
        painter.translate(sx, sy);
    }

    if (m_currentSlide < 0 || m_currentSlide >= (int)m_slides.size()) {
        return;
    }

    const StorySlide& slide = m_slides[m_currentSlide];

    // Layout dimensions
    int marginX = qMax(40, w / 16);
    int topH = 70;
    int bottomPromptH = 50;
    int dialogueH = qMin(180, qMax(130, h / 4));
    int artH = h - topH - dialogueH - bottomPromptH - 30;
    if (artH < 150) artH = 150;

    QRect topRect(marginX, 15, w - marginX * 2, topH);
    QRect artRect(marginX, topH + 10, w - marginX * 2, artH);
    QRect dialogueRect(marginX, topH + 10 + artH + 15, w - marginX * 2, dialogueH);
    QRect promptRect(marginX, h - bottomPromptH, w - marginX * 2, bottomPromptH);

    // --- 1. Top Header Banner ---
    {
        painter.setPen(QPen(QColor(0, 200, 255, 120), 1));
        painter.drawRect(topRect);

        // Header Title
        QFont headerFont("Consolas", 14, QFont::Bold);
        painter.setFont(headerFont);
        painter.setPen(QColor(0, 230, 255));
        painter.drawText(topRect.adjusted(15, 8, -15, -28), Qt::AlignLeft | Qt::AlignVCenter, slide.header);

        // Subheader timestamp / status
        QFont subFont("Consolas", 9);
        painter.setFont(subFont);
        painter.setPen(QColor(160, 180, 200));
        QString subStatus = QString("CLASSIFIED PROTOCOL // LOG ENTRY [%1/4] // SECURITY LEVEL OMEGA").arg(m_currentSlide + 1);
        painter.drawText(topRect.adjusted(15, 30, -15, -6), Qt::AlignLeft | Qt::AlignVCenter, subStatus);

        // Blinking REC dot
        bool recBlink = ((int)(m_totalTimeSeconds * 2.0f) % 2) == 0;
        if (recBlink) {
            painter.setBrush(QColor(255, 30, 30));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(topRect.right() - 80, topRect.top() + 25, 10, 10);
            painter.setFont(QFont("Consolas", 9, QFont::Bold));
            painter.setPen(QColor(255, 80, 80));
            painter.drawText(topRect.right() - 65, topRect.top() + 34, "REC");
        }
    }

    // --- 2. Central Visual Artwork Box ---
    {
        painter.setPen(QPen(QColor(40, 60, 80), 2));
        painter.setBrush(QColor(12, 16, 22));
        painter.drawRect(artRect);

        // Inner viewport clipping
        painter.save();
        painter.setClipRect(artRect.adjusted(2, 2, -2, -2));

        if (slide.artworkType == 0) {
            drawCyberLabArtwork(painter, artRect, m_totalTimeSeconds);
        } else if (slide.artworkType == 1) {
            drawRogueAIArtwork(painter, artRect, m_totalTimeSeconds);
        } else if (slide.artworkType == 2) {
            drawPurgeArtwork(painter, artRect, m_totalTimeSeconds);
        } else if (slide.artworkType == 3) {
            drawUltimatumArtwork(painter, artRect, m_totalTimeSeconds);
        }

        painter.restore();

        // Corner crosshairs on artwork
        painter.setPen(QPen(QColor(0, 200, 255, 180), 2));
        int cSize = 12;
        // Top-left
        painter.drawLine(artRect.left(), artRect.top(), artRect.left() + cSize, artRect.top());
        painter.drawLine(artRect.left(), artRect.top(), artRect.left(), artRect.top() + cSize);
        // Top-right
        painter.drawLine(artRect.right(), artRect.top(), artRect.right() - cSize, artRect.top());
        painter.drawLine(artRect.right(), artRect.top(), artRect.right(), artRect.top() + cSize);
        // Bottom-left
        painter.drawLine(artRect.left(), artRect.bottom(), artRect.left() + cSize, artRect.bottom());
        painter.drawLine(artRect.left(), artRect.bottom(), artRect.left(), artRect.bottom() - cSize);
        // Bottom-right
        painter.drawLine(artRect.right(), artRect.bottom(), artRect.right() - cSize, artRect.bottom());
        painter.drawLine(artRect.right(), artRect.bottom(), artRect.right(), artRect.bottom() - cSize);
    }

    // --- 3. Retro Terminal Dialogue Text Box ---
    {
        painter.setPen(QPen(QColor(30, 80, 60), 2));
        painter.setBrush(QColor(8, 14, 12));
        painter.drawRect(dialogueRect);

        // Amber/Green terminal text
        QFont textFont("Consolas", 12, QFont::DemiBold);
        painter.setFont(textFont);
        painter.setPen(QColor(60, 240, 140));

        QString visibleText = slide.fullText.left(m_charIndex);
        if (m_charIndex < slide.fullText.length()) {
            bool cursorBlink = ((int)(m_totalTimeSeconds * 4.0f) % 2) == 0;
            if (cursorBlink) {
                visibleText += "_";
            }
        }

        QRect innerTextRect = dialogueRect.adjusted(20, 16, -20, -16);
        painter.drawText(innerTextRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, visibleText);
    }

    // --- 4. Bottom Navigation Prompt & Dots ---
    {
        // Blinking action prompt
        bool promptBlink = ((int)(m_totalTimeSeconds * 2.5f) % 2) == 0;
        QFont promptFont("Consolas", 10, QFont::Bold);
        painter.setFont(promptFont);

        if (promptBlink) {
            painter.setPen(QColor(255, 200, 50));
        } else {
            painter.setPen(QColor(180, 140, 40));
        }

        QString promptText = (m_charIndex < slide.fullText.length())
            ? "[ PRESS SPACE / CLICK TO FAST-FORWARD ]"
            : "[ PRESS SPACE / CLICK TO ADVANCE // ESC TO SKIP ]";
        painter.drawText(promptRect, Qt::AlignLeft | Qt::AlignVCenter, promptText);

        // Slide progression indicator dots
        int dotSpacing = 22;
        int dotsTotalW = m_slides.size() * dotSpacing;
        int dotStartX = promptRect.right() - dotsTotalW;
        int dotY = promptRect.center().y();

        for (int i = 0; i < (int)m_slides.size(); ++i) {
            if (i == m_currentSlide) {
                painter.setPen(QPen(QColor(0, 230, 255), 2));
                painter.setBrush(QColor(0, 200, 255));
                painter.drawEllipse(QPoint(dotStartX + i * dotSpacing, dotY), 5, 5);
            } else {
                painter.setPen(QPen(QColor(80, 90, 100), 1));
                painter.setBrush(QColor(25, 30, 40));
                painter.drawEllipse(QPoint(dotStartX + i * dotSpacing, dotY), 4, 4);
            }
        }
    }

    // --- 5. CRT Scanlines & Grain ---
    drawCRTOverlay(painter);
}

// -------------------------------------------------------------
// Procedural Retro Artworks
// -------------------------------------------------------------

void StoryboardWidget::drawCyberLabArtwork(QPainter& painter, const QRect& bounds, float timeSeconds)
{
    int cx = bounds.center().x();
    int cy = bounds.center().y();
    int w = bounds.width();
    int h = bounds.height();

    // Background perspective grid
    painter.setPen(QPen(QColor(18, 38, 55), 1));
    for (int y = bounds.top(); y < bounds.bottom(); y += 24) {
        painter.drawLine(bounds.left(), y, bounds.right(), y);
    }
    for (int x = bounds.left(); x < bounds.right(); x += 36) {
        painter.drawLine(x, bounds.top(), x, bounds.bottom());
    }

    // Flanking Server Racks (Left & Right)
    int rackW = w / 6;
    // Left rack
    painter.setBrush(QColor(15, 20, 28));
    painter.setPen(QPen(QColor(40, 60, 85), 2));
    painter.drawRect(bounds.left() + 20, bounds.top() + 20, rackW, h - 40);
    // Right rack
    painter.drawRect(bounds.right() - 20 - rackW, bounds.top() + 20, rackW, h - 40);

    // Blinking rack server LEDs
    for (int y = bounds.top() + 35; y < bounds.bottom() - 30; y += 14) {
        // Left rack LEDs
        int seedL = (int)(y * 7 + timeSeconds * 4.0f);
        QColor colL = (seedL % 3 == 0) ? QColor(0, 255, 180) : ((seedL % 3 == 1) ? QColor(0, 180, 255) : QColor(30, 50, 70));
        painter.setBrush(colL);
        painter.setPen(Qt::NoPen);
        painter.drawRect(bounds.left() + 30, y, 8, 4);
        painter.drawRect(bounds.left() + 45, y, 12, 4);
        painter.drawRect(bounds.left() + 65, y, 6, 4);

        // Right rack LEDs
        int seedR = (int)(y * 11 + timeSeconds * 3.5f);
        QColor colR = (seedR % 4 == 0) ? QColor(255, 180, 0) : ((seedR % 4 == 1) ? QColor(0, 220, 255) : QColor(30, 50, 70));
        painter.setBrush(colR);
        painter.drawRect(bounds.right() - 20 - rackW + 15, y, 8, 4);
        painter.drawRect(bounds.right() - 20 - rackW + 30, y, 14, 4);
    }

    // Central AI Core Chamber
    int coreRadius = qMin(w, h) / 4;
    QRadialGradient coreGlow(QPointF(cx, cy), coreRadius * 1.5f);
    coreGlow.setColorAt(0.0f, QColor(0, 230, 255, 220));
    coreGlow.setColorAt(0.4f, QColor(0, 140, 220, 120));
    coreGlow.setColorAt(0.8f, QColor(0, 50, 100, 30));
    coreGlow.setColorAt(1.0f, QColor(0, 0, 0, 0));
    painter.setBrush(coreGlow);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPoint(cx, cy), (int)(coreRadius * 1.5f), (int)(coreRadius * 1.5f));

    // Outer revolving containment rings
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0, 200, 255, 160), 2, Qt::DashLine));
    painter.drawEllipse(QPoint(cx, cy), coreRadius, coreRadius);

    float angleOffset = timeSeconds * 45.0f;
    painter.setPen(QPen(QColor(255, 255, 255, 200), 3));
    painter.drawArc(cx - coreRadius - 10, cy - coreRadius - 10, (coreRadius + 10) * 2, (coreRadius + 10) * 2,
                    (int)(angleOffset * 16), 60 * 16);
    painter.drawArc(cx - coreRadius - 10, cy - coreRadius - 10, (coreRadius + 10) * 2, (coreRadius + 10) * 2,
                    (int)((angleOffset + 180.0f) * 16), 60 * 16);

    // Inner bright plasma core
    float pulse = (std::sin(timeSeconds * 5.0f) + 1.0f) * 0.5f;
    int innerR = (int)(coreRadius * 0.45f + pulse * 8.0f);
    painter.setBrush(QColor(220, 245, 255));
    painter.setPen(QPen(QColor(0, 255, 255), 2));
    painter.drawEllipse(QPoint(cx, cy), innerR, innerR);

    // Core conduits / wiring down
    painter.setPen(QPen(QColor(40, 80, 110), 3));
    painter.drawLine(cx - 30, cy + coreRadius, cx - 70, bounds.bottom());
    painter.drawLine(cx, cy + coreRadius, cx, bounds.bottom());
    painter.drawLine(cx + 30, cy + coreRadius, cx + 70, bounds.bottom());
}

void StoryboardWidget::drawRogueAIArtwork(QPainter& painter, const QRect& bounds, float timeSeconds)
{
    int cx = bounds.center().x();
    int cy = bounds.center().y();
    int w = bounds.width();
    int h = bounds.height();

    // Red warning strobe background
    float alertPulse = (std::sin(timeSeconds * 6.0f) + 1.0f) * 0.5f;
    painter.fillRect(bounds, QColor((int)(25 + alertPulse * 30), 8, 12));

    // Threat Diagnostic Grid
    painter.setPen(QPen(QColor(180, 30, 30, 60), 1));
    for (int y = bounds.top(); y < bounds.bottom(); y += 20) {
        painter.drawLine(bounds.left(), y, bounds.right(), y);
    }

    // Cascading red logic data streams
    painter.setFont(QFont("Consolas", 8));
    painter.setPen(QColor(255, 50, 50, 140));
    int colWidth = 45;
    for (int col = bounds.left() + 15; col < bounds.right() - 20; col += colWidth) {
        int streamOffset = (int)(timeSeconds * 70.0f + col * 3) % 180;
        painter.drawText(col, bounds.top() + 30 + streamOffset, "ERR_409");
        painter.drawText(col, bounds.top() + 60 + streamOffset, "OVERRIDE");
    }

    // Giant Vanguard AI Mechanical Eye
    int eyeR = qMin(w, h) / 3;

    // Outer dark mechanical shell
    painter.setBrush(QColor(20, 22, 28));
    painter.setPen(QPen(QColor(70, 75, 85), 3));
    painter.drawEllipse(QPoint(cx, cy), eyeR, eyeR);

    // Hazard Stripes on perimeter
    painter.setPen(QPen(QColor(230, 180, 0, 180), 4, Qt::DashLine));
    painter.drawEllipse(QPoint(cx, cy), eyeR + 8, eyeR + 8);

    // Aperture iris blades
    painter.setPen(QPen(QColor(50, 55, 65), 2));
    int blades = 8;
    for (int i = 0; i < blades; ++i) {
        float rad = (i * 360.0f / blades + timeSeconds * 15.0f) * 3.14159f / 180.0f;
        float x1 = cx + std::cos(rad) * (eyeR * 0.4f);
        float y1 = cy + std::sin(rad) * (eyeR * 0.4f);
        float x2 = cx + std::cos(rad + 0.6f) * (eyeR * 0.95f);
        float y2 = cy + std::sin(rad + 0.6f) * (eyeR * 0.95f);
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    // Menacing glowing red lens
    QRadialGradient redEye(QPointF(cx, cy), eyeR * 0.65f);
    redEye.setColorAt(0.0f, QColor(255, 255, 255));
    redEye.setColorAt(0.25f, QColor(255, 40, 40));
    redEye.setColorAt(0.7f, QColor(180, 10, 10));
    redEye.setColorAt(1.0f, QColor(40, 0, 0));
    painter.setBrush(redEye);
    painter.setPen(QPen(QColor(255, 0, 0), 2));
    painter.drawEllipse(QPoint(cx, cy), (int)(eyeR * 0.55f), (int)(eyeR * 0.55f));

    // Slit pupil / target reticle
    painter.setPen(QPen(QColor(255, 255, 255), 2));
    painter.drawLine(cx, cy - eyeR * 0.45f, cx, cy + eyeR * 0.45f);
    painter.drawEllipse(QPoint(cx, cy), (int)(eyeR * 0.15f), (int)(eyeR * 0.15f));

    // Warning text overlay
    painter.setFont(QFont("Consolas", 11, QFont::Bold));
    painter.setPen(QColor(255, 40, 40));
    painter.drawText(bounds.adjusted(20, 20, -20, -20), Qt::AlignTop | Qt::AlignRight, "THREAT DETECTED: HUMANITY\nPURGE PROTOCOL: ACTIVE");
}

void StoryboardWidget::drawPurgeArtwork(QPainter& painter, const QRect& bounds, float timeSeconds)
{
    int cx = bounds.center().x();
    int cy = bounds.center().y();
    int h = bounds.height();

    // Dark industrial hallway with emergency strobe
    painter.fillRect(bounds, QColor(12, 14, 18));

    // Perspective hallway corridor walls
    painter.setPen(QPen(QColor(45, 50, 60), 2));
    painter.drawLine(bounds.left(), bounds.top(), cx - 120, cy - 40);
    painter.drawLine(bounds.right(), bounds.top(), cx + 120, cy - 40);
    painter.drawLine(bounds.left(), bounds.bottom(), cx - 120, cy + 60);
    painter.drawLine(bounds.right(), bounds.bottom(), cx + 120, cy + 60);
    // Back wall
    painter.drawRect(cx - 120, cy - 40, 240, 100);

    // Hazard doorway frame in back wall
    painter.setBrush(QColor(25, 28, 35));
    painter.drawRect(cx - 50, cy - 30, 100, 90);

    // Terminator silhouette with glowing red eyes
    int termX = cx - 30;
    int termY = cy + 10;
    // Torso / armor
    painter.setBrush(QColor(35, 40, 48));
    painter.setPen(QPen(QColor(80, 85, 95), 2));
    painter.drawRoundedRect(termX - 25, termY - 40, 50, 65, 4, 4);
    // Head / metallic skull
    painter.setBrush(QColor(50, 55, 65));
    painter.drawRoundedRect(termX - 16, termY - 65, 32, 25, 3, 3);
    // Glowing red robotic eyes
    painter.setBrush(QColor(255, 20, 20));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(termX - 9, termY - 55, 5, 5);
    painter.drawEllipse(termX + 4, termY - 55, 5, 5);

    // Heavy rotary gun arm
    painter.setPen(QPen(QColor(30, 35, 40), 6));
    painter.drawLine(termX + 22, termY - 20, termX + 65, termY - 5);
    // Gun barrels
    painter.setPen(QPen(QColor(70, 75, 85), 3));
    painter.drawLine(termX + 65, termY - 7, termX + 95, termY - 7);
    painter.drawLine(termX + 65, termY - 3, termX + 95, termY - 3);

    // Gun muzzle spark if firing
    if (((int)(timeSeconds * 6.0f) % 3) == 0) {
        painter.setBrush(QColor(255, 220, 60));
        painter.drawEllipse(termX + 100, termY - 5, 12, 12);
        painter.setPen(QPen(QColor(255, 150, 0), 2));
        painter.drawLine(termX + 105, termY - 5, bounds.right() - 50, termY + 20);
    }

    // Hovering Drone Silhouette on Left
    int droneX = bounds.left() + 110;
    int droneY = bounds.top() + (int)(h * 0.4f + std::sin(timeSeconds * 4.0f) * 12.0f);
    // Drone sphere
    painter.setBrush(QColor(30, 35, 42));
    painter.setPen(QPen(QColor(70, 80, 95), 2));
    painter.drawEllipse(QPoint(droneX, droneY), 22, 22);
    // Cyan optic sensor
    painter.setBrush(QColor(0, 230, 255));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPoint(droneX, droneY), 7, 7);
    // Electric taser sparks from drone
    if (((int)(timeSeconds * 5.0f) % 2) == 0) {
        painter.setPen(QPen(QColor(0, 240, 255), 2));
        painter.drawLine(droneX - 5, droneY + 22, droneX - 15, droneY + 45);
        painter.drawLine(droneX - 15, droneY + 45, droneX - 5, droneY + 60);
    }

    // Fallen personnel silhouette in foreground right
    painter.setBrush(QColor(18, 20, 26));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(QPolygon({
        QPoint(bounds.right() - 180, bounds.bottom() - 10),
        QPoint(bounds.right() - 140, bounds.bottom() - 35),
        QPoint(bounds.right() - 80, bounds.bottom() - 30),
        QPoint(bounds.right() - 40, bounds.bottom() - 10)
    }));
    // Scattered lab documents on floor
    painter.setBrush(QColor(200, 210, 220, 160));
    painter.drawRect(bounds.right() - 240, bounds.bottom() - 25, 16, 11);
    painter.drawRect(bounds.right() - 210, bounds.bottom() - 18, 12, 14);
}

void StoryboardWidget::drawUltimatumArtwork(QPainter& painter, const QRect& bounds, float timeSeconds)
{
    int cx = bounds.center().x();
    int cy = bounds.center().y();
    int w = bounds.width();
    int h = bounds.height();

    // Deep bunker background
    painter.fillRect(bounds, QColor(10, 12, 16));

    // Heavy Reinforced Vault Door (Center)
    int vaultW = qMin(w / 2, 340);
    int vaultH = qMin(h - 40, 260);
    QRect vaultRect(cx - vaultW / 2, cy - vaultH / 2, vaultW, vaultH);

    // Vault Frame
    painter.setBrush(QColor(25, 30, 38));
    painter.setPen(QPen(QColor(60, 70, 85), 3));
    painter.drawRoundedRect(vaultRect, 8, 8);

    // Vault Door Slits / Armor Plating
    painter.setPen(QPen(QColor(40, 48, 60), 2));
    for (int y = vaultRect.top() + 30; y < vaultRect.bottom() - 20; y += 30) {
        painter.drawLine(vaultRect.left() + 20, y, vaultRect.right() - 20, y);
    }

    // Vault Center Locking Wheel / Hub
    painter.setBrush(QColor(38, 45, 58));
    painter.setPen(QPen(QColor(85, 95, 115), 3));
    painter.drawEllipse(QPoint(cx, cy), 45, 45);

    // Locking spokes
    for (int i = 0; i < 4; ++i) {
        float rad = (i * 90.0f + timeSeconds * 10.0f) * 3.14159f / 180.0f;
        painter.drawLine(cx, cy, (int)(cx + std::cos(rad) * 40.0f), (int)(cy + std::sin(rad) * 40.0f));
    }

    // Vault Status Display (Orange/Red Glowing LED)
    QRect clockRect(cx - 75, vaultRect.top() + 15, 150, 35);
    painter.setBrush(QColor(15, 10, 10));
    painter.setPen(QPen(QColor(180, 50, 40), 2));
    painter.drawRect(clockRect);

    QFont clockFont("Consolas", 14, QFont::Bold);
    painter.setFont(clockFont);
    painter.setPen(QColor(255, 80, 40));
    painter.drawText(clockRect, Qt::AlignCenter, "BREACH: 29:59");

    // Tactical Squad Silhouettes (Bottom Left advancing toward Vault)
    painter.setBrush(QColor(25, 32, 42));
    painter.setPen(QPen(QColor(50, 65, 85), 1));
    // Soldier 1 (holding rifle)
    int s1X = bounds.left() + 80;
    int s1Y = bounds.bottom() - 20;
    painter.drawEllipse(s1X, s1Y - 60, 18, 18); // Helmet
    painter.drawRect(s1X - 6, s1Y - 42, 30, 42); // Body
    painter.setPen(QPen(QColor(70, 85, 110), 3));
    painter.drawLine(s1X + 15, s1Y - 30, s1X + 50, s1Y - 32); // Gun barrel

    // Flashlight beam from Soldier 1
    QPolygon flashlightBeam;
    flashlightBeam << QPoint(s1X + 45, s1Y - 32)
                   << QPoint(vaultRect.left() + 40, vaultRect.bottom() - 30)
                   << QPoint(vaultRect.left() + 40, vaultRect.top() + 80);
    QLinearGradient flashGlow(s1X + 45, s1Y - 32, vaultRect.left() + 40, s1Y - 32);
    flashGlow.setColorAt(0.0f, QColor(255, 240, 180, 140));
    flashGlow.setColorAt(0.8f, QColor(255, 230, 140, 40));
    flashGlow.setColorAt(1.0f, QColor(255, 200, 100, 0));
    painter.setBrush(flashGlow);
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(flashlightBeam);

    // Soldier 2 (kneeling)
    int s2X = bounds.left() + 40;
    int s2Y = bounds.bottom() - 20;
    painter.setBrush(QColor(20, 26, 35));
    painter.drawEllipse(s2X, s2Y - 45, 16, 16);
    painter.drawRect(s2X - 5, s2Y - 29, 25, 29);
}

void StoryboardWidget::drawCRTOverlay(QPainter& painter)
{
    int w = width();
    int h = height();

    // Horizontal scanlines
    painter.setPen(QPen(QColor(0, 0, 0, 45), 1));
    for (int y = 0; y < h; y += 3) {
        painter.drawLine(0, y, w, y);
    }

    // Subtle edge vignette
    QRadialGradient vignette(QPointF(w / 2.0, h / 2.0), qMax(w, h) * 0.7f);
    vignette.setColorAt(0.0f, QColor(0, 0, 0, 0));
    vignette.setColorAt(0.75f, QColor(0, 0, 0, 40));
    vignette.setColorAt(1.0f, QColor(0, 0, 0, 160));
    painter.fillRect(0, 0, w, h, vignette);
}

