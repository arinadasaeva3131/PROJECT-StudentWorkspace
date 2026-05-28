#include "pomodorowidget.h"
#include "pomodoroglwidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDate>
#include <QSqlError>
#include <QDebug>

void PomodoroWidget::saveSession(int minutes) {
    if (minutes <= 0) return;

    QSqlQuery q;
    q.prepare("INSERT INTO pomodoro_sessions (subject_id, work_duration_min, date) VALUES (?, ?, ?)");
    q.addBindValue(m_activeSubjectId == -1 ? QVariant() : m_activeSubjectId);
    q.addBindValue(minutes);
    q.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));

    if (!q.exec()) {
        qDebug() << "!!! ОШИБКА ЗАПИСИ В БД:" << q.lastError().text();
    } else {
        qDebug() << "Успешно сохранено:" << minutes << "мин. для ID предмета:" << m_activeSubjectId;
    }
}

PomodoroWidget::PomodoroWidget(QWidget *parent)
    : QWidget(parent), m_isRunning(false), m_isWorkPhase(true), m_timeLeft(0), m_elapsedSeconds(0), m_activeSubjectId(-1) {

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    QLabel *title = new QLabel("Таймер", this);
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: #F5F5F5;");
    layout->addWidget(title);

    QHBoxLayout *settingsLayout = new QHBoxLayout();
    settingsLayout->addWidget(new QLabel("Предмет:", this));
    m_cbSubject = new QComboBox(this);
    m_cbSubject->setStyleSheet("background: #2A2A35; color: white; padding: 6px; font-size: 16px; border-radius: 6px;");
    settingsLayout->addWidget(m_cbSubject);

    settingsLayout->addWidget(new QLabel(" Работа (мин):", this));
    m_sbWork = new QSpinBox(this); m_sbWork->setRange(1, 120); m_sbWork->setValue(25);
    m_sbWork->setStyleSheet("background: #2A2A35; color: white; padding: 6px; font-size: 16px; border-radius: 6px;");
    settingsLayout->addWidget(m_sbWork);

    settingsLayout->addWidget(new QLabel(" Отдых (мин):", this));
    m_sbRest = new QSpinBox(this); m_sbRest->setRange(1, 60); m_sbRest->setValue(5);
    m_sbRest->setStyleSheet("background: #2A2A35; color: white; padding: 6px; font-size: 16px; border-radius: 6px;");
    settingsLayout->addWidget(m_sbRest);

    m_btnStart = new QPushButton("Старт", this);
    m_btnStart->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; font-size: 16px; padding: 10px 20px; border-radius: 8px;");
    settingsLayout->addWidget(m_btnStart);
    settingsLayout->addStretch();
    layout->addLayout(settingsLayout);

    m_glWidget = new PomodoroGLWidget(this);
    layout->addWidget(m_glWidget, 1);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PomodoroWidget::updateTimer);
    connect(m_btnStart, &QPushButton::clicked, this, &PomodoroWidget::toggleTimer);
}

void PomodoroWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    int savedSubjectId = m_cbSubject->currentData().toInt();
    loadSubjects();
    int index = m_cbSubject->findData(savedSubjectId);
    if (index != -1) {
        m_cbSubject->setCurrentIndex(index);
    } else {
        m_cbSubject->setCurrentIndex(0);
    }
}

void PomodoroWidget::loadSubjects() {
    m_cbSubject->blockSignals(true);
    m_cbSubject->clear();
    m_cbSubject->addItem("Без привязки к предмету", -1);
    QSqlQuery q("SELECT id, name FROM subjects");
    while(q.next()) m_cbSubject->addItem(q.value(1).toString(), q.value(0).toInt());
    m_cbSubject->blockSignals(false);
}

void PomodoroWidget::toggleTimer() {
    if (m_isRunning) {
        int elapsedMinutes = m_elapsedSeconds / 60;
        qDebug() << "Остановка. elapsedSeconds:" << m_elapsedSeconds << "минут:" << elapsedMinutes;

        if (m_isWorkPhase && elapsedMinutes > 0) {
            saveSession(elapsedMinutes);
        } else {
            qDebug() << "Не сохраняем: не рабочая фаза или 0 минут";
        }

        m_timer->stop();
        m_isRunning = false;
        m_glWidget->setActive(false);
        m_elapsedSeconds = 0;

        m_btnStart->setText("Старт");
        m_btnStart->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; font-size: 16px; padding: 10px 20px; border-radius: 8px;");
    } else {
        m_activeSubjectId = m_cbSubject->currentData().toInt();
        qDebug() << "Старт с ID предмета:" << m_activeSubjectId;

        m_isWorkPhase = true;
        m_currentTotal = m_sbWork->value() * 60;
        m_timeLeft = m_currentTotal;
        m_elapsedSeconds = 0;

        m_timer->start(1000);
        m_isRunning = true;
        m_glWidget->setActive(true);
        m_glWidget->setTime(m_currentTotal, m_timeLeft, m_isWorkPhase);

        m_btnStart->setText("Стоп");
        m_btnStart->setStyleSheet("background-color: #E53935; color: white; font-weight: bold; font-size: 16px; padding: 10px 20px; border-radius: 8px;");
    }
}

void PomodoroWidget::updateTimer() {
    m_timeLeft--;
    if (m_isWorkPhase) m_elapsedSeconds++;

    m_glWidget->setTime(m_currentTotal, m_timeLeft, m_isWorkPhase);

    if (m_timeLeft <= 0) {
        if (m_isWorkPhase) {
            saveSession(m_sbWork->value());
            m_elapsedSeconds = 0;
            m_isWorkPhase = false;
            m_currentTotal = m_sbRest->value() * 60;
            m_timeLeft = m_currentTotal;
            QMessageBox::information(this, "Время отдыха", "Сессия работы завершена.");
        } else {
            m_isWorkPhase = true;
            m_currentTotal = m_sbWork->value() * 60;
            m_timeLeft = m_currentTotal;
            QMessageBox::information(this, "За работу", "Отдых окончен.");
        }
        m_glWidget->setTime(m_currentTotal, m_timeLeft, m_isWorkPhase);
    }
}