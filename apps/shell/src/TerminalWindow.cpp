#include "TerminalWindow.h"

#include <QKeyEvent>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QFontDatabase>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QColor>
#include <algorithm>

namespace {
QTextCharFormat normalFormat()
{
    QTextCharFormat format;
    format.setForeground(QColor("#F2F3F5"));
    return format;
}

QTextCharFormat errorFormat()
{
    QTextCharFormat format;
    format.setForeground(QColor("#FF5C65"));
    return format;
}
}

TerminalWidget::TerminalWidget(QWidget *parent) : QPlainTextEdit(parent)
{
    setReadOnly(false);
    setUndoRedoEnabled(false);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(14);
    setFont(font);
    setTabChangesFocus(false);
    setFocusPolicy(Qt::StrongFocus);

    setStyleSheet(R"(
        QPlainTextEdit {
            background-color: #20232D;
            color: #F2F3F5;
            border: none;
            padding: 16px;
            selection-background-color: #39445A;
        }
    )");

    completions = {"cd", "pwd", "exit", "clear", "jobs", "fg", "bg", "kill",
                   "export", "unset", "ls", "echo", "cat", "mkdir", "touch",
                   "rm", "cp", "mv", "whoami", "date", "uname", "git", "cmake",
                   "make", "python3", "g++", "clang++"};
}

void TerminalWidget::setPrompt(const QString &newPrompt) { prompt = newPrompt; }
void TerminalWidget::setCommandCompletions(const QStringList &commands) { completions = commands; }

void TerminalWidget::moveCursorToEnd()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

QString TerminalWidget::currentInput() const
{
    QTextCursor cursor(document());
    cursor.setPosition(inputStart);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

void TerminalWidget::replaceCurrentInput(const QString &text)
{
    QTextCursor cursor(document());
    cursor.setPosition(inputStart);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.insertText(text, normalFormat());
    setTextCursor(cursor);
}

void TerminalWidget::removePrompt()
{
    if (!promptVisible)
        return;

    QTextCursor cursor(document());
    cursor.setPosition(inputStart);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    promptVisible = false;
}

void TerminalWidget::showPrompt()
{
    if (promptVisible)
        return;

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);

    // Add a line break only when the previous content did not already end one.
    if (cursor.position() > 0 &&
        document()->characterAt(cursor.position() - 1) != QChar('\n')) {
        cursor.insertText("\n", normalFormat());
    }

    // Explicitly reset the format so a previous error cannot color the prompt.
    cursor.insertText(prompt + " ", normalFormat());
    inputStart = cursor.position();
    promptVisible = true;
    setTextCursor(cursor);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::appendOutput(const QString &text)
{
    if (text.isEmpty())
        return;

    const bool hadPrompt = promptVisible;
    const QString pendingInput = hadPrompt ? currentInput() : QString();

    removePrompt();

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, normalFormat());

    if (!text.endsWith('\n'))
        cursor.insertText("\n", normalFormat());

    showPrompt();

    if (hadPrompt && !pendingInput.isEmpty()) {
        QTextCursor end(document());
        end.movePosition(QTextCursor::End);
        end.insertText(pendingInput, normalFormat());
        setTextCursor(end);
    }
}

void TerminalWidget::appendError(const QString &text)
{
    if (text.isEmpty())
        return;

    const bool hadPrompt = promptVisible;
    const QString pendingInput = hadPrompt ? currentInput() : QString();

    removePrompt();

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, errorFormat());

    if (!text.endsWith('\n'))
        cursor.insertText("\n", errorFormat());

    // showPrompt() uses normalFormat(), so only the error text is red.
    showPrompt();

    if (hadPrompt && !pendingInput.isEmpty()) {
        QTextCursor end(document());
        end.movePosition(QTextCursor::End);
        end.insertText(pendingInput, normalFormat());
        setTextCursor(end);
    }
}

void TerminalWidget::startSession(const QString &welcome, const QString &newPrompt)
{
    clear();
    prompt = newPrompt;
    promptVisible = false;

    QTextCursor cursor(document());
    cursor.insertText(welcome + "\n", normalFormat());
    showPrompt();
}

void TerminalWidget::navigateHistory(int direction)
{
    if (history.isEmpty())
        return;

    if (direction < 0) {
        if (historyIndex == history.size())
            savedInput = currentInput();
        historyIndex = std::max(0, historyIndex - 1);
    } else {
        historyIndex = std::min(static_cast<int>(history.size()), historyIndex + 1);
    }

    replaceCurrentInput(historyIndex == history.size() ? savedInput : history.at(historyIndex));
    moveCursorToEnd();
}

void TerminalWidget::autocomplete()
{
    QTextCursor cursor = textCursor();
    if (cursor.position() < inputStart)
        return;

    QString input = currentInput();
    int position = cursor.position() - inputStart;
    int start = position;
    int end = position;

    while (start > 0 && !input[start - 1].isSpace())
        --start;
    while (end < input.size() && !input[end].isSpace())
        ++end;

    const QString token = input.mid(start, position - start);
    if (token.isEmpty())
        return;

    QStringList matches;
    const int slash = token.lastIndexOf('/');
    const QString dirPart = slash >= 0 ? token.left(slash + 1) : QString();
    const QString prefix = slash >= 0 ? token.mid(slash + 1) : token;

    QDir dir(dirPart.isEmpty() ? QDir::currentPath() : dirPart);
    if (dir.exists()) {
        const auto entries = dir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : entries) {
            if (entry.fileName().startsWith(prefix, Qt::CaseInsensitive)) {
                matches << dirPart + entry.fileName() + (entry.isDir() ? "/" : "");
            }
        }
    }

    if (matches.isEmpty() && slash < 0) {
        for (const QString &item : completions) {
            if (item.startsWith(token, Qt::CaseInsensitive))
                matches << item;
        }

        const QString path = QProcessEnvironment::systemEnvironment().value("PATH");
        for (const QString &directory : path.split(QDir::listSeparator())) {
            QDir pathDir(directory);
            const auto entries = pathDir.entryInfoList(
                QDir::Files | QDir::Executable, QDir::Name);
            for (const QFileInfo &entry : entries) {
                if (entry.fileName().startsWith(token, Qt::CaseInsensitive))
                    matches << entry.fileName();
            }
        }
    }

    matches.removeDuplicates();
    matches.sort(Qt::CaseInsensitive);
    if (matches.isEmpty())
        return;

    if (matches.size() > 1) {
        appendOutput("\n" + matches.join("    "));
        input = currentInput();
    }

    const QString replacement = matches.first();
    input.replace(start, end - start, replacement);
    replaceCurrentInput(input);

    QTextCursor updated(document());
    updated.setPosition(inputStart + start + replacement.size());
    setTextCursor(updated);
}

void TerminalWidget::protectPreviousOutput()
{
    if (textCursor().position() < inputStart) {
        QTextCursor cursor(document());
        cursor.setPosition(document()->characterCount() - 1);
        setTextCursor(cursor);
    }
}

void TerminalWidget::submitCommand()
{
    if (!promptVisible)
        return;

    // currentInput() starts after the prompt; prompt text is never submitted.
    const QString command = currentInput().trimmed();

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n", normalFormat());

    inputStart = cursor.position();
    promptVisible = false;
    historyIndex = history.size();

    if (!command.isEmpty() && (history.isEmpty() || history.last() != command))
        history.append(command);

    completionIndex = -1;
    savedInput.clear();

    // Emit only the command text, never the rendered prompt.
    emit commandEntered(command);
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    protectPreviousOutput();

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        submitCommand();
        return;
    case Qt::Key_Up:
        navigateHistory(-1);
        return;
    case Qt::Key_Down:
        navigateHistory(1);
        return;
    case Qt::Key_Tab:
        autocomplete();
        return;
    case Qt::Key_Backspace:
        if (textCursor().position() <= inputStart)
            return;
        break;
    case Qt::Key_Home: {
        QTextCursor cursor = textCursor();
        cursor.setPosition(inputStart);
        setTextCursor(cursor);
        return;
    }
    default:
        break;
    }

    if (textCursor().position() < inputStart)
        moveCursorToEnd();

    completionIndex = -1;
    QPlainTextEdit::keyPressEvent(event);
}

TerminalWindow::TerminalWindow(QWidget *parent) : QMainWindow(parent), shell(this)
{
    setupUI();

    connect(terminal, &TerminalWidget::commandEntered,
            this, &TerminalWindow::executeCommand);
    connect(&shell, &Shell::outputReady,
            terminal, &TerminalWidget::appendOutput);
    connect(&shell, &Shell::errorReady,
            terminal, &TerminalWidget::appendError);

    connect(&shell, &Shell::promptChanged, this,
            [this](const QString &directory) {
        terminal->setPrompt(QString("orbit:%1 $").arg(directory));
        terminal->showPrompt();
    });

    connect(&shell, &Shell::shellExited, this, [this]() {
        terminal->removePrompt();
        terminal->setDisabled(true);
    });

    terminal->startSession(
        "Orbit Shell v0.1.0 — Developer environment ready. Type a command to begin.",
        QString("orbit:%1 $").arg(shell.currentDirectory()));
}

void TerminalWindow::setupUI()
{
    setWindowTitle("Orbit Terminal");
    resize(1100, 750);
    terminal = new TerminalWidget(this);
    setCentralWidget(terminal);
    setStyleSheet("QMainWindow { background: #20232D; }");
    terminal->setFocus();
}

void TerminalWindow::executeCommand(const QString &command)
{
    const QString cleanedCommand = command.trimmed();
    if (cleanedCommand.isEmpty())
        return;

    if (cleanedCommand == "clear") {
        terminal->startSession(
            "Orbit Shell v0.1.0 — ready.",
            QString("orbit:%1 $").arg(shell.currentDirectory()));
        return;
    }

    shell.executeLine(cleanedCommand);
}
