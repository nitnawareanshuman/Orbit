#pragma once

#include <QString>
#include <QStringList>

struct Command
{
    QString executable;
    QStringList arguments;

    bool isEmpty() const
    {
        return executable.trimmed().isEmpty();
    }
};