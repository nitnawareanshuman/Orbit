#pragma once

#include <QMainWindow>

class FileManagerWidget;

class FileManagerWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit FileManagerWindow(QWidget *parent = nullptr);

private:
    FileManagerWidget *fileManager = nullptr;
};
