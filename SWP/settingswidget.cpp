#include "settingswidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QFrame>
#include <QTime>

SettingsWidget::SettingsWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(50, 50, 50, 50);
    layout->setSpacing(30);

    QLabel *title = new QLabel("Настройки программы", this);
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: #F5F5F5;");
    layout->addWidget(title);

    QFrame *schFrame = new QFrame(this);
    schFrame->setStyleSheet("QFrame { background-color: #1E1E2E; border-radius: 12px; padding: 20px; }");
    QVBoxLayout *schLayout = new QVBoxLayout(schFrame);

    QLabel *schTitle = new QLabel("Расписание", schFrame);
    schTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #FFFFBA; background: transparent;");
    schLayout->addWidget(schTitle);

    QCheckBox *oneWeekCb = new QCheckBox("Использовать однонедельный формат расписания (без чётности)", schFrame);
    oneWeekCb->setStyleSheet("font-size: 16px; color: #DDDDDD; background: transparent;");
    oneWeekCb->setChecked(DatabaseManager::instance().isOneWeekFormat());
    schLayout->addWidget(oneWeekCb);
    connect(oneWeekCb, &QCheckBox::toggled, this, [=](bool checked){ DatabaseManager::instance().setOneWeekFormat(checked); });
    layout->addWidget(schFrame);

    QFrame *notifFrame = new QFrame(this);
    notifFrame->setStyleSheet("QFrame { background-color: #1E1E2E; border-radius: 12px; padding: 20px; }");
    QVBoxLayout *notifLayout = new QVBoxLayout(notifFrame);

    QLabel *notifTitle = new QLabel("Уведомления и Цели", notifFrame);
    notifTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #FFFFBA; background: transparent;");
    notifLayout->addWidget(notifTitle);

    QCheckBox *remindCb = new QCheckBox("Ежедневное напоминание о Домашнем задании", notifFrame);
    remindCb->setStyleSheet("font-size: 16px; color: #DDDDDD; background: transparent;");
    remindCb->setChecked(DatabaseManager::instance().isHomeworkReminderEnabled());
    notifLayout->addWidget(remindCb);

    QString spinStyle = "QSpinBox { background: #2A2A35; color: white; border: 1px solid #555; border-radius: 6px; padding: 5px; font-size: 16px; }";

    QHBoxLayout *timeLayout = new QHBoxLayout();
    QLabel *timeLbl = new QLabel("Время напоминания:", notifFrame);
    timeLbl->setStyleSheet("font-size: 16px; color: #AAAAAA; background: transparent;");
    timeLayout->addWidget(timeLbl);

    QTime dbTime = DatabaseManager::instance().getHomeworkReminderTime();
    m_sbHour = new QSpinBox(notifFrame); m_sbHour->setRange(0, 23); m_sbHour->setValue(dbTime.hour()); m_sbHour->setStyleSheet(spinStyle);
    m_sbMin = new QSpinBox(notifFrame); m_sbMin->setRange(0, 59); m_sbMin->setValue(dbTime.minute()); m_sbMin->setStyleSheet(spinStyle);

    timeLayout->addWidget(m_sbHour);
    QLabel *colon1 = new QLabel(":", notifFrame); colon1->setStyleSheet("font-size: 18px; font-weight: bold; background: transparent;");
    timeLayout->addWidget(colon1);
    timeLayout->addWidget(m_sbMin);
    timeLayout->addStretch();
    notifLayout->addLayout(timeLayout);

    notifLayout->addSpacing(15);

    QHBoxLayout *goalLayout = new QHBoxLayout();
    QLabel *goalLbl = new QLabel("Ежедневная цель обучения:", notifFrame);
    goalLbl->setStyleSheet("font-size: 16px; color: #AAAAAA; background: transparent;");
    goalLayout->addWidget(goalLbl);

    int goalMinTotal = DatabaseManager::instance().getDailyGoalMin();
    QSpinBox *sbGoalHour = new QSpinBox(notifFrame); sbGoalHour->setRange(0, 24); sbGoalHour->setValue(goalMinTotal / 60); sbGoalHour->setStyleSheet(spinStyle);
    QSpinBox *sbGoalMin = new QSpinBox(notifFrame); sbGoalMin->setRange(0, 59); sbGoalMin->setValue(goalMinTotal % 60); sbGoalMin->setStyleSheet(spinStyle);

    goalLayout->addWidget(sbGoalHour);
    QLabel *lblH = new QLabel("ч.", notifFrame); lblH->setStyleSheet("font-size: 16px; color: #888; background: transparent;"); goalLayout->addWidget(lblH);
    goalLayout->addWidget(sbGoalMin);
    QLabel *lblM = new QLabel("мин.", notifFrame); lblM->setStyleSheet("font-size: 16px; color: #888; background: transparent;"); goalLayout->addWidget(lblM);
    goalLayout->addStretch();
    notifLayout->addLayout(goalLayout);

    layout->addWidget(notifFrame);

    connect(remindCb, &QCheckBox::toggled, this, [=](bool s){ DatabaseManager::instance().setHomeworkReminderEnabled(s); });
    connect(m_sbHour, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::saveTime);
    connect(m_sbMin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::saveTime);

    auto saveGoal = [=]() { DatabaseManager::instance().setDailyGoalMin(sbGoalHour->value() * 60 + sbGoalMin->value()); };
    connect(sbGoalHour, QOverload<int>::of(&QSpinBox::valueChanged), this, saveGoal);
    connect(sbGoalMin, QOverload<int>::of(&QSpinBox::valueChanged), this, saveGoal);

    layout->addStretch();
}

void SettingsWidget::saveTime() {
    DatabaseManager::instance().setHomeworkReminderTime(QTime(m_sbHour->value(), m_sbMin->value()));
}