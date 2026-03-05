#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("MLE-MP");
    QApplication::setApplicationName("Minecraft Legacy Launcher");

    MainWindow window;
    window.show();

    return app.exec();
}
