#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTime>

class DatabaseManager {
public:
    static DatabaseManager& instance();
    bool initDatabase();

    int getCurrentWeekParity() const;
    void toggleParityInversion();
    bool isParityInverted() const;

    bool isOneWeekFormat() const;
    void setOneWeekFormat(bool isOneWeek);

    bool isHomeworkReminderEnabled() const;
    void setHomeworkReminderEnabled(bool enabled);
    QTime getHomeworkReminderTime() const;
    void setHomeworkReminderTime(const QTime& time);

    // НОВОЕ: Цель обучения на день (в минутах)
    int getDailyGoalMin() const;
    void setDailyGoalMin(int minutes);

private:
    DatabaseManager();
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H