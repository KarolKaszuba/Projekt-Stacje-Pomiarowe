#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Utworzenie instancji MainWindow
    MainWindow mainWindow;
    mainWindow.show();

    // Wywołanie pobierania danych
    //mainWindow.fetchStations();

    return app.exec();
}
