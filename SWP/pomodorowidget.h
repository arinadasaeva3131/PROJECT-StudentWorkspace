#ifndef POMODOROWIDGET_H
#define POMODOROWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTime>

class PomodoroGLWidget;
class QComboBox;
class QSpinBox;
class QPushButton;

class PomodoroWidget : public QWidget {
    Q_OBJECT
public:
    explicit PomodoroWidget(QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void toggleTimer();
    void updateTimer();
    void loadSubjects();
private:
    void saveSession(int minutes);

    PomodoroGLWidget *m_glWidget;
    QComboBox *m_cbSubject;
    QSpinBox *m_sbWork;
    QSpinBox *m_sbRest;
    QPushButton *m_btnStart;

    QTimer *m_timer;
    bool m_isRunning;
    bool m_isWorkPhase;
    int m_timeLeft;
    int m_currentTotal;
    int m_elapsedSeconds; // Считаем каждую секунду
    int m_activeSubjectId; // Предмет, зафиксированный в начале сессии
};

#endif