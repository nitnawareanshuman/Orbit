#include "ProcessManager.h"

#include <QDir>
#include <QProcessEnvironment>

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent),
      process(new QProcess(this))
{
    process->setProcessChannelMode(
        QProcess::SeparateChannels);

    connect(
        process,
        &QProcess::readyReadStandardOutput,
        this,
        [this]()
        {
            const QByteArray data =
                process->readAllStandardOutput();

            emit outputReady(
                QString::fromLocal8Bit(data));
        });

    connect(
        process,
        &QProcess::readyReadStandardError,
        this,
        [this]()
        {
            const QByteArray data =
                process->readAllStandardError();

            emit errorReady(
                QString::fromLocal8Bit(data));
        });

    connect(
        process,
        &QProcess::started,
        this,
        [this]()
        {
            emit processStarted(
                static_cast<qint64>(process->processId()));
        });

    connect(
        process,
        &QProcess::finished,
        this,
        [this](int exitCode, QProcess::ExitStatus)
        {
            emit processFinished(exitCode);
        });

    connect(
        process,
        &QProcess::errorOccurred,
        this,
        [this](QProcess::ProcessError error)
        {
            if (error == QProcess::FailedToStart)
            {
                emit errorReady(
                    "orbit: failed to start process: " +
                    process->errorString() + "\n");
            }
        });
}

bool ProcessManager::isRunning() const
{
    return process->state() != QProcess::NotRunning;
}

void ProcessManager::execute(const Command &command)
{
    if (command.isEmpty())
        return;

    if (isRunning())
    {
        emit errorReady(
            "orbit: a process is already running\n");

        return;
    }

    process->setWorkingDirectory(QDir::currentPath());

    process->start(
        command.executable,
        command.arguments);
}

void ProcessManager::terminate()
{
    if (isRunning())
    {
        process->terminate();
    }
}