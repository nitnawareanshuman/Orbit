#include "ProcessManager.h"

#include <QDir>
#include <QFileInfo>

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
{
}

bool ProcessManager::isRunning() const
{
    return running;
}

void ProcessManager::cleanup()
{
    for (QProcess *process : processes)
    {
        if (process->state() != QProcess::NotRunning)
        {
            process->kill();
            process->waitForFinished();
        }

        delete process;
    }

    processes.clear();
    running = false;
}

void ProcessManager::execute(const Pipeline &pipeline)
{
    if (pipeline.isEmpty())
        return;

    if (running)
    {
        emit errorReady(
            "orbit: another pipeline is running\n");

        return;
    }

    running = true;

    const int count = pipeline.commands.size();

    processes.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        const Command &command = pipeline.commands[i];

        auto *process = new QProcess(this);

        process->setProcessChannelMode(
            QProcess::SeparateChannels);

        process->setProgram(command.executable);
        process->setArguments(command.arguments);
        process->setWorkingDirectory(QDir::currentPath());

        // Handle input redirection.
        if (!command.inputFile.isEmpty())
        {
            const QFileInfo info(command.inputFile);

            if (!info.exists() || !info.isFile())
            {
                emit errorReady(
                    "orbit: input file not found: " +
                    command.inputFile + "\n");

                delete process;
                cleanup();
                return;
            }

            process->setStandardInputFile(
                info.absoluteFilePath());
        }

        // Handle output redirection.
        if (!command.outputFile.isEmpty())
        {
            const QFileInfo info(command.outputFile);

            if (info.exists() && info.isDir())
            {
                emit errorReady(
                    "orbit: output is a directory: " +
                    command.outputFile + "\n");

                delete process;
                cleanup();
                return;
            }

            const QIODevice::OpenMode mode =
                command.appendOutput
                    ? QIODevice::Append
                    : QIODevice::Truncate;

            process->setStandardOutputFile(
                info.absoluteFilePath(),
                mode);
        }

        // Connect stderr independently.
        connect(
            process,
            &QProcess::readyReadStandardError,
            this,
            [this, process]()
            {
                emit errorReady(
                    QString::fromLocal8Bit(
                        process->readAllStandardError()));
            });

        // Display only the last command's stdout.
        if (i == count - 1 &&
            command.outputFile.isEmpty())
        {
            connect(
                process,
                &QProcess::readyReadStandardOutput,
                this,
                [this, process]()
                {
                    emit outputReady(
                        QString::fromLocal8Bit(
                            process->readAllStandardOutput()));
                });
        }

        processes.append(process);
    }

    // Connect stdout of every stage to stdin of next stage.
    for (int i = 0; i < count - 1; ++i)
    {
        processes[i]->setStandardOutputProcess(
            processes[i + 1]);
    }

    // Track completion of the entire pipeline.
    const auto state = new int(count);

    for (int i = 0; i < count; ++i)
    {
        connect(
            processes[i],
            &QProcess::finished,
            this,
            [this, state, i](int exitCode,
                             QProcess::ExitStatus status)
            {
                Q_UNUSED(i);

                if (status == QProcess::CrashExit)
                {
                    emit errorReady(
                        "orbit: process crashed\n");
                }

                if (exitCode != 0)
                {
                    emit errorReady(
                        QString("orbit: process exited with code %1\n")
                            .arg(exitCode));
                }

                --(*state);

                if (*state == 0)
                {
                    running = false;

                    delete state;

                    emit processFinished(exitCode);
                }
            });
    }

    // Start downstream processes first.
    for (int i = count - 1; i >= 0; --i)
    {
        processes[i]->start();

        if (!processes[i]->waitForStarted())
        {
            emit errorReady(
                "orbit: failed to start " +
                processes[i]->program() + ": " +
                processes[i]->errorString() + "\n");

            cleanup();
            delete state;
            return;
        }

        emit processStarted(
            static_cast<qint64>(processes[i]->processId()));
    }
}

void ProcessManager::terminate()
{
    for (QProcess *process : processes)
    {
        if (process->state() != QProcess::NotRunning)
            process->terminate();
    }
}