#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
class QComboBox;
class QLabel;
class QProgressBar;

class StatisticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatisticsWidget(QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void loadData();
private:
    // Вспомогательная функция для создания одинаковых шкал
    QProgressBar* createProgressBar();

    QComboBox *m_cbSubject;
    QLabel *m_totalTodayLabel;

    QProgressBar *m_todayBar;
    QProgressBar *m_weekBar;
    QProgressBar *m_monthBar;

    int m_todayMin = 0;
    int m_weekMin = 0;
    int m_monthMin = 0;
};
#endif