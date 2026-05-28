#include "pomodoroglwidget.h"
#include <cmath>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <QRandomGenerator>

const float F_PI = 3.14159265358979323846f;

static const QStringList motivationalPhrases = {
    "Ты справишься!", "Каждый шаг приближает к цели", "Не сдавайся!",
    "У тебя всё получится", "Работай с умом, а не на износ", "Помни, зачем ты начал",
    "Твоё будущее создаётся сегодня", "Великие дела требуют времени", "Сфокусируйся на главном", "Ты молодец!"
};

PomodoroGLWidget::PomodoroGLWidget(QWidget *parent) : QOpenGLWidget(parent), m_phraseTimer(nullptr) {
    m_rotX = 15.0f;
    m_rotY = 0.0f;

    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, [this]() {
        m_animationTime += 0.015f;
        update();
    });
    m_animationTimer->start(16);
}

void PomodoroGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.09f, 0.09f, 0.09f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void PomodoroGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = float(w) / float(h ? h : 1);
    float fovY = 45.0f, nearPlane = 0.1f, farPlane = 100.0f;
    float top = nearPlane * tanf(fovY * F_PI / 360.0f);
    float right = top * aspect;
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

void PomodoroGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -8.0f * m_zoom);
    glRotatef(m_rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(m_rotY, 0.0f, 1.0f, 0.0f);

    float fraction = (m_total > 0) ? float(m_remaining) / float(m_total) : 0.0f;
    float visualScale = m_isWork ? (0.2f + 0.8f * fraction) : (0.2f + 0.8f * (1.0f - fraction));
    float morphProgress = m_isWork ? fraction : (1.0f - fraction);

    glPushMatrix();
    glScalef(visualScale, visualScale, visualScale);
    drawEnergyBattery(m_isWork, morphProgress);
    glPopMatrix();

    drawOrbitalParticles(m_isWork);

    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 48, QFont::Bold));
    QString text = m_active ? QString("%1:%2").arg(m_remaining / 60, 2, 10, QChar('0')).arg(m_remaining % 60, 2, 10, QChar('0')) : "00:00";
    painter.drawText(rect().adjusted(0, 30, 0, 0), Qt::AlignTop | Qt::AlignHCenter, text);

    if (!m_currentPhrase.isEmpty()) {
        painter.setFont(QFont("Arial", 18, QFont::StyleItalic));
        painter.setPen(QColor(255, 255, 186));
        painter.drawText(rect().adjusted(0, 0, 0, -30), Qt::AlignBottom | Qt::AlignHCenter, m_currentPhrase);
    }
    painter.end();
}

void PomodoroGLWidget::mousePressEvent(QMouseEvent *e) { m_lastPos = e->pos(); }
void PomodoroGLWidget::mouseMoveEvent(QMouseEvent *e) {
    if (e->buttons() & Qt::LeftButton) {
        m_rotY += (e->pos().x() - m_lastPos.x()) * 0.5f;
        m_rotX += (e->pos().y() - m_lastPos.y()) * 0.5f;
    }
    m_lastPos = e->pos(); update();
}
void PomodoroGLWidget::wheelEvent(QWheelEvent *e) {
    m_zoom += e->angleDelta().y() / 120.0f * 0.1f;
    if (m_zoom < 0.5f) m_zoom = 0.5f;
    if (m_zoom > 3.0f) m_zoom = 3.0f;
    update();
}

void PomodoroGLWidget::setActive(bool active) {
    m_active = active;
    if (active) {
        if (!m_phraseTimer) {
            m_phraseTimer = new QTimer(this);
            connect(m_phraseTimer, &QTimer::timeout, this, &PomodoroGLWidget::showRandomPhrase);
        }
        QTimer::singleShot(5000, this, &PomodoroGLWidget::showRandomPhrase);
        if (!m_phraseTimer->isActive()) m_phraseTimer->start(30000);
    } else {
        if (m_phraseTimer) m_phraseTimer->stop();
        m_currentPhrase.clear();
    }
    update();
}

void PomodoroGLWidget::setTime(int totalSeconds, int remainingSeconds, bool isWork) {
    m_total = totalSeconds; m_remaining = remainingSeconds; m_isWork = isWork; update();
}

void PomodoroGLWidget::drawMeltingConeStarMesh(float maxRadius, float thickness, int slices, int stacks, float morphProgress) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    float currentRadius = maxRadius + 0.03f * sinf(m_animationTime * 0.5f);

    for (int i = 0; i < stacks; ++i) {
        float phi0 = F_PI * (-0.5f + (float)i / stacks);
        float phi1 = F_PI * (-0.5f + (float)(i + 1) / stacks);
        float y0 = thickness * sinf(phi0), y1 = thickness * sinf(phi1);
        float zr0 = cosf(phi0), zr1 = cosf(phi1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * F_PI * (float)j / slices;
            float starAngle = fmodf(theta + F_PI / 2.0f, 2.0f * F_PI / 5.0f) - (F_PI / 5.0f);
            float normAngle = qAbs(starAngle) / (F_PI / 5.0f);
            float starModStrength = 0.52f * morphProgress;
            float starMod = (1.0f - starModStrength) + starModStrength * (1.0f - normAngle);
            float r0 = currentRadius * starMod * zr0, r1 = currentRadius * starMod * zr1;
            float cosT = cosf(theta), sinT = sinf(theta);
            glNormal3f(cosT * zr0 * starMod, sinf(phi0), sinT * zr0 * starMod);
            glVertex3f(r0 * cosT, y0, r0 * sinT);
            glVertex3f(r1 * cosT, y1, r1 * sinT);
        }
        glEnd();
    }
}

void PomodoroGLWidget::drawSphere(float radius, int slices, int stacks) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (int i = 0; i < stacks; ++i) {
        float lat0 = F_PI * (-0.5f + (float)i / stacks);
        float lat1 = F_PI * (-0.5f + (float)(i + 1) / stacks);
        float z0 = sinf(lat0), zr0 = cosf(lat0);
        float z1 = sinf(lat1), zr1 = cosf(lat1);
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float x = cosf(2.0f * F_PI * j / slices), y = sinf(2.0f * F_PI * j / slices);
            glNormal3f(x * zr0, y * zr0, z0); glVertex3f(radius * x * zr0, radius * y * zr0, radius * z0);
            glNormal3f(x * zr1, y * zr1, z1); glVertex3f(radius * x * zr1, radius * y * zr1, radius * z1);
        }
        glEnd();
    }
}

void PomodoroGLWidget::drawEnergyBattery(bool isWork, float morphProgress) {
    QColor currentEnergy = isWork ? QColor(255, 175, 0, 240) : QColor(0, 230, 255, 240);
    float maxRadius = 1.8f, thickness = 0.8f;
    float twinkle = 0.15f * sinf(m_animationTime * 5.0f);
    float alpha = qBound(0.0f, currentEnergy.alphaF() + twinkle, 1.0f);

    glPushMatrix();
    glRotatef(m_animationTime * 1.3f, 0.0f, 1.0f, 0.0f);
    glRotatef(1.5f * sinf(m_animationTime * 0.4f), 1.0f, 0.0f, 0.0f);

    glColor4f(currentEnergy.redF(), currentEnergy.greenF(), currentEnergy.blueF(), alpha);
    drawMeltingConeStarMesh(maxRadius, thickness, 65, 40, morphProgress);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); glLineWidth(1.0f);
    glColor4f(1.0f, 0.95f, 0.7f, 0.1f + twinkle*0.2f);
    drawMeltingConeStarMesh(maxRadius, thickness, 65, 40, morphProgress);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

void PomodoroGLWidget::drawOrbitalParticles(bool isWork) {
    QColor currentEnergy = isWork ? QColor(255, 175, 0, 240) : QColor(0, 230, 255, 240);
    int dropsCount = 6;
    for (int i = 0; i < dropsCount; ++i) {
        glPushMatrix();
        float currentAngle = i * (2.0f * F_PI / dropsCount) + ((i % 2 == 0 ? 0.04f : -0.03f) * m_animationTime);
        float orbitDistance = 2.4f + 0.15f * sinf(m_animationTime * 0.5f + i);
        glTranslatef(cosf(currentAngle) * orbitDistance, sinf(m_animationTime * 0.3f + i) * 0.5f, sinf(currentAngle) * orbitDistance);
        glColor4f(currentEnergy.redF(), currentEnergy.greenF(), currentEnergy.blueF(), 0.55f);
        drawSphere(0.15f + 0.02f * sinf(m_animationTime * 0.7f + i), 12, 8);
        glPopMatrix();
    }
}

void PomodoroGLWidget::showRandomPhrase() {
    if (motivationalPhrases.isEmpty()) return;
    m_currentPhrase = motivationalPhrases[QRandomGenerator::global()->bounded(motivationalPhrases.size())];
    update();
    QTimer::singleShot(3000, this, &PomodoroGLWidget::hidePhrase);
}

void PomodoroGLWidget::hidePhrase() { m_currentPhrase.clear(); update(); }