#ifndef RIDDLE_TERMINAL_DIALOG_H
#define RIDDLE_TERMINAL_DIALOG_H

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class RiddleTerminalDialog : public QDialog {
    Q_OBJECT

public:
    explicit RiddleTerminalDialog(int sector, QWidget *parent = nullptr);
    ~RiddleTerminalDialog() override = default;

    bool isSolved() const { return m_isSolved; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onSubmit();
    void onHint();

private:
    void setupUI();
    bool checkAnswer(const QString& answer) const;

    int m_sector;
    bool m_isSolved = false;

    QString m_riddleText;
    QString m_hintText;
    std::vector<QString> m_validAnswers;

    QLineEdit *m_answerInput = nullptr;
    QPushButton *m_submitBtn = nullptr;
    QPushButton *m_hintBtn = nullptr;
    QPushButton *m_abortBtn = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // RIDDLE_TERMINAL_DIALOG_H

