#pragma once
#include "Command.h"
#include <QString>
#include <optional>

class CommandParser
{
public:
    static std::optional<Pipeline> parse(const QString &input, QString *error = nullptr);
};
