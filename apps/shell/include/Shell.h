#pragma once
#include "CommandParser.h"
#include "ProcessManager.h"
#include <QObject>

class Shell : public QObject
{
    Q_OBJECT
public:
    explicit Shell(QObject *parent = nullptr);
    void executeLine(const QString &input);
    QString currentDirectory() const;
    void stop();

signals:
    void outputReady(const QString &text);
    void errorReady(const QString &text);
    void promptChanged(const QString &directory);
    void directoryChanged(const QString &directory);
    void openFileRequested(const QString &absolutePath);
    void shellExited();

private:
    ProcessManager processManager;
    bool running = true;
    void executeBuiltin(const Command &command);
};
