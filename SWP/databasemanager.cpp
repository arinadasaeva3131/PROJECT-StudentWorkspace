#include "databasemanager.h"
#include <QSqlError>
#include <QDebug>
#include <QDate>

DatabaseManager::DatabaseManager() {}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initDatabase() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("student_workspace.db");
    if (!m_db.open()) {
        qDebug() << "Не удалось открыть БД:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;
    bool ok = true;

    ok &= query.exec("CREATE TABLE IF NOT EXISTS subjects ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "name TEXT UNIQUE, "
                     "color TEXT)");

    ok &= query.exec("CREATE TABLE IF NOT EXISTS schedule ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "subject_id INTEGER, "
                     "event_name TEXT, "
                     "day_of_week INTEGER, "
                     "week_parity INTEGER, "
                     "start_time TEXT, "
                     "end_time TEXT, "
                     "room TEXT, "
                     "is_event INTEGER DEFAULT 0, "
                     "event_date TEXT, "
                     "is_done INTEGER DEFAULT 0, "
                     "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE)");

    ok &= query.exec("CREATE TABLE IF NOT EXISTS notes ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "subject_id INTEGER, "
                     "title TEXT, "
                     "content_richtext TEXT, "
                     "created_date TEXT, "
                     "modified_date TEXT, "
                     "folder_id INTEGER DEFAULT -1, "
                     "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE)");

    // ========== ИСПРАВЛЕНИЕ: добавлена таблица pomodoro_sessions ==========
    ok &= query.exec("CREATE TABLE IF NOT EXISTS pomodoro_sessions ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "subject_id INTEGER, "
                     "work_duration_min INTEGER, "
                     "date TEXT)");

    ok &= query.exec("CREATE TABLE IF NOT EXISTS settings ("
                     "key TEXT PRIMARY KEY, "
                     "value TEXT)");

    ok &= query.exec("PRAGMA foreign_keys = ON;");

    if (!ok) {
        qDebug() << "Ошибка создания таблиц:" << query.lastError().text();
        return false;
    }

    return true;
}

// ========== ОСТАЛЬНЫЕ МЕТОДЫ (без изменений) ==========

int DatabaseManager::getCurrentWeekParity() const {
    if (isOneWeekFormat()) return 2;
    int baseParity = QDate::currentDate().weekNumber() % 2;
    if (isParityInverted()) return (baseParity == 0) ? 1 : 0;
    return baseParity;
}

void DatabaseManager::toggleParityInversion() {
    bool current = isParityInverted();
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES ('parity_inverted', ?)");
    q.addBindValue(!current ? 1 : 0);
    q.exec();
}

bool DatabaseManager::isParityInverted() const {
    QSqlQuery q("SELECT value FROM settings WHERE key = 'parity_inverted'");
    if (q.exec() && q.next()) return q.value(0).toInt() == 1;
    return false;
}

bool DatabaseManager::isOneWeekFormat() const {
    QSqlQuery q("SELECT value FROM settings WHERE key = 'one_week_format'");
    if (q.exec() && q.next()) return q.value(0).toInt() == 1;
    return false;
}

void DatabaseManager::setOneWeekFormat(bool isOneWeek) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES ('one_week_format', ?)");
    q.addBindValue(isOneWeek ? 1 : 0);
    q.exec();
}

bool DatabaseManager::isHomeworkReminderEnabled() const {
    QSqlQuery q("SELECT value FROM settings WHERE key = 'hw_remind_enabled'");
    if (q.exec() && q.next()) return q.value(0).toInt() == 1;
    return false;
}

void DatabaseManager::setHomeworkReminderEnabled(bool enabled) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES ('hw_remind_enabled', ?)");
    q.addBindValue(enabled ? 1 : 0);
    q.exec();
}

QTime DatabaseManager::getHomeworkReminderTime() const {
    QSqlQuery q("SELECT value FROM settings WHERE key = 'hw_remind_time'");
    if (q.exec() && q.next()) return QTime::fromString(q.value(0).toString(), "HH:mm");
    return QTime(18, 0);
}

void DatabaseManager::setHomeworkReminderTime(const QTime& time) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES ('hw_remind_time', ?)");
    q.addBindValue(time.toString("HH:mm"));
    q.exec();
}

int DatabaseManager::getDailyGoalMin() const {
    QSqlQuery q("SELECT value FROM settings WHERE key = 'daily_goal_min'");
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 120;
}

void DatabaseManager::setDailyGoalMin(int minutes) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES ('daily_goal_min', ?)");
    q.addBindValue(minutes);
    q.exec();
}