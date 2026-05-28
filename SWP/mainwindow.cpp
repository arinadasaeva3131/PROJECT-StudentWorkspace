#include "mainwindow.h"
#include "mainpagewidget.h"
#include "schedulewidget.h"
#include "noteswidget.h"
#include "pomodorowidget.h"
#include "statisticswidget.h"
#include "settingswidget.h"
#include "databasemanager.h"
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QPushButton>
#include <QApplication>
#include <QTime>
#include <QPainter>
#include <QSqlQuery>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_hwNotifiedToday(false) {
    qApp->setStyleSheet(
        "QDialog { background-color: #1E1E2E; color: #F5F5F5; font-size: 15px; }"
        "QLabel { background: transparent; color: #F5F5F5; }"
        "QMessageBox QLabel { background: transparent; }"
        );

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #151515;");
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QFrame *sidebar = new QFrame(this);
    sidebar->setFixedWidth(240);
    sidebar->setStyleSheet("QFrame { background-color: #1A1A24; border-right: 1px solid #2D2D3D; }");
    QVBoxLayout *sbLayout = new QVBoxLayout(sidebar);
    sbLayout->setContentsMargins(10, 30, 10, 30);
    sbLayout->setSpacing(15);

    QLabel *logo = new QLabel("StudentSpace", this);
    logo->setStyleSheet("font-size: 22px; font-weight: bold; color: #FFFFBA; margin-bottom: 20px; background: transparent;");
    logo->setAlignment(Qt::AlignCenter);
    sbLayout->addWidget(logo);

    m_navGroup = new QButtonGroup(this);

    // ИСПРАВЛЕНИЕ 1: Вернули значки, переименовали таймер
    QStringList navItems = {"Настройки", "Главная", "Расписание", "Заметки", "Таймер", "Статистика"};
    QStringList icons = {"⚙️ ", "🏠 ", "📅 ", "📝 ", "⭐ ", "📊 "};

    for (int i = 0; i < navItems.size(); ++i) {
        QPushButton *btn = new QPushButton(icons[i] + navItems[i], this);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { font-size: 18px; padding: 12px 15px; text-align: left; background: transparent; color: #B0B0C0; border: none; border-radius: 8px; }"
            "QPushButton:hover { background: #2A2A35; color: #FFFFFF; }"
            "QPushButton:checked { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2D3748, stop:1 transparent); color: #FFFFBA; font-weight: bold; border-left: 4px solid #FFFFBA; }"
            );
        if (i == 1) btn->setChecked(true);
        sbLayout->addWidget(btn);
        m_navGroup->addButton(btn, i);
    }
    sbLayout->addStretch();

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(new SettingsWidget(this));
    MainPageWidget *main = new MainPageWidget(this);
    m_stackedWidget->addWidget(main);
    m_stackedWidget->addWidget(new ScheduleWidget(this));
    m_notesPage = new NotesWidget(this);
    m_stackedWidget->addWidget(m_notesPage);
    m_stackedWidget->addWidget(new PomodoroWidget(this));
    m_stackedWidget->addWidget(new StatisticsWidget(this));

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(m_stackedWidget);
    setCentralWidget(centralWidget);

    connect(m_navGroup, &QButtonGroup::idClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(main, &MainPageWidget::requestGoToNotes, this, &MainWindow::handleGoToNotes);
    connect(main, &MainPageWidget::requestGoToStats, this, &MainWindow::handleGoToStats);

    m_stackedWidget->setCurrentIndex(1);
    resize(1280, 800);

    m_trayIcon = new QSystemTrayIcon(this);
    QPixmap iconPix(32, 32); iconPix.fill(Qt::transparent);
    QPainter p(&iconPix);
    p.setFont(QFont("Arial", 20));
    p.drawText(iconPix.rect(), Qt::AlignCenter, "⭐");
    m_trayIcon->setIcon(QIcon(iconPix));
    m_trayIcon->show();

    m_notificationTimer = new QTimer(this);
    connect(m_notificationTimer, &QTimer::timeout, this, &MainWindow::checkNotifications);
    // ИСПРАВЛЕНИЕ 4: Проверяем каждые 10 секунд, чтобы точно не пропустить минуту
    m_notificationTimer->start(10000);
}

void MainWindow::checkNotifications() {
    QDateTime now = QDateTime::currentDateTime();

    // Уведомление о ДЗ
    if (DatabaseManager::instance().isHomeworkReminderEnabled()) {
        QTime hwTime = DatabaseManager::instance().getHomeworkReminderTime();
        if (now.time().hour() == hwTime.hour() && now.time().minute() == hwTime.minute()) {
            if (!m_hwNotifiedToday) {
                m_trayIcon->showMessage("Домашнее задание", "Самое время проверить и выполнить домашнее задание!", QSystemTrayIcon::Information, 10000);
                m_hwNotifiedToday = true;
            }
        } else {
            m_hwNotifiedToday = false; // Сбрасываем флаг, когда минута прошла
        }
    }

    // Уведомления перед парами
    QSqlQuery q;
    q.prepare("SELECT id, message FROM reminders WHERE is_completed = 0 AND datetime <= ?");
    q.addBindValue(now.toString(Qt::ISODate));
    if (q.exec()) {
        while (q.next()) {
            int id = q.value(0).toInt();
            QString msg = q.value(1).toString();
            m_trayIcon->showMessage("Расписание", msg, QSystemTrayIcon::Information, 10000);

            QSqlQuery u;
            u.prepare("UPDATE reminders SET is_completed = 1 WHERE id = ?");
            u.addBindValue(id);
            u.exec();
        }
    }
}

void MainWindow::handleGoToNotes(int id) {
    m_stackedWidget->setCurrentIndex(3);
    m_navGroup->button(3)->setChecked(true);
    m_notesPage->setSubjectFilter(id);
}

void MainWindow::handleGoToStats(int id) {
    Q_UNUSED(id);
    m_stackedWidget->setCurrentIndex(5);
    m_navGroup->button(5)->setChecked(true);
}