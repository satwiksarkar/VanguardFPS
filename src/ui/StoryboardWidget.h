#ifndef STORYBOARD_WIDGET_H
#define STORYBOARD_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <vector>
#include <QPainter>
#include <QFontDatabase>

struct StorySlide {
    QString header;
    QString fullText;
    int artworkType; // 0 = CyberLab, 1 = RogueAI, 2 = Purge, 3 = Ultimatum
};

class StoryboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit StoryboardWidget(QWidget *parent = nullptr);
    ~StoryboardWidget() override = default;

    void resetStory();

signals:
    void storyFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onTick();

private:
    void drawCyberLabArtwork(QPainter& painter, const QRect& bounds, float timeSeconds);
    void drawRogueAIArtwork(QPainter& painter, const QRect& bounds, float timeSeconds);
    void drawPurgeArtwork(QPainter& painter, const QRect& bounds, float timeSeconds);
    void drawUltimatumArtwork(QPainter& painter, const QRect& bounds, float timeSeconds);
    void drawCRTOverlay(QPainter& painter);

    void advanceSlide();

    std::vector<StorySlide> m_slides;
    int m_currentSlide = 0;
    int m_charIndex = 0;
    float m_charTimer = 0.0f;
    float m_slideElapsed = 0.0f;
    float m_screenshakeTimer = 0.0f;
    float m_screenshakeIntensity = 0.0f;

    QTimer m_animTimer;
    QElapsedTimer m_elapsedTimer;
    float m_totalTimeSeconds = 0.0f;
};

#endif // STORYBOARD_WIDGET_H

