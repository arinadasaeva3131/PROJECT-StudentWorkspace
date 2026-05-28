#ifndef MAINPAGEWIDGET_H
#define MAINPAGEWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QShowEvent>

class MainPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainPageWidget(QWidget *parent = nullptr);
    ~MainPageWidget() = default;

public slots:
    void loadData();

signals:
    void requestGoToNotes(int subjectId);
    void requestGoToStats(int subjectId);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void addSubject();
    void deleteSelectedSubject();
    void handleSubjectClick();
    void toggleEventStatus(QListWidgetItem *item);
    void updateCurrentPairStatus();

private:
    QGridLayout *m_subjectsLayout;

    // Списки для "Сегодня" и "Ближайших событий"
    QListWidget *m_todayList;
    QLabel *m_todayEmptyLabel;
    QListWidget *m_upcomingList;
    QLabel *m_upcomingEmptyLabel;

    // Элементы для панели фокуса времени
    QLabel *m_focusStatusLabel;
    QProgressBar *m_focusProgressBar;
    QTimer *m_statusTimer;

    int m_selectedSubjectId;
    QString m_selectedSubjectName;
    QPushButton *m_activeButton;

    void setupUI();
    void loadSubjects();
    void loadTodaySchedule();
    QIcon createColorCircleIcon(const QString &hexColor);
};

#endif // MAINPAGEWIDGET_H