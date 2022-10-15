#include "mainwindow.h"

#include <QApplication>
#include <QDateTime>

#include "logger.h"

int main(int argc, char *argv[])
{
    Logger *logger = Logger::getInstance();
    logger->writeMessage("App", QtInfoMsg, "Start application...");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
