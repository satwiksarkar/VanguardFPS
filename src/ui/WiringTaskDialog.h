#ifndef WIRING_TASK_DIALOG_H
#define WIRING_TASK_DIALOG_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <QDialog>
#include <QPoint>
#include <QColor>

class WiringTaskDialog : public QDialog {
    Q_OBJECT

public:
    explicit WiringTaskDialog(int sector, QWidget *parent = nullptr);
    ~WiringTaskDialog() override = default;

    bool isCompleted() const { return m_isCompleted; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void initializeWires();
    bool checkAllConnected() const;

    int m_sector;
    bool m_isCompleted = false;

    // 4 Wire colors: Red, Blue, Yellow, Magenta
    std::array<QColor, 4> m_colors;
    std::array<int, 4> m_leftOrder;   // Shuffled color indices
    std::array<int, 4> m_rightOrder;  // Destination color indices

    // Connections: index on left connects to index on right (-1 if unconnected)
    std::array<int, 4> m_connections;

    int m_draggingLeftIndex = -1;
    QPoint m_dragPos;

    static constexpr int PIN_COUNT = 4;
    static constexpr int LEFT_X = 85;
    static constexpr int RIGHT_X = 495;
    static constexpr int START_Y = 125;
    static constexpr int PIN_SPACING = 68;
    static constexpr int PIN_RADIUS = 18;
};

#endif // WIRING_TASK_DIALOG_H

