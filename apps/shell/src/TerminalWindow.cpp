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
    completions = {"cd", "pwd", "exit", "clear", "jobs", "fg", "bg", "kill", "export", "unset",
                   "ls", "echo", "cat", "mkdir", "touch", "rm", "cp", "mv", "whoami", "date",
                   "uname", "git", "cmake", "make", "python3", "g++", "clang++"};
}

void TerminalWidget::setPrompt(const QString &newPrompt) { prompt = newPrompt; }
void TerminalWidget::setCommandCompletions(const QStringList &commands) { completions = commands; }
void TerminalWidget::moveCursorToEnd() { QTextCursor c = textCursor(); c.movePosition(QTextCursor::End); setTextCursor(c); }

QString TerminalWidget::currentInput() const
{
    QTextCursor c(document()); c.setPosition(inputStart);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    return c.selectedText();
}

void TerminalWidget::replaceCurrentInput(const QString &text)
{
    QTextCursor c(document()); c.setPosition(inputStart);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    c.insertText(text); setTextCursor(c);
}

void TerminalWidget::removePrompt()
{
    if (!promptVisible) return;
    QTextCursor c(document());
    c.setPosition(inputStart);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    promptVisible = false;
}

void TerminalWidget::showPrompt()
{
    if (promptVisible) return;
    QTextCursor c(document());
    c.movePosition(QTextCursor::End);
    if (c.position() > 0 && document()->characterAt(c.position() - 1) != QChar('\n'))
        c.insertText("\n");
    c.insertText(prompt + " ");
    inputStart = c.position();
    promptVisible = true;
    setTextCursor(c);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::appendOutput(const QString &text)
{
    if (text.isEmpty()) return;
    const QString pendingInput = promptVisible ? currentInput() : QString();
    removePrompt();
    QTextCursor c(document());
    c.movePosition(QTextCursor::End);
    QTextCharFormat normal;
    normal.setForeground(QColor("#FFFFFF"));
    c.insertText(text, normal);
    if (!text.endsWith('\n')) c.insertText("\n", normal);
    if (promptVisible) return;
    showPrompt();
    if (!pendingInput.isEmpty()) {
        QTextCursor end(document()); end.movePosition(QTextCursor::End);
        end.insertText(pendingInput);
        setTextCursor(end);
    }
}

void TerminalWidget::appendError(const QString &text)
{
    if (text.isEmpty()) return;
    const QString pendingInput = promptVisible ? currentInput() : QString();
    removePrompt();
    QTextCursor c(document());
    c.movePosition(QTextCursor::End);
    QTextCharFormat errorFormat;
    errorFormat.setForeground(QColor("#FF6B6B"));
    c.insertText(text, errorFormat);
    if (!text.endsWith('\n')) c.insertText("\n", errorFormat);
    showPrompt();
    if (!pendingInput.isEmpty()) {
        QTextCursor end(document()); end.movePosition(QTextCursor::End);
        end.insertText(pendingInput);
        setTextCursor(end);
    }
}

void TerminalWidget::startSession(const QString &welcome, const QString &newPrompt)
{
    clear(); prompt = newPrompt; promptVisible = false;
    QTextCursor c(document());
    QTextCharFormat normal; normal.setForeground(QColor("#FFFFFF"));
    c.insertText(welcome + "\n", normal);
    showPrompt();
}

void TerminalWidget::navigateHistory(int direction)
{
    if (history.isEmpty()) return;
    if (direction < 0) {
        if (historyIndex == history.size()) savedInput = currentInput();
        historyIndex = std::max(0, historyIndex - 1);
    } else historyIndex = std::min(static_cast<int>(history.size()), historyIndex + 1);
    replaceCurrentInput(historyIndex == history.size() ? savedInput : history.at(historyIndex));
    moveCursorToEnd();
}

void TerminalWidget::autocomplete()
{
    QTextCursor cursor = textCursor();
    if (cursor.position() < inputStart) return;
    QString input = currentInput();
    int pos = cursor.position() - inputStart, start = pos, end = pos;
    while (start > 0 && !input[start - 1].isSpace()) --start;
    while (end < input.size() && !input[end].isSpace()) ++end;
    const QString token = input.mid(start, pos - start);
    if (token.isEmpty()) return;
    QStringList matches;
    const int slash = token.lastIndexOf('/');
    const QString dirPart = slash >= 0 ? token.left(slash + 1) : QString();
    const QString prefix = slash >= 0 ? token.mid(slash + 1) : token;
    QDir dir(dirPart.isEmpty() ? QDir::currentPath() : dirPart);
    if (dir.exists()) {
        for (const QFileInfo &entry : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name)) {
            if (entry.fileName().startsWith(prefix, Qt::CaseInsensitive))
                matches << dirPart + entry.fileName() + (entry.isDir() ? "/" : "");
        }
    }
    if (matches.isEmpty() && slash < 0) {
        for (const QString &item : completions) if (item.startsWith(token, Qt::CaseInsensitive)) matches << item;
        const QString path = QProcessEnvironment::systemEnvironment().value("PATH");
        for (const QString &directory : path.split(QDir::listSeparator())) {
            QDir pathDir(directory);
            for (const QFileInfo &entry : pathDir.entryInfoList(QDir::Files | QDir::Executable, QDir::Name))
                if (entry.fileName().startsWith(token, Qt::CaseInsensitive)) matches << entry.fileName();
        }
    }
    matches.removeDuplicates(); matches.sort(Qt::CaseInsensitive);
    if (matches.isEmpty()) return;
    if (matches.size() > 1) {
        QString candidates = "\n" + matches.join("    ") + "\n";
        appendOutput(candidates);
        input = currentInput();
    }
    const QString replacement = matches.first();
    input.replace(start, end - start, replacement);
    replaceCurrentInput(input);
    QTextCursor updated(document()); updated.setPosition(inputStart + start + replacement.size()); setTextCursor(updated);
}

void TerminalWidget::protectPreviousOutput()
{
    if (textCursor().position() < inputStart) {
        QTextCursor c(document()); c.setPosition(document()->characterCount() - 1); setTextCursor(c);
    }
}

void TerminalWidget::submitCommand()
{
    const QString command = currentInput();
    QTextCursor c(document()); c.movePosition(QTextCursor::End); c.insertText("\n");
    inputStart = c.position(); promptVisible = false; historyIndex = history.size();
    if (!command.trimmed().isEmpty() && (history.isEmpty() || history.last() != command)) history.append(command);
    completionIndex = -1; savedInput.clear(); emit commandEntered(command);
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    protectPreviousOutput();
    switch (event->key()) {
    case Qt::Key_Return: case Qt::Key_Enter: submitCommand(); return;
    case Qt::Key_Up: navigateHistory(-1); return;
    case Qt::Key_Down: navigateHistory(1); return;
    case Qt::Key_Tab: autocomplete(); return;
    case Qt::Key_Backspace: if (textCursor().position() <= inputStart) return; break;
    case Qt::Key_Home: { QTextCursor c = textCursor(); c.setPosition(inputStart); setTextCursor(c); return; }
    default: break;
    }
    if (textCursor().position() < inputStart) moveCursorToEnd();
    completionIndex = -1;
    QPlainTextEdit::keyPressEvent(event);
}

TerminalWindow::TerminalWindow(QWidget *parent) : QMainWindow(parent), shell(this)
{
    setupUI();
    connect(terminal, &TerminalWidget::commandEntered, this, &TerminalWindow::executeCommand);
    connect(&shell, &Shell::outputReady, terminal, &TerminalWidget::appendOutput);
    connect(&shell, &Shell::errorReady, terminal, &TerminalWidget::appendError);
    connect(&shell, &Shell::promptChanged, this, [this](const QString &directory) {
        terminal->setPrompt(QString("orbit:%1 $").arg(directory));
        terminal->showPrompt();
    });
    connect(&shell, &Shell::shellExited, this, [this]() { terminal->removePrompt(); terminal->setDisabled(true); });
    terminal->startSession("Orbit Shell v0.1.0 — Developer environment ready. Type a command to begin.",
                           QString("orbit:%1 $").arg(shell.currentDirectory()));
}

void TerminalWindow::setupUI()
{
    setWindowTitle("Orbit Terminal"); resize(1100, 750);
    terminal = new TerminalWidget(this); setCentralWidget(terminal);
    setStyleSheet("QMainWindow { background: #20232D; }"); terminal->setFocus();
}

void TerminalWindow::executeCommand(const QString &command)
{
    if (command.trimmed().isEmpty()) return;
    if (command.trimmed() == "clear") {
        terminal->startSession("Orbit Shell v0.1.0 — ready.", QString("orbit:%1 $").arg(shell.currentDirectory()));
        return;
    }
    shell.executeLine(command);
}
