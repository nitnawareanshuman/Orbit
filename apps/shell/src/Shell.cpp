#include "Shell.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>

#ifdef Q_OS_UNIX
#include <csignal>
#endif

Shell::Shell(QObject *parent) : QObject(parent), processManager(this)
{
    connect(&processManager, &ProcessManager::outputReady,
            this, &Shell::outputReady);
    connect(&processManager, &ProcessManager::errorReady,
            this, &Shell::errorReady);

    // Restore the interactive prompt after every foreground external command
    // (for example touch, ls, git, cmake) has finished.
    connect(&processManager, &ProcessManager::processFinished,
            this, [this](int)
            { emit promptChanged(currentDirectory()); });
}

QString Shell::currentDirectory() const
{
    return QDir::currentPath();
}

void Shell::executeLine(const QString &input)
{
    if (!running)
        return;

    const QString commandLine = input.trimmed();
    if (commandLine.isEmpty())
        return;

    QString error;
    const auto pipeline = CommandParser::parse(commandLine, &error);
    if (!pipeline)
    {
        emit errorReady("orbit: " + error + '\n');
        return;
    }
    if (pipeline->isEmpty())
        return;

    if (pipeline->commands.size() == 1 && !pipeline->background)
    {
        const Command &command = pipeline->commands.first();
        const QString name = command.executable;
        if (name == "cd" || name == "pwd" || name == "exit" || name == "echo" ||
            name == "jobs" || name == "fg" || name == "bg" || name == "kill" ||
            name == "export" || name == "unset" || name == "mkdir" || name == "cat")
        {
            executeBuiltin(command);
            return;
        }
    }

    processManager.execute(*pipeline, commandLine);
}

void Shell::executeBuiltin(const Command &command)
{
    const QString name = command.executable;
    const QStringList args = command.arguments;

    if (name == "pwd")
    {
        emit outputReady(currentDirectory() + '\n');
        return;
    }

    if (name == "cd")
    {
        const QString path = args.isEmpty() || args.first() == "~"
                                 ? QProcessEnvironment::systemEnvironment().value("HOME")
                                 : args.first();
        if (path.isEmpty())
        {
            emit errorReady("orbit: HOME is not set\n");
            return;
        }
        if (!QDir::setCurrent(path))
        {
            emit errorReady("orbit: cd: cannot access " + path + '\n');
            return;
        }
        emit promptChanged(currentDirectory());
        emit directoryChanged(currentDirectory());
        return;
    }

    if (name == "mkdir")
    {
        if (args.isEmpty())
        {
            emit errorReady("orbit: mkdir: missing operand\n");
            return;
        }

        bool makeParents = false;
        QStringList paths;
        for (const QString &arg : args)
        {
            if (arg == "-p" || arg == "--parents")
                makeParents = true;
            else if (arg.startsWith('-'))
            {
                emit errorReady("orbit: mkdir: unsupported option: " + arg + '\n');
                return;
            }
            else
                paths << arg;
        }

        if (paths.isEmpty())
        {
            emit errorReady("orbit: mkdir: missing operand\n");
            return;
        }

        QStringList created;
        QStringList failed;
        for (const QString &path : paths)
        {
            QDir dir;
            const bool ok = makeParents ? dir.mkpath(path) : dir.mkdir(path);
            if (ok)
                created << QFileInfo(path).absoluteFilePath();
            else
                failed << path;
        }

        for (const QString &path : failed)
            emit errorReady("orbit: mkdir: cannot create directory '" + path + "'\n");

        if (!created.isEmpty())
        {
            emit outputReady("Created directory" + QString(created.size() == 1 ? ": " : "ies: ") + created.join(", ") + '\n');
            emit directoryChanged(currentDirectory());
        }
        return;
    }

    if (name == "cat")
    {
        if (args.isEmpty())
        {
            emit errorReady("orbit: cat: missing file operand\n");
            return;
        }
        for (const QString &path : args)
        {
            QFileInfo info(path);
            if (!info.exists() || !info.isFile())
            {
                emit errorReady("orbit: cat: " + path + ": no such file\n");
                continue;
            }
            emit openFileRequested(info.absoluteFilePath());
        }
        return;
    }

    if (name == "exit")
    {
        if (!args.isEmpty())
        {
            emit errorReady("orbit: exit: too many arguments\n");
            return;
        }
        stop();
        return;
    }

    if (name == "echo")
    {
        emit outputReady(args.join(' ') + '\n');
        return;
    }

    if (name == "jobs")
    {
        emit outputReady(processManager.jobsText());
        return;
    }

    if (name == "fg" || name == "bg" || name == "kill")
    {
        if (args.isEmpty())
        {
            emit errorReady("orbit: " + name + ": job id required\n");
            return;
        }

        bool ok = false;
        int id = args.last().toInt(&ok);
        if (!ok)
        {
            QString value = args.last();
            if (value.startsWith('%'))
                value.remove(0, 1);
            id = value.toInt(&ok);
        }
        if (!ok)
        {
            emit errorReady("orbit: " + name + ": invalid job id\n");
            return;
        }

        bool success = false;
        if (name == "fg")
            success = processManager.foreground(id);
        else if (name == "bg")
            success = processManager.background(id);
        else
        {
#ifdef Q_OS_UNIX
            int signalNumber = SIGTERM;
            if (args.size() > 1 && args.first().startsWith('-'))
            {
                const QString signalName = args.first().mid(1).toUpper();
                if (signalName == "KILL")
                    signalNumber = SIGKILL;
                else if (signalName == "INT")
                    signalNumber = SIGINT;
                else if (signalName == "STOP")
                    signalNumber = SIGSTOP;
                else if (signalName == "CONT")
                    signalNumber = SIGCONT;
            }
            success = processManager.signalJob(id, signalNumber);
#else
            success = processManager.signalJob(id, 15);
#endif
        }
        if (!success)
            emit errorReady("orbit: " + name + ": no such active job: %" + QString::number(id) + '\n');
        return;
    }

    if (name == "export")
    {
        if (args.isEmpty())
        {
            const QStringList environment = QProcessEnvironment::systemEnvironment().toStringList();
            emit outputReady(environment.join('\n') + '\n');
            return;
        }
        for (const QString &assignment : args)
        {
            const int separator = assignment.indexOf('=');
            if (separator <= 0)
            {
                emit errorReady("orbit: export: expected NAME=value\n");
                continue;
            }
            qputenv(assignment.left(separator).toLocal8Bit(), assignment.mid(separator + 1).toLocal8Bit());
        }
        emit promptChanged(currentDirectory());
        return;
    }

    if (name == "unset")
    {
        for (const QString &key : args)
            qunsetenv(key.toLocal8Bit().constData());
        emit promptChanged(currentDirectory());
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
