#include <QApplication>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Generador Visual de Scanners");

    MainWindow window;
    window.show();

    return QApplication::exec();
}
