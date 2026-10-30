
#include "TerminalWindow.h"

#include <QKeyEvent>
#include <QTextCursor>
#include <QScrollBar>
#include <QFontDatabase>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QProcessEnvironment>
#include <QApplication>

#include <algorithm>

// MARK: - TerminalWidget

TerminalWidget::TerminalWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(false);
    setUndoRedoEnabled(false);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont font = QFontDatabase::systemFont(
        QFontDatabase::FixedFont);

    font.setPointSize(11);
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

    completions = {
        "cd", "pwd", "exit", "clear",
        "ls", "echo", "cat", "mkdir",
        "touch", "rm", "cp", "mv",
        "whoami", "date", "uname",
        "git", "cmake", "make",
        "python3", "g++", "clang++"};
}

void TerminalWidget::setPrompt(const QString &newPrompt)
{
    prompt = newPrompt;
}

void TerminalWidget::setCommandCompletions(
    const QStringList &commands)
{
    completions = commands;
}

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
    cursor.movePosition(
        QTextCursor::End,
        QTextCursor::KeepAnchor);

    return cursor.selectedText();
}

void TerminalWidget::replaceCurrentInput(
    const QString &text)
{
    QTextCursor cursor(document());

    cursor.setPosition(inputStart);
    cursor.movePosition(
        QTextCursor::End,
        QTextCursor::KeepAnchor);

    cursor.insertText(text);

    setTextCursor(cursor);
}

void TerminalWidget::appendOutput(const QString &text)
{
    QTextCursor cursor(document());

    cursor.movePosition(QTextCursor::End);

    // Temporarily separate output from the current input.
    cursor.insertText("\n");

    cursor.insertText(text);

    if (!text.endsWith('\n'))
        cursor.insertText("\n");

    cursor.insertText(prompt + " ");

    inputStart = cursor.position();

    setTextCursor(cursor);

    verticalScrollBar()->setValue(
        verticalScrollBar()->maximum());
}

void TerminalWidget::navigateHistory(int direction)
{
    if (history.isEmpty())
        return;

    if (direction < 0)
    {
        if (historyIndex == history.size())
            savedInput = currentInput();

        historyIndex = std::max(0, historyIndex - 1);
    }
    else
    {
        historyIndex = std::min(
            static_cast<int>(history.size()),
            historyIndex + 1);
    }

    if (historyIndex == history.size())
    {
        replaceCurrentInput(savedInput);
    }
    else
    {
        replaceCurrentInput(history.at(historyIndex));
    }

    moveCursorToEnd();
}

void TerminalWidget::autocomplete()
{
    QTextCursor cursor = textCursor();

    if (cursor.position() < inputStart)
        return;

    QString input = currentInput();

    int relativePosition = cursor.position() - inputStart;

    int tokenStart = relativePosition;

    while (tokenStart > 0 &&
           !input[tokenStart - 1].isSpace())
    {
        --tokenStart;
    }

    int tokenEnd = relativePosition;

    while (tokenEnd < input.size() &&
           !input[tokenEnd].isSpace())
    {
        ++tokenEnd;
    }

    const QString token = input.mid(
        tokenStart,
        relativePosition - tokenStart);

    if (token.isEmpty())
        return;

    QStringList matches;

    // First try filesystem path completion.
    const int slash = token.lastIndexOf('/');

    const QString directoryPart =
        slash >= 0 ? token.left(slash + 1) : QString();

    const QString filePrefix =
        slash >= 0 ? token.mid(slash + 1) : token;

    QDir directory;

    QString searchPath = directoryPart.isEmpty()
                             ? QDir::currentPath()
                             : directoryPart;

    if (QDir(searchPath).exists())
    {
        directory.setPath(searchPath);

        QFileInfoList entries = directory.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot,
            QDir::Name);

        for (const QFileInfo &entry : entries)
        {
            if (entry.fileName().startsWith(
                    filePrefix,
                    Qt::CaseInsensitive))
            {
                QString completed = directoryPart +
                                    entry.fileName();

                if (entry.isDir())
                    completed += "/";

                matches.append(completed);
            }
        }
    }

    // If no path matches, try command completion.
    if (matches.isEmpty() && slash < 0)
    {
        for (const QString &command : completions)
        {
            if (command.startsWith(
                    token,
                    Qt::CaseInsensitive))
            {
                matches.append(command);
            }
        }

        // Include executable files available on PATH.
        const QString pathValue =
            QProcessEnvironment::systemEnvironment()
                .value("PATH");

        const QStringList pathDirectories =
            pathValue.split(QDir::listSeparator());

        for (const QString &pathDirectory : pathDirectories)
        {
            QDir pathDir(pathDirectory);

            for (const QFileInfo &entry :
                 pathDir.entryInfoList(
                     QDir::Files | QDir::Executable,
                     QDir::Name))
            {
                if (entry.fileName().startsWith(
                        token,
                        Qt::CaseInsensitive))
                {
                    matches.append(entry.fileName());
                }
            }
        }
    }

    matches.removeDuplicates();
    matches.sort(Qt::CaseInsensitive);

    if (matches.isEmpty())
        return;

    // Cycle through matches on repeated Tab presses.
    if (matches.size() == 1)
    {
        QString completed = matches.first();

        input.replace(
            tokenStart,
            tokenEnd - tokenStart,
            completed);

        replaceCurrentInput(input);

        QTextCursor updated(document());

        updated.setPosition(
            inputStart + tokenStart + completed.length());

        setTextCursor(updated);

        completionIndex = -1;
        return;
    }

    ++completionIndex;

    if (completionIndex >= matches.size())
        completionIndex = 0;

    const QString completed = matches.at(completionIndex);

    input.replace(
        tokenStart,
        tokenEnd - tokenStart,
        completed);

    replaceCurrentInput(input);

    QTextCursor updated(document());

    updated.setPosition(
        inputStart + tokenStart + completed.length());

    setTextCursor(updated);

    if (completionIndex == 0)
    {
        // Show candidates without destroying the active input.
        QString candidateText =
            "\n";

        for (const QString &match : matches)
            candidateText += match + "    ";

        candidateText += "\n" + prompt + " " + input;

        QTextCursor end(document());

        end.movePosition(QTextCursor::End);
        end.insertText(candidateText);

        inputStart = document()->characterCount() - input.length() - 1;

        moveCursorToEnd();
    }
}

void TerminalWidget::protectPreviousOutput()
{
    QTextCursor cursor = textCursor();

    if (cursor.position() < inputStart)
    {
        cursor.setPosition(
            document()->characterCount() - 1);

        setTextCursor(cursor);
    }
}

void TerminalWidget::submitCommand()
{
    QString command = currentInput();

    QTextCursor cursor(document());

    cursor.movePosition(QTextCursor::End);

    cursor.insertText("\n");

    inputStart = cursor.position();

    historyIndex = history.size();

    if (!command.trimmed().isEmpty())
    {
        if (history.isEmpty() ||
            history.last() != command)
        {
            history.append(command);
        }
    }

    completionIndex = -1;
    savedInput.clear();

    emit commandEntered(command);
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();

    // Never allow editing previous output.
    protectPreviousOutput();

    switch (key)
    {
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
    {
        if (textCursor().position() <= inputStart)
            return;

        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    case Qt::Key_Home:
    {
        QTextCursor cursor = textCursor();

        cursor.setPosition(inputStart);

        setTextCursor(cursor);
        return;
    }

    default:
        break;
    }

    // Prevent modifications before the active prompt.
    if (textCursor().position() < inputStart)
        moveCursorToEnd();

    completionIndex = -1;

    QPlainTextEdit::keyPressEvent(event);
}

// MARK: - TerminalWindow

TerminalWindow::TerminalWindow(QWidget *parent)
    : QMainWindow(parent),
      shell(this)
{
    setupUI();

    connect(
        terminal,
        &TerminalWidget::commandEntered,
        this,
        &TerminalWindow::executeCommand);

    connect(
        &shell,
        &Shell::outputReady,
        terminal,
        &TerminalWidget::appendOutput);

    connect(
        &shell,
        &Shell::errorReady,
        terminal,
        &TerminalWidget::appendOutput);

    connect(
        &shell,
        &Shell::promptChanged,
        this,
        [this](const QString &directory)
        {
            terminal->setPrompt(
                QString("orbit:%1 $").arg(directory));

            terminal->appendOutput("");
        });

    connect(
        &shell,
        &Shell::shellExited,
        this,
        [this]()
        {
            terminal->setDisabled(true);
        });

    terminal->setPrompt(
        QString("orbit:%1 $").arg(shell.currentDirectory()));

    terminal->appendOutput(
        "Welcome to Orbit Shell v0.1.0\n"
        "A developer environment built for Orbit.\n"
        "Type commands below. Press TAB for completion.\n"
        "Use UP/DOWN arrows to navigate command history.\n\n");
}

void TerminalWindow::setupUI()
{
    setWindowTitle("Orbit Terminal");
    resize(1100, 750);

    terminal = new TerminalWidget(this);

    setCentralWidget(terminal);

    setStyleSheet(R"(
        QMainWindow {
            background: #20232D;
        }
    )");

    terminal->setFocus();
}

void TerminalWindow::executeCommand(
    const QString &command)
{
    if (command.trimmed().isEmpty())
    {
        terminal->appendOutput("");
        return;
    }

    if (command.trimmed() == "clear")
    {
        terminal->clear();

        terminal->setPrompt(
            QString("orbit:%1 $").arg(shell.currentDirectory()));

        terminal->appendOutput("");
        return;
    }

    shell.executeLine(command);
}