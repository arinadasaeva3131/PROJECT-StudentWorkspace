#ifndef POMODOROGLWIDGET_H
#define POMODOROGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_1_1>
#include <QTimer>
#include <QPoint>

class PomodoroGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_1_1 {
    Q_OBJECT
public:
    explicit PomodoroGLWidget(QWidget *parent = nullptr);
    void setActive(bool active);
    void setTime(int totalSeconds, int remainingSeconds, bool isWork);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    void drawMeltingConeStarMesh(float maxRadius, float thickness, int slices, int stacks, float morphProgress);
    void drawSphere(float radius, int slices, int stacks);
    void drawEnergyBattery(bool isWork, float morphProgress);
    void drawOrbitalParticles(bool isWork);
    void showRandomPhrase();
    void hidePhrase();

    QTimer *m_animationTimer;
    QTimer *m_phraseTimer;
    float m_animationTime = 0.0f;
    float m_rotX = 0.0f, m_rotY = 0.0f, m_zoom = 1.0f;
    QPoint m_lastPos;

    bool m_active = false;
    bool m_isWork = true;
    int m_total = 0;
    int m_remaining = 0;
    QString m_currentPhrase;
};

#endif // POMODOROGLWIDGET_H