#include "CommandParser.h"

namespace
{
    enum class TokenType
    {
        Word,
        Pipe,
        Input,
        Output,
        Append
    };

    struct Token
    {
        TokenType type;
        QString value;
    };

    std::optional<QVector<Token>> tokenize(
        const QString &input,
        QString *error)
    {
        QVector<Token> tokens;

        QString current;
        QChar quote;

        bool escaping = false;
        bool tokenStarted = false;

        auto flush = [&]()
        {
            if (tokenStarted)
            {
                tokens.append({TokenType::Word, current});
                current.clear();
                tokenStarted = false;
            }
        };

        for (qsizetype i = 0; i < input.size(); ++i)
        {
            const QChar c = input[i];

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
                    quote = QChar();
                else
                    current += c;

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
                flush();
                continue;
            }

            if (c == '|')
            {
                flush();
                tokens.append({TokenType::Pipe, "|"});
                continue;
            }

            if (c == '<')
            {
                flush();
                tokens.append({TokenType::Input, "<"});
                continue;
            }

            if (c == '>')
            {
                flush();

                if (i + 1 < input.size() &&
                    input[i + 1] == '>')
                {
                    tokens.append({TokenType::Append, ">>"});
                    ++i;
                }
                else
                {
                    tokens.append({TokenType::Output, ">"});
                }

                continue;
            }

            current += c;
            tokenStarted = true;
        }

        if (escaping)
        {
            if (error)
                *error = "Trailing escape character";

            return std::nullopt;
        }

        if (!quote.isNull())
        {
            if (error)
                *error = "Unmatched quotation mark";

            return std::nullopt;
        }

        flush();

        return tokens;
    }
}

std::optional<Pipeline> CommandParser::parse(
    const QString &input,
    QString *error)
{
    auto tokens = tokenize(input, error);

    if (!tokens)
        return std::nullopt;

    Pipeline pipeline;
    Command command;

    bool expectInputFile = false;
    bool expectOutputFile = false;

    for (const Token &token : *tokens)
    {
        if (token.type == TokenType::Word)
        {
            if (expectInputFile)
            {
                if (!command.inputFile.isEmpty())
                {
                    if (error)
                        *error = "Multiple input redirections";

                    return std::nullopt;
                }

                command.inputFile = token.value;
                expectInputFile = false;
                continue;
            }

            if (expectOutputFile)
            {
                if (!command.outputFile.isEmpty())
                {
                    if (error)
                        *error = "Multiple output redirections";

                    return std::nullopt;
                }

                command.outputFile = token.value;
                expectOutputFile = false;
                continue;
            }

            if (command.executable.isEmpty())
                command.executable = token.value;
            else
                command.arguments.append(token.value);

            continue;
        }

        if (token.type == TokenType::Input ||
            token.type == TokenType::Output ||
            token.type == TokenType::Append)
        {
            if (expectInputFile || expectOutputFile)
            {
                if (error)
                    *error = "Missing redirection filename";

                return std::nullopt;
            }

            if (token.type == TokenType::Input)
                expectInputFile = true;
            else
            {
                expectOutputFile = true;
                command.appendOutput =
                    token.type == TokenType::Append;
            }

            continue;
        }

        if (token.type == TokenType::Pipe)
        {
            if (expectInputFile || expectOutputFile ||
                command.isEmpty())
            {
                if (error)
                    *error = "Invalid command before pipe";

                return std::nullopt;
            }

            pipeline.commands.append(command);
            command = Command();

            continue;
        }
    }

    if (expectInputFile || expectOutputFile)
    {
        if (error)
            *error = "Missing redirection filename";

        return std::nullopt;
    }

    if (!command.isEmpty())
        pipeline.commands.append(command);

    if (pipeline.commands.isEmpty())
        return Pipeline{};

    return pipeline;
}