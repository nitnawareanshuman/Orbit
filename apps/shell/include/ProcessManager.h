#pragma once

#include "Command.h"

#include <QObject>
#include <QProcess>
#include <QString>

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(QObject *parent = nullptr);

    bool isRunning() const;

    void execute(const Command &command);

    void terminate();

signals:
    void outputReady(const QString &text);
    void errorReady(const QString &text);

    void processStarted(qint64 pid);
    void processFinished(int exitCode);

private:
    QProcess *process;
};