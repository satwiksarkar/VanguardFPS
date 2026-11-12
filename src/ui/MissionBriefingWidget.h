#ifndef MISSION_BRIEFING_WIDGET_H
#define MISSION_BRIEFING_WIDGET_H

#include <cstdint>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QFontDatabase>

class MissionBriefingWidget : public QWidget {
    Q_OBJECT

public:
    explicit MissionBriefingWidget(QWidget *parent = nullptr);
    ~MissionBriefingWidget() override = default;

    void updateHostStatus(bool isHost, bool isSolo = false);

signals:
    void beginProtocolRequested();
    void abortMissionRequested();

private slots:
    void onBeginProtocolClicked();
    void onAbortClicked();

private:
    void setupUI();
    QFrame* createCard(const QString& title, const QString& badge, const QColor& badgeCol, QWidget* contentWidget);

    QPushButton* m_beginProtocolBtn = nullptr;
    QPushButton* m_abortBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_modeHeaderLabel = nullptr;

    bool m_isHost = true;
    bool m_isSolo = false;
};

#endif // MISSION_BRIEFING_WIDGET_H

