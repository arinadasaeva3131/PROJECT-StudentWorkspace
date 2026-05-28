#ifndef SCHEDULEWIDGET_H
#define SCHEDULEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>

class ScheduleWidget : public QWidget {
    Q_OBJECT

public:
    explicit ScheduleWidget(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void loadSchedule();
    void addScheduleItem(int prefillDay = -1, int prefillRow = -1);
    void handleCellClick(int row, int column);
    void handleHeaderClick(int index);
    void editScheduleItem(int id);
    void addReminderForEvent(int eventId); // НОВОЕ: Слот для создания внутреннего напоминания

private:
    QTableWidget *m_scheduleTable;
    QComboBox *m_weekParityCombo;
    QLabel *m_dateRangeLabel;
    QPushButton *m_btnInvert;
    QLabel *m_labelCurrent;
};

#endif // SCHEDULEWIDGET_H