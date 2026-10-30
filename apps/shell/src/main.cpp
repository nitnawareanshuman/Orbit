#include "TerminalWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Orbit Shell");
    QCoreApplication::setApplicationVersion("0.1.0");

    TerminalWindow window;

    window.show();

    return app.exec();
}