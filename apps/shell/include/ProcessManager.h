#pragma once

#include "Command.h"

#include <QObject>
#include <QProcess>
#include <QVector>

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(QObject *parent = nullptr);

    bool isRunning() const;

    void execute(const Pipeline &pipeline);

    void terminate();

signals:
    void outputReady(const QString &text);
    void errorReady(const QString &text);

    void processStarted(qint64 pid);
    void processFinished(int exitCode);

private:
    QVector<QProcess *> processes;

    bool running = false;

    void cleanup();
};