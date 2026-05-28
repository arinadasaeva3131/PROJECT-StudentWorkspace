#include "reminderwidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSqlQuery>
#include <QDateTime>
#include <QStyle>

ReminderWidget::ReminderWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout *formLayout = new QHBoxLayout();
    m_msgEdit = new QLineEdit(this);
    m_msgEdit->setPlaceholderText("Текст напоминания...");
    m_dtEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_dtEdit->setCalendarPopup(true);

    QPushButton *btnAdd = new QPushButton("Добавить", this);
    formLayout->addWidget(m_msgEdit, 2);
    formLayout->addWidget(m_dtEdit, 1);
    formLayout->addWidget(btnAdd, 0);
    rootLayout->addLayout(formLayout);

    m_remindersList = new QListWidget(this);
    rootLayout->addWidget(m_remindersList);

    m_trayIcon = new QSystemTrayIcon(this);
    // Используем стандартную системную иконку Qt, чтобы не требовать ассет-файл
    m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    m_trayIcon->show();

    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(30000); // 30 секунд по ТЗ
    connect(m_checkTimer, &QTimer::timeout, this, &ReminderWidget::checkReminders);
    m_checkTimer->start();

    connect(btnAdd, &QPushButton::clicked, this, &ReminderWidget::addReminder);
    loadReminders();
}

void ReminderWidget::loadReminders() {
    m_remindersList->clear();
    QSqlQuery q("SELECT datetime, message, is_completed FROM reminders ORDER BY datetime ASC");
    while(q.next()) {
        QString dt = q.value(0).toString();
        QString msg = q.value(1).toString();
        int done = q.value(2).toInt();
        QString out = QString("[%1] %2 %3").arg(dt, msg, done ? "(Выполнено)" : "");
        m_remindersList->addItem(out);
    }
}

void ReminderWidget::addReminder() {
    if(m_msgEdit->text().trimmed().isEmpty()) return;

    QSqlQuery q;
    q.prepare("INSERT INTO reminders (datetime, message) VALUES (?, ?)");
    q.addBindValue(m_dtEdit->dateTime().toString(Qt::ISODate));
    q.addBindValue(m_msgEdit->text().trimmed());
    q.exec();

    m_msgEdit->clear();
    loadReminders();
}

void ReminderWidget::checkReminders() {
    QString nowStr = QDateTime::currentDateTime().toString(Qt::ISODate);
    QSqlQuery q;
    q.prepare("SELECT id, message FROM reminders WHERE datetime <= ? AND is_completed = 0");
    q.addBindValue(nowStr);

    if(q.exec()) {
        while(q.next()) {
            int id = q.value(0).toInt();
            QString msg = q.value(1).toString();

            m_trayIcon->showMessage("АРМ Студента", msg, QSystemTrayIcon::Information, 5000);

            QSqlQuery u;
            u.prepare("UPDATE reminders SET is_completed = 1 WHERE id = ?");
            u.addBindValue(id);
            u.exec();
        }
        loadReminders();
    }
}