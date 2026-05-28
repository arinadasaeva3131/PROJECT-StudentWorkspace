#include "mainwindow.h"
#include "databasemanager.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // КРИТИЧЕСКИ ВАЖНО: сначала включаем базу данных, и только потом интерфейс!
    if (!DatabaseManager::instance().initDatabase()) {
        QMessageBox::critical(nullptr, "Ошибка базы данных",
                              "Не удалось загрузить или создать базу данных.\n"
                              "Приложение будет закрыто.");
        return -1;
    }

    // База успешно подключена, теперь можно безопасно запускать окна
    MainWindow w;
    w.show();

    return a.exec();
}