#include "statisticswidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QSqlQuery>
#include <QDate>
#include <QDebug>

StatisticsWidget::StatisticsWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    QLabel *title = new QLabel("Статистика обучения", this);
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: #F5F5F5;");
    layout->addWidget(title);

    m_totalTodayLabel = new QLabel("Наработано сегодня: 0 мин.", this);
    m_totalTodayLabel->setStyleSheet("font-size: 20px; color: #BAE1FF; font-weight: bold;");
    layout->addWidget(m_totalTodayLabel);

    // Создаем шкалы
    auto addBarSection = [&](const QString &text, QProgressBar *&bar) {
        layout->addWidget(new QLabel(text, this));
        bar = createProgressBar();
        layout->addWidget(bar);
    };

    addBarSection("Прогресс за сегодня:", m_todayBar);
    addBarSection("Прогресс за неделю:", m_weekBar);
    addBarSection("Прогресс за месяц:", m_monthBar);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Фильтр по предмету:", this));
    m_cbSubject = new QComboBox(this);
    m_cbSubject->setStyleSheet("background: #2A2A35; color: white; padding: 6px; font-size: 16px; border-radius: 6px;");
    filterLayout->addWidget(m_cbSubject);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    connect(m_cbSubject, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::loadData);
    layout->addStretch();
}

QProgressBar* StatisticsWidget::createProgressBar() {
    QProgressBar *bar = new QProgressBar(this);
    bar->setFixedHeight(20);
    bar->setTextVisible(true);
    bar->setStyleSheet(
        "QProgressBar { background-color: #1A1A24; border: 1px solid #333; border-radius: 7px; color: white; text-align: center; }"
        "QProgressBar::chunk { background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4CAF50, stop:1 #BFFCC6); border-radius: 6px; }"
        );
    return bar;
}

void StatisticsWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    m_cbSubject->blockSignals(true);
    m_cbSubject->clear();
    m_cbSubject->addItem("Все предметы", -1);
    QSqlQuery q("SELECT id, name FROM subjects");
    while(q.next()) m_cbSubject->addItem(q.value(1).toString(), q.value(0).toInt());
    m_cbSubject->blockSignals(false);
    loadData(); // Данные обновятся при показе
}

void StatisticsWidget::loadData() {
    int subId = m_cbSubject->currentData().toInt();
    QDate today = QDate::currentDate();

    // Функция запроса с учетом ID предмета
    auto querySum = [&](const QDate &start, const QDate &end) -> int {
        QString sql = "SELECT SUM(work_duration_min) FROM pomodoro_sessions WHERE date BETWEEN ? AND ?";
        QSqlQuery q;

        if (subId != -1) {
            sql += " AND subject_id = ?";
            q.prepare(sql);
            q.addBindValue(start.toString(Qt::ISODate));
            q.addBindValue(end.toString(Qt::ISODate));
            q.addBindValue(subId);
        } else {
            q.prepare(sql);
            q.addBindValue(start.toString(Qt::ISODate));
            q.addBindValue(end.toString(Qt::ISODate));
        }

        if (q.exec() && q.next()) return q.value(0).toInt();
        return 0;
    };

    m_todayMin = querySum(today, today);
    m_weekMin  = querySum(today.addDays(-6), today);
    m_monthMin = querySum(today.addDays(-29), today);

    // Обновляем шкалы
    int goal = DatabaseManager::instance().getDailyGoalMin();

    // Сегодня
    m_todayBar->setMaximum(goal > 0 ? goal : 60);
    m_todayBar->setValue(m_todayMin);
    m_todayBar->setFormat(QString("%1 / %2 мин").arg(m_todayMin).arg(m_todayBar->maximum()));

    // Неделя (цель * 7)
    m_weekBar->setMaximum(goal > 0 ? goal * 7 : 420);
    m_weekBar->setValue(m_weekMin);
    m_weekBar->setFormat(QString("%1 / %2 мин").arg(m_weekMin).arg(m_weekBar->maximum()));

    // Месяц (цель * 30)
    m_monthBar->setMaximum(goal > 0 ? goal * 30 : 1800);
    m_monthBar->setValue(m_monthMin);
    m_monthBar->setFormat(QString("%1 / %2 мин").arg(m_monthMin).arg(m_monthBar->maximum()));

    m_totalTodayLabel->setText(QString("Наработано сегодня: %1 мин.").arg(m_todayMin));
}