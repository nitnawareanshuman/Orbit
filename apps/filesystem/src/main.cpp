#include "FileManagerWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Orbit Files");
    QCoreApplication::setApplicationVersion("0.1.0");

    FileManagerWindow window;
    window.show();
    return app.exec();
}
