#include "WiringTaskDialog.h"
#include "world/TaskManager.h"
#include "audio/AudioManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QTimer>
#include <random>
#include <chrono>

WiringTaskDialog::WiringTaskDialog(int sector, QWidget *parent)
    : QDialog(parent)
    , m_sector(sector)
{
    setFixedSize(580, 440);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    // Standard Among Us wire colors: Red, Blue, Yellow, Magenta
    m_colors[0] = QColor(231, 76, 60);  // Red
    m_colors[1] = QColor(52, 152, 219); // Blue
    m_colors[2] = QColor(241, 196, 15); // Yellow
    m_colors[3] = QColor(155, 89, 182); // Magenta

    initializeWires();
}

void WiringTaskDialog::initializeWires() {
    for (int i = 0; i < PIN_COUNT; ++i) {
        m_leftOrder[i] = i;
        m_rightOrder[i] = i;
        m_connections[i] = -1;
    }

    // Seeded random shuffle for varied wire order
    unsigned seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
    std::default_random_engine rng(seed);

    std::shuffle(m_leftOrder.begin(), m_leftOrder.end(), rng);
    std::shuffle(m_rightOrder.begin(), m_rightOrder.end(), rng);

    // Ensure it's not pre-solved straight lines
    bool allSame = true;
    for (int i = 0; i < PIN_COUNT; ++i) {
        if (m_leftOrder[i] != m_rightOrder[i]) {
            allSame = false;
            break;
        }
    }
    if (allSame) {
        std::swap(m_rightOrder[0], m_rightOrder[1]);
    }
}

bool WiringTaskDialog::checkAllConnected() const {
    for (int i = 0; i < PIN_COUNT; ++i) {
        int rightIndex = m_connections[i];
        if (rightIndex == -1) return false;
        if (m_leftOrder[i] != m_rightOrder[rightIndex]) return false;
    }
    return true;
}

void WiringTaskDialog::mousePressEvent(QMouseEvent *event) {
    if (m_isCompleted) return;

    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();

        // Check close button [X] at (542, 14, 24, 24)
        QRect closeBtn(width() - 38, 14, 24, 24);
        if (closeBtn.contains(pos)) {
            reject();
            return;
        }

        // Check if user clicked on one of the left pins
        for (int i = 0; i < PIN_COUNT; ++i) {
            int py = START_Y + i * PIN_SPACING;
            int dx = pos.x() - LEFT_X;
            int dy = pos.y() - py;
            if (dx * dx + dy * dy <= PIN_RADIUS * PIN_RADIUS + 120) {
                m_draggingLeftIndex = i;
                m_dragPos = pos;
                m_connections[i] = -1; // Disconnect to re-drag
                AudioManager::instance().playSound(SoundType::UI_WIRE_CLICK);
                update();
                return;
            }
        }
    }
    QDialog::mousePressEvent(event);
}

void WiringTaskDialog::mouseMoveEvent(QMouseEvent *event) {
    if (m_draggingLeftIndex != -1) {
        m_dragPos = event->pos();
        update();
    }
    QDialog::mouseMoveEvent(event);
}

void WiringTaskDialog::mouseReleaseEvent(QMouseEvent *event) {
    if (m_draggingLeftIndex != -1 && event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();

        // Check if released on a right pin
        int matchedRightIndex = -1;
        for (int j = 0; j < PIN_COUNT; ++j) {
            int py = START_Y + j * PIN_SPACING;
            int dx = pos.x() - RIGHT_X;
            int dy = pos.y() - py;
            if (dx * dx + dy * dy <= (PIN_RADIUS + 12) * (PIN_RADIUS + 12)) {
                matchedRightIndex = j;
                break;
            }
        }

        if (matchedRightIndex != -1) {
            // Check if colors match
            if (m_leftOrder[m_draggingLeftIndex] == m_rightOrder[matchedRightIndex]) {
                // Clear any other left pin that was previously attached to this right pin
                for (int i = 0; i < PIN_COUNT; ++i) {
                    if (m_connections[i] == matchedRightIndex) {
                        m_connections[i] = -1;
                    }
                }
                m_connections[m_draggingLeftIndex] = matchedRightIndex;
                AudioManager::instance().playSound(SoundType::UI_WIRE_CLICK);

                if (checkAllConnected()) {
                    m_isCompleted = true;
                    update();
                    // Close dialog with success after 550ms
                    QTimer::singleShot(550, this, &QDialog::accept);
                }
            } else {
                // Wrong wire color -> snaps back
                m_connections[m_draggingLeftIndex] = -1;
            }
        } else {
            m_connections[m_draggingLeftIndex] = -1;
        }

        m_draggingLeftIndex = -1;
        update();
    }
    QDialog::mouseReleaseEvent(event);
}

void WiringTaskDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

void WiringTaskDialog::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();

    // 1. Industrial panel background
    painter.fillRect(rect(), QColor(20, 24, 30));

    // Outer beveled frame
    painter.setPen(QPen(QColor(60, 75, 95), 3));
    painter.drawRect(rect().adjusted(1, 1, -2, -2));

    painter.setPen(QPen(QColor(35, 45, 58), 1));
    painter.drawRect(rect().adjusted(6, 6, -7, -7));

    // Corner rivets
    auto drawRivet = [&](int rx, int ry) {
        painter.setBrush(QColor(90, 100, 115));
        painter.setPen(QColor(30, 35, 45));
        painter.drawEllipse(rx - 3, ry - 3, 6, 6);
    };
    drawRivet(14, 14); drawRivet(w - 14, 14);
    drawRivet(14, h - 14); drawRivet(w - 14, h - 14);

    // Title banner
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setBold(true);
    monoFont.setPointSize(11);
    painter.setFont(monoFont);

    painter.setPen(QColor(0, 230, 255));
    painter.drawText(24, 34, "ELECTRICAL JUNCTION TERMINAL // BREAKER CALIBRATION");

    monoFont.setPointSize(9);
    painter.setFont(monoFont);
    painter.setPen(QColor(160, 180, 200));
    QString secName = QString::fromStdString(TaskManager::getSectorName(m_sector));
    painter.drawText(24, 54, QString("LOCATION: %1").arg(secName));

    // Close button [X]
    QRect closeBtn(w - 38, 14, 24, 24);
    painter.fillRect(closeBtn, QColor(40, 20, 25));
    painter.setPen(QColor(220, 60, 60));
    painter.drawRect(closeBtn);
    painter.drawText(closeBtn, Qt::AlignCenter, "X");

    // Divider bar
    painter.setPen(QColor(45, 55, 70));
    painter.drawLine(20, 70, w - 20, 70);

    // 2. Draw Left and Right Sockets
    for (int i = 0; i < PIN_COUNT; ++i) {
        int py = START_Y + i * PIN_SPACING;

        // Left terminal base & colored pin
        QColor leftColor = m_colors[m_leftOrder[i]];
        QRect leftBox(LEFT_X - 45, py - 18, 45, 36);
        painter.fillRect(leftBox, QColor(32, 38, 48));
        painter.setPen(QColor(70, 80, 95));
        painter.drawRect(leftBox);

        // Terminal socket ring
        painter.setBrush(leftColor);
        painter.setPen(QPen(leftColor.lighter(130), 2));
        painter.drawEllipse(LEFT_X - 14, py - 14, 28, 28);
        painter.setBrush(QColor(20, 24, 30));
        painter.drawEllipse(LEFT_X - 5, py - 5, 10, 10);

        // Right terminal base & receptor pin
        QColor rightColor = m_colors[m_rightOrder[i]];
        QRect rightBox(RIGHT_X, py - 18, 45, 36);
        painter.fillRect(rightBox, QColor(32, 38, 48));
        painter.setPen(QColor(70, 80, 95));
        painter.drawRect(rightBox);

        // Receptor socket ring
        painter.setBrush(rightColor);
        painter.setPen(QPen(rightColor.lighter(130), 2));
        painter.drawEllipse(RIGHT_X - 14, py - 14, 28, 28);
        painter.setBrush(QColor(20, 24, 30));
        painter.drawEllipse(RIGHT_X - 5, py - 5, 10, 10);
    }

    // 3. Draw Connected Wires (curved Bezier paths)
    for (int i = 0; i < PIN_COUNT; ++i) {
        int rightIndex = m_connections[i];
        if (rightIndex != -1) {
            int startX = LEFT_X;
            int startY = START_Y + i * PIN_SPACING;
            int endX = RIGHT_X;
            int endY = START_Y + rightIndex * PIN_SPACING;

            QColor wireColor = m_colors[m_leftOrder[i]];

            QPainterPath path;
            path.moveTo(startX, startY);
            // Cubic bezier control points for realistic hanging electrical conduit
            path.cubicTo(startX + 130, startY + 10, endX - 130, endY + 10, endX, endY);

            // Wire shadow / outline
            painter.strokePath(path, QPen(QColor(10, 12, 16), 8));
            // Colored body
            painter.strokePath(path, QPen(wireColor, 5, Qt::SolidLine, Qt::RoundCap));
            // Specular core highlight
            painter.strokePath(path, QPen(wireColor.lighter(160), 1.5, Qt::SolidLine, Qt::RoundCap));

            // Connection spark indicator at right pin
            painter.setBrush(QColor(255, 255, 200));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(endX - 4, endY - 4, 8, 8);
        }
    }

    // 4. Draw currently dragged wire
    if (m_draggingLeftIndex != -1) {
        int startX = LEFT_X;
        int startY = START_Y + m_draggingLeftIndex * PIN_SPACING;
        int endX = m_dragPos.x();
        int endY = m_dragPos.y();

        QColor wireColor = m_colors[m_leftOrder[m_draggingLeftIndex]];

        QPainterPath path;
        path.moveTo(startX, startY);
        path.cubicTo(startX + (endX - startX) * 0.4, startY,
                     endX - (endX - startX) * 0.4, endY,
                     endX, endY);

        painter.strokePath(path, QPen(QColor(10, 12, 16), 8));
        painter.strokePath(path, QPen(wireColor, 5, Qt::SolidLine, Qt::RoundCap));
        painter.strokePath(path, QPen(wireColor.lighter(160), 1.5, Qt::SolidLine, Qt::RoundCap));

        // Drag tip head
        painter.setBrush(wireColor.lighter(140));
        painter.drawEllipse(endX - 7, endY - 7, 14, 14);
    }

    // 5. Footer status readout
    QRect footerBox(20, h - 50, w - 40, 34);
    painter.fillRect(footerBox, QColor(14, 18, 24));
    painter.setPen(QColor(40, 50, 65));
    painter.drawRect(footerBox);

    monoFont.setPointSize(10);
    painter.setFont(monoFont);

    if (m_isCompleted) {
        painter.setPen(QColor(0, 255, 120));
        painter.drawText(footerBox, Qt::AlignCenter, "ALL CIRCUITS CONNECTED! AUXILIARY POWER RESTORED!");
    } else {
        painter.setPen(QColor(200, 215, 230));
        painter.drawText(footerBox, Qt::AlignCenter, "DRAG WIRES TO MATCHING COLOR TERMINALS | [ESC] EXIT");
    }
}

