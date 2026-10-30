#include "ProcessManager.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QProcessEnvironment>

#ifdef Q_OS_UNIX
#include <csignal>
#include <cerrno>
#include <unistd.h>
#endif

ProcessManager::ProcessManager(QObject *parent) : QObject(parent) {}

bool ProcessManager::isRunning() const
{
    for (const ProcessJob &job : jobs)
        if (!job.finished && !job.background)
            return true;
    return false;
}

QString ProcessManager::stateName(const ProcessJob &job)
{
    if (job.finished) return job.exitCode == 0 ? "Done" : "Exit";
    if (job.process && job.process->state() == QProcess::Running) return "Running";
    return "Starting";
}

QString ProcessManager::jobsText() const
{
    QStringList lines;
    for (auto it = jobs.cbegin(); it != jobs.cend(); ++it)
        lines << QString("[%1] %2  %3")
                     .arg(it.key())
                     .arg(stateName(it.value()))
                     .arg(it.value().command);
    return lines.isEmpty() ? "No active jobs\n" : lines.join('\n') + '\n';
}

void ProcessManager::execute(const Pipeline &pipeline, const QString &originalCommand)
{
    if (pipeline.isEmpty()) return;

    // The current Qt6 shell executes a pipeline through QProcess. A pipeline is
    // represented as one job; QProcess owns the OS child process lifecycle.
    if (pipeline.commands.size() != 1) {
        emit errorReady("orbit: multi-stage pipelines are not yet supported by job control\n");
        return;
    }

    const Command &command = pipeline.commands.first();
    auto *process = new QProcess(this);
    process->setProgram(command.executable);
    process->setArguments(command.arguments);
    process->setWorkingDirectory(QDir::currentPath());
    process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    process->setProcessChannelMode(QProcess::SeparateChannels);

    if (!command.inputFile.isEmpty()) {
        QFileInfo input(command.inputFile);
        if (!input.exists() || !input.isFile()) {
            emit errorReady("orbit: input file not found: " + command.inputFile + '\n');
            process->deleteLater();
            return;
        }
        process->setStandardInputFile(input.absoluteFilePath());
    }
    if (!command.outputFile.isEmpty()) {
        QFileInfo output(command.outputFile);
        if (output.exists() && output.isDir()) {
            emit errorReady("orbit: output is a directory: " + command.outputFile + '\n');
            process->deleteLater();
            return;
        }
        process->setStandardOutputFile(output.absoluteFilePath(),
            command.appendOutput ? QIODevice::Append : QIODevice::Truncate);
    }

    const int id = nextJobId++;
    ProcessJob job;
    job.id = id;
    job.command = originalCommand.isEmpty() ? command.executable : originalCommand;
    job.process = process;
    job.background = pipeline.background;
    jobs.insert(id, job);
    if (!pipeline.background) foregroundJobId = id;

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process, id]() {
        const QString data = QString::fromLocal8Bit(process->readAllStandardOutput());
        if (!data.isEmpty()) emit outputReady(data);
        Q_UNUSED(id);
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        const QString data = QString::fromLocal8Bit(process->readAllStandardError());
        if (!data.isEmpty()) emit errorReady(data);
    });
    connect(process, &QProcess::started, this, [this, process, id]() {
        emit processStarted(static_cast<qint64>(process->processId()));
        emit jobChanged();
        if (jobs.value(id).background)
            emit outputReady(QString("[%1] %2\n").arg(id).arg(process->processId()));
    });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, id](int code, QProcess::ExitStatus status) {
        finishJob(id, code, status);
    });
    connect(process, &QProcess::errorOccurred, this, [this, process, id](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit errorReady("orbit: failed to start " + process->program() + ": " + process->errorString() + '\n');
            finishJob(id, 127, QProcess::NormalExit);
        }
    });

    process->start();
    if (pipeline.background) {
        // Return immediately; signals continue to update the job table.
        emit jobChanged();
    } else {
        // Foreground is asynchronous too, preserving GUI responsiveness.
        emit jobChanged();
    }
}

void ProcessManager::finishJob(int jobId, int exitCode, QProcess::ExitStatus status)
{
    auto it = jobs.find(jobId);
    if (it == jobs.end() || it->finished) return;
    it->finished = true;
    it->exitCode = exitCode;
    if (status == QProcess::CrashExit)
        emit errorReady(QString("orbit: job %1 terminated by a signal\n").arg(jobId));
    else if (exitCode != 0)
        emit errorReady(QString("orbit: job %1 exited with status %2\n").arg(jobId).arg(exitCode));
    if (foregroundJobId == jobId) {
        foregroundJobId = 0;
        emit processFinished(exitCode);
    } else {
        emit outputReady(QString("[%1] Done (%2) %3\n").arg(jobId).arg(exitCode).arg(it->command));
    }
    emit jobChanged();
}

bool ProcessManager::foreground(int jobId)
{
    auto it = jobs.find(jobId);
    if (it == jobs.end() || it->finished || !it->process) return false;
    it->background = false;
    foregroundJobId = jobId;
    emit outputReady(QString("[%1] foreground: %2\n").arg(jobId).arg(it->command));
    return true;
}

bool ProcessManager::background(int jobId)
{
    auto it = jobs.find(jobId);
    if (it == jobs.end() || it->finished || !it->process) return false;
    it->background = true;
    if (foregroundJobId == jobId) foregroundJobId = 0;
    emit outputReady(QString("[%1] continued in background\n").arg(jobId));
    emit jobChanged();
    return true;
}

bool ProcessManager::signalJob(int jobId, int signalNumber)
{
    auto it = jobs.find(jobId);
    if (it == jobs.end() || it->finished || !it->process) return false;
#ifdef Q_OS_UNIX
    const qint64 pid = it->process->processId();
    if (pid <= 0) return false;
    return ::kill(static_cast<pid_t>(pid), signalNumber) == 0;
#else
    Q_UNUSED(signalNumber);
    it->process->terminate();
    return true;
#endif
}

void ProcessManager::terminate()
{
    for (ProcessJob &job : jobs) {
        if (!job.finished && job.process) {
            job.process->terminate();
            if (!job.process->waitForFinished(1500)) job.process->kill();
        }
    }
}
