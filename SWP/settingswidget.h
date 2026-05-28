#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
class QSpinBox;

class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);
private slots:
    void saveTime();
private:
    QSpinBox *m_sbHour;
    QSpinBox *m_sbMin;
};
#endif