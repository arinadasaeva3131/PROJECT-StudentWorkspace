#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QSystemTrayIcon>
#include <QTimer>

class NotesWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void handleGoToNotes(int subjectId);
    void handleGoToStats(int subjectId);
    void checkNotifications();

private:
    QStackedWidget *m_stackedWidget;
    QButtonGroup *m_navGroup;
    NotesWidget *m_notesPage;

    QSystemTrayIcon *m_trayIcon;
    QTimer *m_notificationTimer;

    bool m_hwNotifiedToday; // Флаг, чтобы уведомление не спамило
};

#endif // MAINWINDOW_H