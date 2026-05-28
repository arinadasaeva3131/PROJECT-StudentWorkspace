#include "schedulewidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QTimeEdit>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDate>
#include <QSet>
#include <QMap>
#include <QMenu>
#include <QSpinBox>
#include <QInputDialog>
#include <QDebug>

ScheduleWidget::ScheduleWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(15);

    QHBoxLayout *topBar = new QHBoxLayout();

    m_labelCurrent = new QLabel("Текущая неделя: " +
                                    QString(DatabaseManager::instance().getCurrentWeekParity() == 0 ? "Чётная" : "Нечётная"), this);
    m_labelCurrent->setStyleSheet("color: #888; font-style: italic;");
    topBar->addWidget(m_labelCurrent);

    m_weekParityCombo = new QComboBox(this);
    m_weekParityCombo->addItems({"Отображать: Чётная", "Отображать: Нечётная"});
    m_weekParityCombo->setFixedWidth(160);
    topBar->addWidget(m_weekParityCombo);

    m_btnInvert = new QPushButton("Сменить чётность", this);
    topBar->addWidget(m_btnInvert);

    m_dateRangeLabel = new QLabel(this);
    m_dateRangeLabel->setStyleSheet("color: #888888; font-size: 13px; margin-left: 10px;");
    topBar->addWidget(m_dateRangeLabel);
    topBar->addStretch();

    QPushButton *btnAdd = new QPushButton("+ Добавить занятие / событие", this);
    btnAdd->setStyleSheet("QPushButton { background-color: #2D3748; color: #BAE1FF; font-weight: bold; padding: 8px 16px; border-radius: 6px; }"
                          "QPushButton:hover { background-color: #3A4A63; }");
    topBar->addWidget(btnAdd);
    rootLayout->addLayout(topBar);

    m_scheduleTable = new QTableWidget(7, 8, this);
    m_scheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_scheduleTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_scheduleTable->verticalHeader()->setVisible(false);

    m_scheduleTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_scheduleTable->verticalHeader()->setMinimumSectionSize(65);
    m_scheduleTable->setWordWrap(true);

    m_scheduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    for (int i = 1; i <= 7; ++i) {
        m_scheduleTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    for (int i = 0; i < 7; ++i) {
        m_scheduleTable->verticalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    m_scheduleTable->verticalHeader()->setMinimumSectionSize(65);

    m_scheduleTable->setStyleSheet(
        "QTableWidget { background-color: #151515; gridline-color: #2D2D2D; color: #F5F5F5; }"
        "QHeaderView::section { background-color: #252525; color: #BAE1FF; font-weight: bold; padding: 6px; border: 1px solid #121212; cursor: pointer; }"
        );

    rootLayout->addWidget(m_scheduleTable);

    connect(m_scheduleTable, &QTableWidget::cellClicked, this, &ScheduleWidget::handleCellClick);
    connect(m_scheduleTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &ScheduleWidget::handleHeaderClick);

    connect(m_btnInvert, &QPushButton::clicked, this, [=](){
        DatabaseManager::instance().toggleParityInversion();
        int newParity = DatabaseManager::instance().getCurrentWeekParity();
        m_labelCurrent->setText("Текущая неделя: " + QString(newParity == 0 ? "Чётная" : "Нечётная"));
        m_weekParityCombo->blockSignals(true);
        if (newParity != 2) m_weekParityCombo->setCurrentIndex(newParity);
        m_weekParityCombo->blockSignals(false);
        loadSchedule();
    });

    connect(m_weekParityCombo, &QComboBox::currentIndexChanged, this, &ScheduleWidget::loadSchedule);
    connect(btnAdd, &QPushButton::clicked, this, [this](){ addScheduleItem(-1, -1); });

    loadSchedule();
}

void ScheduleWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    bool isOneWeek = DatabaseManager::instance().isOneWeekFormat();
    m_weekParityCombo->setVisible(!isOneWeek);
    m_btnInvert->setVisible(!isOneWeek);

    int currentParity = DatabaseManager::instance().getCurrentWeekParity();
    if (currentParity != 2) {
        m_weekParityCombo->blockSignals(true);
        m_weekParityCombo->setCurrentIndex(currentParity);
        m_weekParityCombo->blockSignals(false);
    }
    m_labelCurrent->setText("Текущая неделя: " + QString(currentParity == 2 ? "Одна неделя" : (currentParity == 0 ? "Чётная" : "Нечётная")));
    loadSchedule();
}

void ScheduleWidget::loadSchedule() {
    m_scheduleTable->setRowCount(7);

    QStringList fixedClassTimes = {
        "1 пара\n08:00 - 09:30", "2 пара\n09:40 - 11:10", "3 пара\n11:30 - 13:00",
        "4 пара\n13:20 - 14:50", "5 пара\n15:00 - 16:30", "6 пара\n16:40 - 18:10",
        "События\n(Разовые)"
    };

    for (int r = 0; r < 7; ++r) {
        QTableWidgetItem *timeCell = new QTableWidgetItem(fixedClassTimes[r]);
        timeCell->setTextAlignment(Qt::AlignCenter);
        timeCell->setFont(QFont("Segoe UI", 9, QFont::Bold));
        timeCell->setForeground(QColor(r == 6 ? "#FFFFBA" : "#BAE1FF"));
        if (r == 6) timeCell->setBackground(QColor("#2A2215"));
        m_scheduleTable->setItem(r, 0, timeCell);
    }

    for (int r = 0; r < 7; ++r) {
        for (int d = 1; d <= 7; ++d) {
            QTableWidgetItem *emptyItem = new QTableWidgetItem("");
            if (r == 6) emptyItem->setBackground(QColor("#1A150E"));
            m_scheduleTable->setItem(r, d, emptyItem);
        }
    }

    int targetParity = m_weekParityCombo->currentIndex();
    QDate today = QDate::currentDate();
    int currentParity = DatabaseManager::instance().getCurrentWeekParity();

    int daysToMonday = today.dayOfWeek() - 1;
    QDate targetMonday = today.addDays(-daysToMonday);

    if (currentParity != 2 && targetParity != currentParity) {
        targetMonday = targetMonday.addDays(7);
    }
    QDate targetSunday = targetMonday.addDays(6);

    m_dateRangeLabel->setText(QString("(%1 — %2)").arg(targetMonday.toString("dd.MM.yyyy"), targetSunday.toString("dd.MM.yyyy")));

    QStringList shortDays = {"Время", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
    QStringList headers;
    headers << shortDays[0];
    for (int i = 1; i <= 7; ++i) {
        QDate dayDate = targetMonday.addDays(i - 1);
        headers << QString("%1\n(%2)").arg(shortDays[i], dayDate.toString("dd.MM"));
    }
    m_scheduleTable->setHorizontalHeaderLabels(headers);

    QMap<int, QMap<int, QVariantList>> cellIdMap;

    QSqlQuery qReg;
    qReg.prepare("SELECT s.id, s.start_time, s.day_of_week, subj.name, s.event_name, s.room, subj.color "
                 "FROM schedule s LEFT JOIN subjects subj ON s.subject_id = subj.id "
                 "WHERE s.is_event = 0 AND (s.week_parity = :parity OR s.week_parity = 2 OR :currentParity = 2)");
    qReg.bindValue(":parity", targetParity);
    qReg.bindValue(":currentParity", currentParity);

    if (qReg.exec()) {
        while (qReg.next()) {
            int id = qReg.value(0).toInt();
            QString start = qReg.value(1).toString().trimmed();
            int day = qReg.value(2).toInt();
            QString subjName = qReg.value(3).toString();
            QString evName = qReg.value(4).toString();
            QString room = qReg.value(5).toString();
            QString colorHex = qReg.value(6).toString();

            int targetRow = -1;
            if (start == "08:00" || start == "8:00") targetRow = 0;
            else if (start == "09:40" || start == "9:40") targetRow = 1;
            else if (start == "11:30") targetRow = 2;
            else if (start == "13:20") targetRow = 3;
            else if (start == "15:00") targetRow = 4;
            else if (start == "16:40") targetRow = 5;

            if (targetRow == -1 || day < 1 || day > 7) continue;

            QTableWidgetItem *item = m_scheduleTable->item(targetRow, day);
            QString content = evName.isEmpty() ? subjName : QString("%1\n(%2)").arg(subjName, evName);
            if (!room.isEmpty()) content += QString(" [%1]").arg(room);

            QString oldText = item->text();
            item->setText(oldText.isEmpty() ? content : oldText + "\n┈┈┈┈┈┈┈┈┈┈\n" + content);
            item->setTextAlignment(Qt::AlignCenter);

            if (!colorHex.isEmpty()) {
                item->setBackground(QColor(colorHex));
                item->setForeground(QColor("#151515"));
                item->setFont(QFont("Segoe UI", 9, QFont::Bold));
            }

            cellIdMap[targetRow][day].append(id);
            item->setData(Qt::UserRole, cellIdMap[targetRow][day]);
        }
    }

    QSqlQuery qEv;
    qEv.prepare("SELECT s.id, s.start_time, s.end_time, s.event_date, subj.name, s.event_name, s.room "
                "FROM schedule s LEFT JOIN subjects subj ON s.subject_id = subj.id "
                "WHERE s.is_event = 1 AND s.event_date BETWEEN ? AND ?");
    qEv.addBindValue(targetMonday.toString(Qt::ISODate));
    qEv.addBindValue(targetSunday.toString(Qt::ISODate));

    if (qEv.exec()) {
        while (qEv.next()) {
            int id = qEv.value(0).toInt();
            QString start = qEv.value(1).toString();
            QString end = qEv.value(2).toString();
            QDate evDate = QDate::fromString(qEv.value(3).toString(), Qt::ISODate);
            QString subjName = qEv.value(4).toString();
            QString evName = qEv.value(5).toString();
            QString room = qEv.value(6).toString();

            int day = evDate.dayOfWeek();
            if (day < 1 || day > 7) continue;

            QTableWidgetItem *item = m_scheduleTable->item(6, day);
            item->setBackground(QColor("#2A2215"));
            item->setForeground(QColor("#FFFFBA"));
            item->setFont(QFont("Segoe UI", 9, QFont::Bold));

            QString label = evName.isEmpty() ? subjName : evName;
            QString details = QString("• %1-%2: %3").arg(start, end, label);
            if (!room.isEmpty()) details += QString(" (%1)").arg(room);

            QString oldText = item->text();
            item->setText(oldText.isEmpty() ? details : oldText + "\n" + details);
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

            cellIdMap[6][day].append(id);
            item->setData(Qt::UserRole, cellIdMap[6][day]);
        }
    }
}

void ScheduleWidget::handleCellClick(int row, int column) {
    if (column == 0) return;

    QTableWidgetItem *item = m_scheduleTable->item(row, column);
    QVariantList ids = item ? item->data(Qt::UserRole).toList() : QVariantList();

    QMenu menu(this);
    menu.setStyleSheet("QMenu { background-color: #2D2D30; color: white; border: 1px solid #444; padding: 4px; }");

    QAction *addAct = menu.addAction(ids.isEmpty() ? "Добавить занятие сюда" : "Добавить ещё занятие сюда");

    if (!ids.isEmpty()) {
        menu.addSeparator();
        for (const auto& vId : ids) {
            int bid = vId.toInt();
            QSqlQuery nameQ;
            nameQ.prepare("SELECT s.event_name, s.start_time, sub.name FROM schedule s LEFT JOIN subjects sub ON s.subject_id = sub.id WHERE s.id = ?");
            nameQ.addBindValue(bid);

            QString label = "Элемент расписания";
            if (nameQ.exec() && nameQ.next()) {
                QString ev = nameQ.value(0).toString();
                label = QString("[%1] %2").arg(nameQ.value(1).toString(), ev.isEmpty() ? nameQ.value(2).toString() : ev);
            }

            QMenu *subMenu = (ids.size() > 1) ? menu.addMenu(label) : &menu;
            if (ids.size() > 1) subMenu->setStyleSheet("QMenu { background-color: #2D2D30; color: white; border: 1px solid #444; }");

            QAction *actRemind = subMenu->addAction("Включить напоминание");
            QAction *actEdit = subMenu->addAction("Редактировать");
            QAction *actDel = subMenu->addAction("Удалить");

            connect(actRemind, &QAction::triggered, this, [this, bid]() { addReminderForEvent(bid); });
            connect(actEdit, &QAction::triggered, this, [this, bid]() { editScheduleItem(bid); });
            connect(actDel, &QAction::triggered, this, [this, bid]() {
                if (QMessageBox::question(this, "Удаление", "Удалить эту запись?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    QSqlQuery q; q.prepare("DELETE FROM schedule WHERE id = ?"); q.addBindValue(bid);
                    if (q.exec()) loadSchedule();
                }
            });
        }
    }

    if (menu.exec(QCursor::pos()) == addAct) {
        addScheduleItem(column, row);
    }
}

void ScheduleWidget::handleHeaderClick(int index) {
    if (index == 0) return;
    QStringList days = {"", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"};
    QMenu menu(this);
    menu.setStyleSheet("QMenu { background-color: #2D2D30; color: white; border: 1px solid #444; }");
    QAction *addAct = menu.addAction("Добавить элемент на " + days[index]);
    QAction *clearAct = menu.addAction("Очистить день (Удалить пары)");

    QAction *selected = menu.exec(QCursor::pos());
    if (selected == addAct) {
        addScheduleItem(index, -1);
    } else if (selected == clearAct) {
        if (QMessageBox::question(this, "Очистить", "Удалить все регулярные пары на этот день?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QSqlQuery q; q.prepare("DELETE FROM schedule WHERE day_of_week = ? AND is_event = 0"); q.addBindValue(index);
            if (q.exec()) loadSchedule();
        }
    }
}

void ScheduleWidget::addScheduleItem(int prefillDay, int prefillRow) {
    QDialog dlg(this);
    dlg.setWindowTitle("Добавить элемент");
    dlg.setStyleSheet("QDialog { background-color: #2D2D30; color: #F5F5F5; font-size: 14px; }"
                      "QLineEdit, QComboBox, QSpinBox { background-color: #1E1E1E; color: white; padding: 6px; border: 1px solid #555; border-radius: 4px; }");

    QFormLayout *layout = new QFormLayout(&dlg);
    layout->setContentsMargins(25, 25, 25, 25);
    layout->setSpacing(12);

    QComboBox *cbType = new QComboBox(&dlg);
    cbType->addItems({"Повторяющееся занятие (Вуз)", "Однократное событие (Дата)"});
    layout->addRow("Тип записи:", cbType);

    QComboBox *cbSubject = new QComboBox(&dlg);
    cbSubject->addItem("Нет (Общее)", -1);
    QSqlQuery sq("SELECT id, name FROM subjects");
    while (sq.next()) cbSubject->addItem(sq.value(1).toString(), sq.value(0).toInt());
    layout->addRow("Предмет:", cbSubject);

    QLineEdit *leName = new QLineEdit(&dlg);
    layout->addRow("Название занятия:", leName);

    QComboBox *cbDay = new QComboBox(&dlg);
    cbDay->addItems({"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"});
    cbDay->setCurrentIndex(prefillDay >= 1 ? prefillDay - 1 : QDate::currentDate().dayOfWeek() - 1);

    QComboBox *cbParity = new QComboBox(&dlg);
    cbParity->addItems({"Чётная неделя", "Нечётная неделя", "Обе недели"});
    if (m_weekParityCombo->currentIndex() <= 1) cbParity->setCurrentIndex(m_weekParityCombo->currentIndex());

    QComboBox *cbTimeSlot = new QComboBox(&dlg);
    cbTimeSlot->addItems({"1 пара (08:00 - 09:30)", "2 пара (09:40 - 11:10)", "3 пара (11:30 - 13:00)", "4 пара (13:20 - 14:50)", "5 пара (15:00 - 16:30)", "6 пара (16:40 - 18:10)"});

    QDateEdit *deDate = new QDateEdit(QDate::currentDate(), &dlg);
    deDate->setCalendarPopup(true);
    deDate->setDisplayFormat("dd.MM.yyyy");

    if (prefillRow >= 0 && prefillRow <= 5) {
        cbType->setCurrentIndex(0);
        cbTimeSlot->setCurrentIndex(prefillRow);
    } else if (prefillRow == 6) {
        cbType->setCurrentIndex(1);
        int daysToMon = QDate::currentDate().dayOfWeek() - 1;
        QDate monday = QDate::currentDate().addDays(-daysToMon);
        if (m_weekParityCombo->currentIndex() != DatabaseManager::instance().getCurrentWeekParity() && DatabaseManager::instance().getCurrentWeekParity() != 2) {
            monday = monday.addDays(7);
        }
        deDate->setDate(monday.addDays(prefillDay >= 1 ? prefillDay - 1 : QDate::currentDate().dayOfWeek() - 1));
    }

    QSpinBox *sbStartHour = new QSpinBox(&dlg); sbStartHour->setRange(0,23); sbStartHour->setValue(12);
    QSpinBox *sbStartMin = new QSpinBox(&dlg); sbStartMin->setRange(0,59); sbStartMin->setValue(0);
    QWidget *wStartTime = new QWidget(&dlg); QHBoxLayout *lStart = new QHBoxLayout(wStartTime); lStart->setContentsMargins(0,0,0,0);
    lStart->addWidget(sbStartHour); lStart->addWidget(new QLabel("ч :")); lStart->addWidget(sbStartMin);

    QSpinBox *sbEndHour = new QSpinBox(&dlg); sbEndHour->setRange(0,23); sbEndHour->setValue(13);
    QSpinBox *sbEndMin = new QSpinBox(&dlg); sbEndMin->setRange(0,59); sbEndMin->setValue(30);
    QWidget *wEndTime = new QWidget(&dlg); QHBoxLayout *lEnd = new QHBoxLayout(wEndTime); lEnd->setContentsMargins(0,0,0,0);
    lEnd->addWidget(sbEndHour); lEnd->addWidget(new QLabel("ч :")); lEnd->addWidget(sbEndMin);

    // СОЗДАЕМ ЯВНЫЕ МЕТКИ, ЧТОБЫ ИЗБЕЖАТЬ ОШИБКИ labelForField
    QLabel *lblDay = new QLabel("День недели:", &dlg);         layout->addRow(lblDay, cbDay);
    QLabel *lblParity = new QLabel("Периодичность:", &dlg);    layout->addRow(lblParity, cbParity);
    QLabel *lblTimeSlot = new QLabel("Выбор пары:", &dlg);     layout->addRow(lblTimeSlot, cbTimeSlot);
    QLabel *lblDate = new QLabel("Дата события:", &dlg);       layout->addRow(lblDate, deDate);
    QLabel *lblStart = new QLabel("Время начала:", &dlg);      layout->addRow(lblStart, wStartTime);
    QLabel *lblEnd = new QLabel("Время конца:", &dlg);         layout->addRow(lblEnd, wEndTime);

    QLineEdit *leRoom = new QLineEdit(&dlg);
    layout->addRow("Аудитория:", leRoom);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(btnBox);

    auto toggleFields = [=](int idx) {
        bool isReg = (idx == 0);
        lblDay->setVisible(isReg);       cbDay->setVisible(isReg);
        lblParity->setVisible(isReg);    cbParity->setVisible(isReg);
        lblTimeSlot->setVisible(isReg);  cbTimeSlot->setVisible(isReg);
        lblDate->setVisible(!isReg);     deDate->setVisible(!isReg);
        lblStart->setVisible(!isReg);    wStartTime->setVisible(!isReg);
        lblEnd->setVisible(!isReg);      wEndTime->setVisible(!isReg);
    };
    connect(cbType, &QComboBox::currentIndexChanged, this, toggleFields);
    toggleFields(cbType->currentIndex());

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QSqlQuery q;
        q.prepare("INSERT INTO schedule (subject_id, event_name, day_of_week, week_parity, start_time, end_time, room, is_event, event_date) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

        int subId = cbSubject->currentData().toInt();
        q.addBindValue(subId == -1 ? QVariant() : subId);
        q.addBindValue(leName->text().trimmed().isEmpty() ? QVariant() : leName->text().trimmed());

        if (cbType->currentIndex() == 0) {
            QStringList starts = {"08:00", "09:40", "11:30", "13:20", "15:00", "16:40"};
            QStringList ends   = {"09:30", "11:10", "13:00", "14:50", "16:30", "18:10"};
            int slot = cbTimeSlot->currentIndex();
            q.addBindValue(cbDay->currentIndex() + 1);
            q.addBindValue(cbParity->currentIndex());
            q.addBindValue(starts[slot]);
            q.addBindValue(ends[slot]);
            q.addBindValue(leRoom->text().trimmed());
            q.addBindValue(0);
            q.addBindValue(QVariant());
        } else {
            QString startT = QString("%1:%2").arg(sbStartHour->value(), 2, 10, QChar('0')).arg(sbStartMin->value(), 2, 10, QChar('0'));
            QString endT = QString("%1:%2").arg(sbEndHour->value(), 2, 10, QChar('0')).arg(sbEndMin->value(), 2, 10, QChar('0'));
            q.addBindValue(QVariant());
            q.addBindValue(QVariant());
            q.addBindValue(startT);
            q.addBindValue(endT);
            q.addBindValue(leRoom->text().trimmed());
            q.addBindValue(1);
            q.addBindValue(deDate->date().toString(Qt::ISODate));
        }
        if (q.exec()) loadSchedule();
    }
}

void ScheduleWidget::editScheduleItem(int id) {
    QSqlQuery selectQ;
    selectQ.prepare("SELECT subject_id, event_name, day_of_week, week_parity, start_time, end_time, room, is_event, event_date FROM schedule WHERE id = ?");
    selectQ.addBindValue(id);
    if (!selectQ.exec() || !selectQ.next()) return;

    int dbSubId = selectQ.value(0).isNull() ? -1 : selectQ.value(0).toInt();
    QString dbEventName = selectQ.value(1).toString();
    int dbDay = selectQ.value(2).toInt();
    int dbParity = selectQ.value(3).toInt();
    QString dbStartStr = selectQ.value(4).toString();
    QString dbEndStr = selectQ.value(5).toString();
    QString dbRoom = selectQ.value(6).toString();
    int dbIsEvent = selectQ.value(7).toInt();
    QDate dbDate = QDate::fromString(selectQ.value(8).toString(), Qt::ISODate);

    QDialog dlg(this);
    dlg.setWindowTitle("Редактировать запись");
    dlg.setStyleSheet("QDialog { background-color: #2D2D30; color: #F5F5F5; font-size: 14px; }"
                      "QLineEdit, QComboBox, QSpinBox { background-color: #1E1E1E; color: white; padding: 6px; border: 1px solid #555; border-radius: 4px; }");

    QFormLayout *layout = new QFormLayout(&dlg);
    layout->setContentsMargins(25, 25, 25, 25);
    layout->setSpacing(12);

    QComboBox *cbSubject = new QComboBox(&dlg);
    cbSubject->addItem("Нет (Общее)", -1);
    QSqlQuery sq("SELECT id, name FROM subjects");
    while (sq.next()) cbSubject->addItem(sq.value(1).toString(), sq.value(0).toInt());
    cbSubject->setCurrentIndex(cbSubject->findData(dbSubId));
    layout->addRow("Предмет:", cbSubject);

    QLineEdit *leName = new QLineEdit(&dlg); leName->setText(dbEventName);
    layout->addRow("Название занятия:", leName);

    QComboBox *cbDay = new QComboBox(&dlg); cbDay->addItems({"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"});
    if (dbDay > 0) cbDay->setCurrentIndex(dbDay - 1);

    QComboBox *cbParity = new QComboBox(&dlg); cbParity->addItems({"Чётная неделя", "Нечётная неделя", "Обе недели"});
    cbParity->setCurrentIndex(dbParity);

    QComboBox *cbTimeSlot = new QComboBox(&dlg);
    cbTimeSlot->addItems({"1 пара (08:00 - 09:30)", "2 пара (09:40 - 11:10)", "3 пара (11:30 - 13:00)", "4 пара (13:20 - 14:50)", "5 пара (15:00 - 16:30)", "6 пара (16:40 - 18:10)"});
    int slotIdx = 0;
    if (dbStartStr.startsWith("08:") || dbStartStr.startsWith("8:")) slotIdx = 0;
    else if (dbStartStr.startsWith("09:") || dbStartStr.startsWith("9:")) slotIdx = 1;
    else if (dbStartStr.startsWith("11:")) slotIdx = 2;
    else if (dbStartStr.startsWith("13:")) slotIdx = 3;
    else if (dbStartStr.startsWith("15:")) slotIdx = 4;
    else if (dbStartStr.startsWith("16:")) slotIdx = 5;
    cbTimeSlot->setCurrentIndex(slotIdx);

    QDateEdit *deDate = new QDateEdit(dbDate.isValid() ? dbDate : QDate::currentDate(), &dlg); deDate->setCalendarPopup(true);

    QStringList sParts = dbStartStr.split(":"), eParts = dbEndStr.split(":");
    int sh = sParts.size() > 0 ? sParts[0].toInt() : 12, sm = sParts.size() > 1 ? sParts[1].toInt() : 0;
    int eh = eParts.size() > 0 ? eParts[0].toInt() : 13, em = eParts.size() > 1 ? eParts[1].toInt() : 30;

    QSpinBox *sbStartHour = new QSpinBox(&dlg); sbStartHour->setRange(0,23); sbStartHour->setValue(sh);
    QSpinBox *sbStartMin = new QSpinBox(&dlg); sbStartMin->setRange(0,59); sbStartMin->setValue(sm);
    QWidget *wStartTime = new QWidget(&dlg); QHBoxLayout *lS = new QHBoxLayout(wStartTime); lS->setContentsMargins(0,0,0,0);
    lS->addWidget(sbStartHour); lS->addWidget(new QLabel("ч :")); lS->addWidget(sbStartMin);

    QSpinBox *sbEndHour = new QSpinBox(&dlg); sbEndHour->setRange(0,23); sbEndHour->setValue(eh);
    QSpinBox *sbEndMin = new QSpinBox(&dlg); sbEndMin->setRange(0,59); sbEndMin->setValue(em);
    QWidget *wEndTime = new QWidget(&dlg); QHBoxLayout *lE = new QHBoxLayout(wEndTime); lE->setContentsMargins(0,0,0,0);
    lE->addWidget(sbEndHour); lE->addWidget(new QLabel("ч :")); lE->addWidget(sbEndMin);

    // СОЗДАЕМ ЯВНЫЕ МЕТКИ И ЗДЕСЬ
    QLabel *lblDay = new QLabel("День недели:", &dlg);         layout->addRow(lblDay, cbDay);
    QLabel *lblParity = new QLabel("Периодичность:", &dlg);    layout->addRow(lblParity, cbParity);
    QLabel *lblTimeSlot = new QLabel("Выбор пары:", &dlg);     layout->addRow(lblTimeSlot, cbTimeSlot);
    QLabel *lblDate = new QLabel("Дата события:", &dlg);       layout->addRow(lblDate, deDate);
    QLabel *lblStart = new QLabel("Время начала:", &dlg);      layout->addRow(lblStart, wStartTime);
    QLabel *lblEnd = new QLabel("Время конца:", &dlg);         layout->addRow(lblEnd, wEndTime);

    if (dbIsEvent == 0) {
        lblDate->hide(); deDate->hide();
        lblStart->hide(); wStartTime->hide();
        lblEnd->hide(); wEndTime->hide();
    } else {
        lblDay->hide(); cbDay->hide();
        lblParity->hide(); cbParity->hide();
        lblTimeSlot->hide(); cbTimeSlot->hide();
    }

    QLineEdit *leRoom = new QLineEdit(&dlg); leRoom->setText(dbRoom);
    layout->addRow("Аудитория:", leRoom);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QSqlQuery updateQ;
        updateQ.prepare("UPDATE schedule SET subject_id = ?, event_name = ?, day_of_week = ?, week_parity = ?, start_time = ?, end_time = ?, room = ?, event_date = ? WHERE id = ?");
        int subId = cbSubject->currentData().toInt();
        updateQ.addBindValue(subId == -1 ? QVariant() : subId);
        updateQ.addBindValue(leName->text().trimmed());

        if (dbIsEvent == 0) {
            QStringList starts = {"08:00", "09:40", "11:30", "13:20", "15:00", "16:40"};
            QStringList ends   = {"09:30", "11:10", "13:00", "14:50", "16:30", "18:10"};
            int slot = cbTimeSlot->currentIndex();
            updateQ.addBindValue(cbDay->currentIndex() + 1);
            updateQ.addBindValue(cbParity->currentIndex());
            updateQ.addBindValue(starts[slot]);
            updateQ.addBindValue(ends[slot]);
            updateQ.addBindValue(leRoom->text().trimmed());
            updateQ.addBindValue(QVariant());
        } else {
            QString startT = QString("%1:%2").arg(sbStartHour->value(), 2, 10, QChar('0')).arg(sbStartMin->value(), 2, 10, QChar('0'));
            QString endT   = QString("%1:%2").arg(sbEndHour->value(), 2, 10, QChar('0')).arg(sbEndMin->value(), 2, 10, QChar('0'));
            updateQ.addBindValue(QVariant());
            updateQ.addBindValue(QVariant());
            updateQ.addBindValue(startT);
            updateQ.addBindValue(endT);
            updateQ.addBindValue(leRoom->text().trimmed());
            updateQ.addBindValue(deDate->date().toString(Qt::ISODate));
        }
        updateQ.addBindValue(id);
        if (updateQ.exec()) loadSchedule();
    }
}

void ScheduleWidget::addReminderForEvent(int eventId) {
    QSqlQuery q;
    q.prepare("SELECT event_name, start_time, event_date, is_event, day_of_week, room, sub.name "
              "FROM schedule s LEFT JOIN subjects sub ON s.subject_id = sub.id WHERE s.id = ?");
    q.addBindValue(eventId);
    if (!q.exec() || !q.next()) return;

    QString eventName = q.value(0).toString();
    QString startTimeStr = q.value(1).toString();
    QDate date = QDate::fromString(q.value(2).toString(), Qt::ISODate);
    int isEvent = q.value(3).toInt();
    int dayOfWeek = q.value(4).toInt();
    QString room = q.value(5).toString();
    QString subjectName = q.value(6).toString();

    QString title = eventName.isEmpty() ? subjectName : eventName;
    if (title.isEmpty()) title = "Учебное занятие";

    if (isEvent == 0) {
        date = QDate::currentDate();
        while (date.dayOfWeek() != dayOfWeek) {
            date = date.addDays(1);
        }
    }

    QTime startTime = QTime::fromString(startTimeStr, "HH:mm");
    QDateTime eventDateTime(date, startTime);

    QStringList options = {"В момент начала", "За 5 минут", "За 10 минут", "За 15 минут", "За 30 минут"};
    bool ok;
    QString item = QInputDialog::getItem(this, "Настройка уведомления",
                                         QString("Укажите, когда напомнить о:\n%1 в %2").arg(title, startTimeStr),
                                         options, 0, false, &ok);
    if (!ok) return;

    int minutesBefore = 0;
    if (item == "За 5 минут") minutesBefore = 5;
    else if (item == "За 10 минут") minutesBefore = 10;
    else if (item == "За 15 минут") minutesBefore = 15;
    else if (item == "За 30 минут") minutesBefore = 30;

    QDateTime reminderDateTime = eventDateTime.addSecs(-minutesBefore * 60);

    if (reminderDateTime < QDateTime::currentDateTime()) {
        QMessageBox::warning(this, "Время упущено", "Выбранный интервал напоминания уже остался в прошлом!");
        return;
    }

    QString msg = QString("Скоро начнётся: %1").arg(title);
    if (!room.isEmpty()) msg += QString(" (ауд. %1)").arg(room);
    msg += QString(" в %1").arg(startTimeStr);

    QSqlQuery insQ;
    insQ.prepare("INSERT INTO reminders (datetime, message, is_completed) VALUES (?, ?, 0)");
    insQ.addBindValue(reminderDateTime.toString(Qt::ISODate));
    insQ.addBindValue(msg);

    if (insQ.exec()) {
        QMessageBox::information(this, "Успех",
                                 QString("Напоминание успешно взведено на %1!\nОно сработает автоматически за кулисами.").arg(reminderDateTime.toString("dd.MM.yyyy HH:mm")));
    }
}