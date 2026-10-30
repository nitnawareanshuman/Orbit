#include "Shell.h"

#include <QDir>
#include <QProcessEnvironment>

Shell::Shell(QObject *parent)
    : QObject(parent),
      processManager(this)
{
    connect(
        &processManager,
        &ProcessManager::outputReady,
        this,
        &Shell::outputReady);

    connect(
        &processManager,
        &ProcessManager::errorReady,
        this,
        &Shell::errorReady);

    connect(
        &processManager,
        &ProcessManager::processFinished,
        this,
        [this](int exitCode)
        {
            emit outputReady(
                QString("\n[process exited: %1]\n")
                    .arg(exitCode));

            emit promptChanged(currentDirectory());
        });

    connect(
        &processManager,
        &ProcessManager::processStarted,
        this,
        [this](qint64 pid)
        {
            emit outputReady(
                QString("[started process %1]\n").arg(pid));
        });
}

QString Shell::currentDirectory() const
{
    return QDir::currentPath();
}

void Shell::executeLine(const QString &input)
{
    if (!running)
        return;

    QString error;

    auto pipeline = CommandParser::parse(input, &error);

    if (!pipeline)
    {
        emit errorReady("orbit: " + error + "\n");
        return;
    }

    if (pipeline->isEmpty())
        return;

    // Initially, handle only standalone built-ins.
    if (pipeline->commands.size() == 1)
    {
        const Command &command = pipeline->commands.first();

        const QString name = command.executable;

        if (name == "cd" ||
            name == "pwd" ||
            name == "exit" ||
            name == "echo")
        {
            executeBuiltin(command);
            return;
        }
    }

    processManager.execute(*pipeline);
}

void Shell::executeBuiltin(const Command &command)
{
    const QString name = command.executable;

    if (name == "pwd")
    {
        emit outputReady(currentDirectory() + "\n");
        return;
    }

    if (name == "cd")
    {
        QString path;

        if (command.arguments.isEmpty() ||
            command.arguments.first() == "~")
        {
            path = QProcessEnvironment::systemEnvironment()
                       .value("HOME");
        }
        else
        {
            path = command.arguments.first();
        }

        if (path.isEmpty())
        {
            emit errorReady("orbit: HOME is not set\n");
            return;
        }

        if (!QDir::setCurrent(path))
        {
            emit errorReady(
                "orbit: cd: cannot access " + path + "\n");
        }

        emit promptChanged(currentDirectory());
        return;
    }

    if (name == "exit")
    {
        if (!command.arguments.isEmpty())
        {
            emit errorReady(
                "orbit: exit: too many arguments\n");
            return;
        }

        stop();
    }

    if (name == "echo")
    {
        emit outputReady(
            command.arguments.join(" ") + "\n");

        return;
    }
}

void Shell::stop()
{
    if (!running)
        return;

    running = false;
    processManager.terminate();

    emit outputReady("Orbit Shell terminated.\n");
    emit shellExited();
}