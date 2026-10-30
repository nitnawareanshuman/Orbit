#pragma once

#include "Command.h"

#include <QString>
#include <optional>

class CommandParser
{
public:
    static std::optional<Command> parse(
        const QString &input,
        QString *error = nullptr);
};