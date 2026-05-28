#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QSystemTrayIcon>
#include <QTimer>

class ReminderWidget : public QWidget {
    Q_OBJECT
public:
    explicit ReminderWidget(QWidget *parent = nullptr);

private slots:
    void loadReminders();
    void addReminder();
    void checkReminders();

private:
    QListWidget *m_remindersList;
    QLineEdit *m_msgEdit;
    QDateTimeEdit *m_dtEdit;
    QSystemTrayIcon *m_trayIcon;
    QTimer *m_checkTimer;
};