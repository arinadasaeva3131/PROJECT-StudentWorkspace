#include "mainpagewidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QMenu>
#include <QPainter>
#include <QDate>
#include <QTime>
#include <QLocale>
#include <QTimer>
#include <QDebug>

MainPageWidget::MainPageWidget(QWidget *parent)
    : QWidget(parent), m_selectedSubjectId(-1), m_selectedSubjectName(""), m_activeButton(nullptr)
{
    setupUI();
    loadData();

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainPageWidget::updateCurrentPairStatus);
    m_statusTimer->start(30000);
}

void MainPageWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    loadTodaySchedule();
    updateCurrentPairStatus();
}

void MainPageWidget::setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(40);

    // ========== ЛЕВАЯ КОЛОНКА ==========
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setAlignment(Qt::AlignTop);

    int hour = QTime::currentTime().hour();
    QString greetingText;
    if (hour >= 6 && hour < 12) greetingText = "Доброе утро!";
    else if (hour >= 12 && hour < 18) greetingText = "Добрый день!";
    else if (hour >= 18 && hour < 23) greetingText = "Добрый вечер!";
    else greetingText = "Доброй ночи!";

    QLabel *greetingLabel = new QLabel(greetingText, this);
    greetingLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #F5F5F5; margin-bottom: 5px;");
    leftLayout->addWidget(greetingLabel);

    QLabel *dateLabel = new QLabel(QLocale(QLocale::Russian).toString(QDate::currentDate(), "d MMMM, dddd"), this);
    dateLabel->setStyleSheet("font-size: 16px; color: #888888; margin-bottom: 15px; text-transform: capitalize;");
    leftLayout->addWidget(dateLabel);

    // ПАНЕЛЬ ФОКУСА ТЕКУЩЕЙ ПАРЫ
    m_focusStatusLabel = new QLabel("Сейчас нет занятий", this);
    m_focusStatusLabel->setStyleSheet("font-size: 15px; color: #FFFFBA; font-weight: bold;");

    m_focusProgressBar = new QProgressBar(this);
    m_focusProgressBar->setRange(0, 100);
    m_focusProgressBar->setValue(0);
    m_focusProgressBar->setFixedHeight(12);
    m_focusProgressBar->setTextVisible(false);
    m_focusProgressBar->setStyleSheet(
        "QProgressBar { background-color: #151515; border: 1px solid #333; border-radius: 6px; }"
        "QProgressBar::chunk { background-color: #BAE1FF; border-radius: 5px; }"
        );

    QVBoxLayout *focusPanel = new QVBoxLayout();
    focusPanel->setContentsMargins(15, 15, 15, 15);
    focusPanel->setSpacing(8);
    QLabel *focusTitle = new QLabel("ТЕКУЩИЙ СТАТУС", this);
    focusTitle->setStyleSheet("font-size: 11px; color: #888888; font-weight: bold; letter-spacing: 1px;");
    focusPanel->addWidget(focusTitle);
    focusPanel->addWidget(m_focusStatusLabel);
    focusPanel->addWidget(m_focusProgressBar);

    QFrame *focusFrame = new QFrame(this);
    focusFrame->setFrameShape(QFrame::StyledPanel);
    focusFrame->setStyleSheet("QFrame { background-color: #1A1A1A; border: 1px solid #2D2D2D; border-radius: 10px; }");
    focusFrame->setLayout(focusPanel);
    leftLayout->addWidget(focusFrame);
    leftLayout->addSpacing(20);

    // Раздел предметов
    QHBoxLayout *headerLeft = new QHBoxLayout();
    QLabel *titleLeft = new QLabel("Мои предметы", this);
    titleLeft->setStyleSheet("font-size: 22px; font-weight: bold; color: #DDDDDD;");
    headerLeft->addWidget(titleLeft);

    QPushButton *addBtn = new QPushButton("+ Добавить", this);
    addBtn->setFixedSize(120, 35);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet("QPushButton { background: transparent; color: #BAE1FF; border: 1px solid #BAE1FF; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: rgba(186,225,255,0.1); }");
    headerLeft->addWidget(addBtn);
    headerLeft->addStretch();
    leftLayout->addLayout(headerLeft);

    m_subjectsLayout = new QGridLayout();
    m_subjectsLayout->setSpacing(20);
    leftLayout->addLayout(m_subjectsLayout);

    // ========== ПРАВАЯ КОЛОНКА (Актуальное) ==========
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setAlignment(Qt::AlignTop);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(15);

    QString listStyle =
        "QListWidget { background: #151515; border: 1px solid #2D2D2D; border-radius: 12px; color: white; padding: 10px; font-size: 14px; outline: 0; }"
        "QListWidget::item { padding: 12px; border-bottom: 1px solid #252525; border-radius: 6px; margin-bottom: 4px; background: #1A1A1A; }"
        "QListWidget::item:hover { background: #222222; }"
        "QListWidget::item:selected { background: #2D3748; border-left: 4px solid #BAE1FF; }";

    QString emptyLabelStyle = "color: #777; font-size: 14px; border: 1px dashed #333; border-radius: 12px; padding: 20px; background: #121212;";

    // --- Секция 1: Сегодня ---
    QLabel *titleToday = new QLabel("Расписание на сегодня", this);
    titleToday->setStyleSheet("font-size: 20px; font-weight: bold; color: #F5F5F5; margin-top: 10px;");
    rightLayout->addWidget(titleToday);

    m_todayList = new QListWidget(this);
    m_todayList->setStyleSheet(listStyle);
    rightLayout->addWidget(m_todayList);

    m_todayEmptyLabel = new QLabel("На сегодня пар и событий нет! Можно отдыхать.", this);
    m_todayEmptyLabel->setStyleSheet(emptyLabelStyle);
    m_todayEmptyLabel->setAlignment(Qt::AlignCenter);
    m_todayEmptyLabel->hide();
    rightLayout->addWidget(m_todayEmptyLabel);

    // --- Секция 2: Ближайшие события ---
    QLabel *titleUpcoming = new QLabel("Ближайшие события (7 дней)", this);
    titleUpcoming->setStyleSheet("font-size: 20px; font-weight: bold; color: #F5F5F5; margin-top: 20px;");
    rightLayout->addWidget(titleUpcoming);

    m_upcomingList = new QListWidget(this);
    m_upcomingList->setStyleSheet(listStyle);
    rightLayout->addWidget(m_upcomingList);

    m_upcomingEmptyLabel = new QLabel("В ближайшую неделю событий не предвидится.", this);
    m_upcomingEmptyLabel->setStyleSheet(emptyLabelStyle);
    m_upcomingEmptyLabel->setAlignment(Qt::AlignCenter);
    m_upcomingEmptyLabel->hide();
    rightLayout->addWidget(m_upcomingEmptyLabel);

    mainLayout->addLayout(leftLayout, 60);
    mainLayout->addLayout(rightLayout, 40);

    connect(addBtn, &QPushButton::clicked, this, &MainPageWidget::addSubject);
    connect(m_todayList, &QListWidget::itemChanged, this, &MainPageWidget::toggleEventStatus);
    connect(m_upcomingList, &QListWidget::itemChanged, this, &MainPageWidget::toggleEventStatus);
}

void MainPageWidget::loadData() {
    loadSubjects();
    loadTodaySchedule();
}

void MainPageWidget::loadSubjects() {
    QLayoutItem *child;
    while ((child = m_subjectsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    m_activeButton = nullptr;
    m_selectedSubjectId = -1;
    m_selectedSubjectName = "";

    QSqlQuery q("SELECT id, name, color FROM subjects");
    int row = 0, col = 0;

    while (q.next()) {
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        QString color = q.value(2).toString();
        if (color.isEmpty()) color = "#BAE1FF";

        QPushButton *btn = new QPushButton(name, this);
        btn->setFixedSize(220, 100);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(
                               "QPushButton { background-color: %1; color: #151515; border-radius: 12px; font-size: 16px; font-weight: bold; border: 2px solid transparent; }"
                               "QPushButton:hover { border: 2px solid #FFFFFF; }"
                               ).arg(color));
        btn->setProperty("id", id);
        btn->setProperty("name", name);
        btn->setProperty("color", color);
        connect(btn, &QPushButton::clicked, this, &MainPageWidget::handleSubjectClick);

        m_subjectsLayout->addWidget(btn, row, col);

        col++;
        if (col > 2) { col = 0; row++; }
    }
}

void MainPageWidget::loadTodaySchedule() {
    m_todayList->blockSignals(true);
    m_upcomingList->blockSignals(true);
    m_todayList->clear();
    m_upcomingList->clear();

    QDate today = QDate::currentDate();
    int currentDayOfWeek = today.dayOfWeek();
    int currentParity = DatabaseManager::instance().getCurrentWeekParity();
    QString todayStr = today.toString(Qt::ISODate);

    // 1. ЗАГРУЖАЕМ СЕГОДНЯ
    QSqlQuery qToday;
    qToday.prepare("SELECT s.id, s.start_time, s.end_time, sub.name, s.is_done, s.event_name, s.is_event, s.room "
                   "FROM schedule s "
                   "LEFT JOIN subjects sub ON s.subject_id = sub.id "
                   "WHERE (s.is_event = 0 AND s.day_of_week = :day AND (s.week_parity = :parity OR s.week_parity = 2 OR :parity = 2)) "
                   "   OR (s.is_event = 1 AND s.event_date = :todayDate) "
                   "ORDER BY s.start_time ASC");

    qToday.bindValue(":day", currentDayOfWeek);
    qToday.bindValue(":parity", currentParity);
    qToday.bindValue(":todayDate", todayStr);

    bool hasToday = false;
    if (qToday.exec()) {
        while (qToday.next()) {
            hasToday = true;
            int eventId = qToday.value(0).toInt();
            QString startTime = qToday.value(1).toString();
            QString endTime = qToday.value(2).toString();
            QString subjectName = qToday.value(3).toString();
            int isDone = qToday.value(4).toInt();
            QString eventName = qToday.value(5).toString();
            int isEvent = qToday.value(6).toInt();
            QString room = qToday.value(7).toString();

            if (subjectName.isEmpty()) subjectName = "Общее";

            QString typeIcon = (isEvent == 1) ? "⭐ " : "📚 ";
            QString title = eventName.isEmpty() ? subjectName : (subjectName + " (" + eventName + ")");
            if (!room.isEmpty()) title += " [ауд. " + room + "]";

            QString displayText = QString("%1 %2 - %3  |  %4").arg(typeIcon, startTime, endTime, title);
            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, eventId);
            item->setData(Qt::UserRole + 1, startTime);
            item->setData(Qt::UserRole + 2, endTime);
            item->setData(Qt::UserRole + 3, title);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(isDone ? Qt::Checked : Qt::Unchecked);

            if (isDone) {
                QFont font = item->font(); font.setStrikeOut(true); item->setFont(font);
                item->setForeground(QColor("#666666"));
            } else {
                item->setForeground(QColor("#E0E0E0"));
            }
            m_todayList->addItem(item);
        }
    }
    if (hasToday) { m_todayList->show(); m_todayEmptyLabel->hide(); }
    else { m_todayList->hide(); m_todayEmptyLabel->show(); }

    // 2. ЗАГРУЖАЕМ БЛИЖАЙШИЕ СОБЫТИЯ
    QDate nextWeek = today.addDays(7);
    QSqlQuery qUpcoming;
    qUpcoming.prepare("SELECT s.id, s.start_time, sub.name, s.event_name, s.room, s.event_date "
                      "FROM schedule s "
                      "LEFT JOIN subjects sub ON s.subject_id = sub.id "
                      "WHERE s.is_event = 1 AND s.event_date > ? AND s.event_date <= ? AND s.is_done = 0 "
                      "ORDER BY s.event_date ASC, s.start_time ASC");
    qUpcoming.addBindValue(todayStr);
    qUpcoming.addBindValue(nextWeek.toString(Qt::ISODate));

    bool hasUpcoming = false;
    if (qUpcoming.exec()) {
        while (qUpcoming.next()) {
            hasUpcoming = true;
            int eventId = qUpcoming.value(0).toInt();
            QString timeStr = qUpcoming.value(1).toString();
            QString subjectName = qUpcoming.value(2).toString();
            QString eventName = qUpcoming.value(3).toString();
            QString room = qUpcoming.value(4).toString();
            QDate eDate = QDate::fromString(qUpcoming.value(5).toString(), Qt::ISODate);

            if (subjectName.isEmpty()) subjectName = "Общее";
            QString title = eventName.isEmpty() ? subjectName : (subjectName + " — " + eventName);
            if (!room.isEmpty()) title += " (Ауд. " + room + ")";

            QString displayText = QString("%1  |  %2  |  %3").arg(eDate.toString("dd.MM"), timeStr, title);
            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, eventId);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            item->setForeground(QColor("#E0E0E0"));
            m_upcomingList->addItem(item);
        }
    }
    if (hasUpcoming) { m_upcomingList->show(); m_upcomingEmptyLabel->hide(); }
    else { m_upcomingList->hide(); m_upcomingEmptyLabel->show(); }

    m_todayList->blockSignals(false);
    m_upcomingList->blockSignals(false);
}

void MainPageWidget::updateCurrentPairStatus() {
    QTime now = QTime::currentTime();
    bool pairFound = false;

    for (int i = 0; i < m_todayList->count(); ++i) {
        QListWidgetItem *item = m_todayList->item(i);
        QTime start = QTime::fromString(item->data(Qt::UserRole + 1).toString(), "HH:mm");
        QTime end = QTime::fromString(item->data(Qt::UserRole + 2).toString(), "HH:mm");
        QString title = item->data(Qt::UserRole + 3).toString();

        if (start.isValid() && end.isValid() && now >= start && now <= end) {
            pairFound = true;
            int totalSecs = start.secsTo(end);
            int passedSecs = start.secsTo(now);
            int remainingMins = (now.secsTo(end)) / 60;
            int percent = (passedSecs * 100) / totalSecs;

            m_focusStatusLabel->setText(QString("Идёт: %1\n⏱ Осталось: %2 мин.").arg(title).arg(remainingMins == 0 ? 1 : remainingMins));
            m_focusProgressBar->setValue(percent);

            item->setBackground(QColor("#252B35"));
            item->setFont(QFont("Segoe UI", 10, QFont::Bold));
        } else {
            item->setBackground(QColor("#1A1A1A"));
            item->setFont(QFont("Segoe UI", 10, QFont::Normal));
        }
    }

    if (!pairFound) {
        m_focusStatusLabel->setText("Сейчас пар нет. Время отдыхать! 🙌");
        m_focusProgressBar->setValue(0);
    }
}

void MainPageWidget::toggleEventStatus(QListWidgetItem *item) {
    int eventId = item->data(Qt::UserRole).toInt();
    bool isChecked = (item->checkState() == Qt::Checked);

    QSqlQuery q;
    q.prepare("UPDATE schedule SET is_done = ? WHERE id = ?");
    q.addBindValue(isChecked ? 1 : 0);
    q.addBindValue(eventId);
    if (!q.exec()) return;

    m_todayList->blockSignals(true);
    m_upcomingList->blockSignals(true);
    QFont font = item->font();
    font.setStrikeOut(isChecked);
    item->setFont(font);
    item->setForeground(isChecked ? QColor("#666666") : QColor("#E0E0E0"));
    m_todayList->blockSignals(false);
    m_upcomingList->blockSignals(false);

    QTimer::singleShot(300, this, &MainPageWidget::loadTodaySchedule);
}

QIcon MainPageWidget::createColorCircleIcon(const QString &hexColor) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(hexColor));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, 20, 20);
    return QIcon(pixmap);
}

void MainPageWidget::addSubject() {
    QDialog dialog(this);
    dialog.setWindowTitle("Добавить новый предмет");
    dialog.setStyleSheet("background-color: #2D2D30; color: #F5F5F5; font-size: 14px;");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    layout->addWidget(new QLabel("Название предмета:", &dialog));
    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setStyleSheet("background-color: #1E1E1E; color: white; padding: 8px; border: 1px solid #444; border-radius: 6px;");
    layout->addWidget(nameEdit);

    layout->addWidget(new QLabel("Выберите цвет плашки:", &dialog));
    QComboBox *colorCombo = new QComboBox(&dialog);
    colorCombo->setStyleSheet("QComboBox { background-color: #1E1E1E; color: white; padding: 8px; border: 1px solid #444; border-radius: 6px; }");

    QVector<QString> hexColors = {
        "#FFB3BA", "#FFDFBA", "#FFFFBA", "#BFFCC6", "#BAFFC9",
        "#BAE1FF", "#A8E6CF", "#DCEDC1", "#FFD3B6", "#FFAAA6"
    };
    QStringList colorNames = {"Розовый", "Персиковый", "Лимонный", "Фисташковый", "Мятный", "Голубой", "Бирюзовый", "Оливковый", "Коралл", "Теплый розовый"};
    for (int i = 0; i < hexColors.size(); ++i) {
        colorCombo->addItem(createColorCircleIcon(hexColors[i]), colorNames[i], hexColors[i]);
    }
    layout->addWidget(colorCombo);

    QPushButton *btnOk = new QPushButton("Сохранить", &dialog);
    btnOk->setStyleSheet("background-color: #4CAF50; color: white; padding: 10px; font-weight: bold; border-radius: 6px; border: none;");
    layout->addWidget(btnOk);

    connect(btnOk, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString colorHex = colorCombo->currentData().toString();
        if (name.isEmpty()) return;

        QSqlQuery q;
        q.prepare("INSERT INTO subjects (name, color) VALUES (?, ?)");
        q.addBindValue(name);
        q.addBindValue(colorHex);
        if (q.exec()) loadSubjects();
    }
}

void MainPageWidget::handleSubjectClick() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    m_activeButton = btn;
    m_selectedSubjectId = btn->property("id").toInt();
    m_selectedSubjectName = btn->property("name").toString();

    QMenu menu(this);
    menu.setStyleSheet("QMenu { background-color: #2D2D30; color: white; border: 1px solid #444; }");
    QAction *goToNotes = menu.addAction("Перейти к заметкам");
    QAction *goToStats = menu.addAction("Посмотреть статистику");
    menu.addSeparator();
    QAction *deleteAction = menu.addAction("Удалить предмет");

    QAction *selected = menu.exec(QCursor::pos());
    if (selected == goToNotes) emit requestGoToNotes(m_selectedSubjectId);
    else if (selected == goToStats) emit requestGoToStats(m_selectedSubjectId);
    else if (selected == deleteAction) deleteSelectedSubject();
}

void MainPageWidget::deleteSelectedSubject() {
    if (m_selectedSubjectId == -1) return;
    auto res = QMessageBox::question(this, "Удаление", QString("Удалить \"%1\" со всеми данными?").arg(m_selectedSubjectName), QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        QSqlQuery q;
        q.prepare("DELETE FROM subjects WHERE id = ?");
        q.addBindValue(m_selectedSubjectId);
        if (q.exec()) loadData();
    }
}