#pragma once

#include "Command.h"

#include <QObject>
#include <QProcess>
#include <QHash>
#include <QVector>

struct ProcessJob
{
    int id = 0;
    QString command;
    QProcess *process = nullptr;
    bool background = false;
    bool finished = false;
    int exitCode = -1;
};

class ProcessManager : public QObject
{
    Q_OBJECT
public:
    explicit ProcessManager(QObject *parent = nullptr);

    bool isRunning() const;
    void execute(const Pipeline &pipeline, const QString &originalCommand = {});
    QString jobsText() const;
    bool foreground(int jobId);
    bool background(int jobId);
    bool signalJob(int jobId, int signalNumber);
    void terminate();

signals:
    void outputReady(const QString &text);
    void errorReady(const QString &text);
    void processStarted(qint64 pid);
    void processFinished(int exitCode);
    void jobChanged();

private:
    QHash<int, ProcessJob> jobs;
    int nextJobId = 1;
    int foregroundJobId = 0;

    // reportExitError is false for FailedToStart because a concise
    // "command not found" error has already been emitted.
    void finishJob(int jobId, int exitCode, QProcess::ExitStatus status,
                   bool reportExitError = true);
    static QString stateName(const ProcessJob &job);
};
