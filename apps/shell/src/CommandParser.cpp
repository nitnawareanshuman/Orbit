#include "CommandParser.h"

std::optional<Command> CommandParser::parse(
    const QString &input,
    QString *error)
{
    QStringList tokens;
    QString current;

    QChar quote;
    bool escaping = false;
    bool tokenStarted = false;

    for (const QChar c : input)
    {
        if (escaping)
        {
            current += c;
            escaping = false;
            tokenStarted = true;
            continue;
        }

        if (c == '\\' && quote != '\'')
        {
            escaping = true;
            tokenStarted = true;
            continue;
        }

        if (!quote.isNull())
        {
            if (c == quote)
            {
                quote = QChar();
            }
            else
            {
                current += c;
            }

            continue;
        }

        if (c == '\'' || c == '"')
        {
            quote = c;
            tokenStarted = true;
            continue;
        }

        if (c.isSpace())
        {
            if (tokenStarted)
            {
                tokens.append(current);
                current.clear();
                tokenStarted = false;
            }

            continue;
        }

        current += c;
        tokenStarted = true;
    }

    if (escaping)
    {
        if (!quote.isNull())
        {
            current += '\\';
        }
        else
        {
            current += '\\';
        }
    }

    if (!quote.isNull())
    {
        if (error)
            *error = "Unmatched quotation mark";

        return std::nullopt;
    }

    if (tokenStarted)
        tokens.append(current);

    if (tokens.isEmpty())
        return Command{};

    Command command;

    command.executable = tokens.first();
    command.arguments = tokens.mid(1);

    return command;
}