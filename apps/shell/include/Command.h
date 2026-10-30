#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct Command
{
    QString executable;
    QStringList arguments;
    QString inputFile;
    QString outputFile;
    bool appendOutput = false;

    bool isEmpty() const { return executable.trimmed().isEmpty(); }
};

struct Pipeline
{
    QVector<Command> commands;
    bool background = false;

    bool isEmpty() const { return commands.isEmpty(); }
};
