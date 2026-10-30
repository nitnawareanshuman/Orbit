#include "FileManagerWindow.h"
#include "FileManagerWidget.h"
#include "TextEditorWindow.h"

#include <QDir>
#include <QMenuBar>
#include <QStatusBar>

FileManagerWindow::FileManagerWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Orbit Files");
    resize(1120, 760);

    fileManager = new FileManagerWidget(this);
    setCentralWidget(fileManager);
    statusBar()->showMessage(QDir::homePath());

    connect(fileManager, &FileManagerWidget::fileOpenRequested,
            this, [](const QString &path) {
        auto *editor = new TextEditorWindow(path);
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->show();
    });
}
