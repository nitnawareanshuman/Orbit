#include "CommandParser.h"
#include <QProcessEnvironment>

namespace {
enum class TokenType { Word, Pipe, Input, Output, Append, Background };
struct Token { TokenType type; QString value; };

QString expandVariables(const QString &word)
{
    const auto env = QProcessEnvironment::systemEnvironment();
    QString result;
    for (qsizetype i = 0; i < word.size(); ++i) {
        if (word[i] == '$') {
            if (i + 1 < word.size() && word[i + 1] == '{') {
                const qsizetype end = word.indexOf('}', i + 2);
                if (end >= 0) {
                    result += env.value(word.mid(i + 2, end - i - 2));
                    i = end;
                    continue;
                }
            }
            qsizetype end = i + 1;
            while (end < word.size() && (word[end].isLetterOrNumber() || word[end] == '_')) ++end;
            if (end > i + 1) {
                result += env.value(word.mid(i + 1, end - i - 1));
                i = end - 1;
                continue;
            }
        }
        if (word[i] == '~' && i == 0 && (word.size() == 1 || word[1] == '/'))
            result += env.value("HOME");
        else
            result += word[i];
    }
    return result;
}

std::optional<QVector<Token>> tokenize(const QString &input, QString *error)
{
    QVector<Token> tokens;
    QString current;
    QChar quote;
    bool escaping = false, tokenStarted = false, expand = true;
    auto flush = [&]() {
        if (tokenStarted) {
            tokens.append({TokenType::Word, expand ? expandVariables(current) : current});
            current.clear(); tokenStarted = false; expand = true;
        }
    };

    for (qsizetype i = 0; i < input.size(); ++i) {
        const QChar c = input[i];
        if (escaping) { current += c; escaping = false; tokenStarted = true; continue; }
        if (c == '\\' && quote != '\'') { escaping = true; tokenStarted = true; continue; }
        if (!quote.isNull()) {
            if (c == quote) quote = QChar();
            else current += c;
            continue;
        }
        if (c == '\'' || c == '"') { quote = c; if (c == '\'') expand = false; tokenStarted = true; continue; }
        if (c.isSpace()) { flush(); continue; }
        if (c == '&') { flush(); tokens.append({TokenType::Background, "&"}); continue; }
        if (c == '|') { flush(); tokens.append({TokenType::Pipe, "|"}); continue; }
        if (c == '<') { flush(); tokens.append({TokenType::Input, "<"}); continue; }
        if (c == '>') {
            flush();
            if (i + 1 < input.size() && input[i + 1] == '>') { tokens.append({TokenType::Append, ">>"}); ++i; }
            else tokens.append({TokenType::Output, ">"});
            continue;
        }
        current += c; tokenStarted = true;
    }
    if (escaping) { if (error) *error = "Trailing escape character"; return std::nullopt; }
    if (!quote.isNull()) { if (error) *error = "Unmatched quotation mark"; return std::nullopt; }
    flush();
    return tokens;
}
}

std::optional<Pipeline> CommandParser::parse(const QString &input, QString *error)
{
    auto tokens = tokenize(input, error);
    if (!tokens) return std::nullopt;

    Pipeline pipeline;
    Command command;
    bool expectInput = false, expectOutput = false, sawBackground = false;
    for (qsizetype i = 0; i < tokens->size(); ++i) {
        const Token &token = (*tokens)[i];
        if (token.type == TokenType::Word) {
            if (expectInput) {
                if (!command.inputFile.isEmpty()) { if (error) *error = "Multiple input redirections"; return std::nullopt; }
                command.inputFile = token.value; expectInput = false; continue;
            }
            if (expectOutput) {
                if (!command.outputFile.isEmpty()) { if (error) *error = "Multiple output redirections"; return std::nullopt; }
                command.outputFile = token.value; expectOutput = false; continue;
            }
            if (command.executable.isEmpty()) command.executable = token.value;
            else command.arguments.append(token.value);
            continue;
        }
        if (token.type == TokenType::Input || token.type == TokenType::Output || token.type == TokenType::Append) {
            if (expectInput || expectOutput) { if (error) *error = "Missing redirection filename"; return std::nullopt; }
            if (token.type == TokenType::Input) expectInput = true;
            else { expectOutput = true; command.appendOutput = token.type == TokenType::Append; }
            continue;
        }
        if (token.type == TokenType::Pipe) {
            if (expectInput || expectOutput || command.isEmpty()) { if (error) *error = "Invalid command before pipe"; return std::nullopt; }
            pipeline.commands.append(command); command = Command(); continue;
        }
        if (token.type == TokenType::Background) {
            if (sawBackground || i != tokens->size() - 1 || command.isEmpty()) { if (error) *error = "'&' must appear once at the end of a command"; return std::nullopt; }
            sawBackground = true; pipeline.background = true;
        }
    }
    if (expectInput || expectOutput) { if (error) *error = "Missing redirection filename"; return std::nullopt; }
    if (!command.isEmpty()) pipeline.commands.append(command);
    if (pipeline.commands.isEmpty()) return Pipeline{};
    return pipeline;
}
