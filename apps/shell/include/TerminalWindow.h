
#pragma once

#include "Shell.h"

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QStringList>

class TerminalWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);

    void appendOutput(const QString &text);
    void setPrompt(const QString &prompt);
    void setCommandCompletions(const QStringList &commands);

signals:
    void commandEntered(const QString &command);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QString prompt;
    QStringList history;
    QStringList completions;

    int historyIndex = 0;
    int inputStart = 0;
    int completionIndex = -1;

    QString savedInput;

    QString currentInput() const;
    void replaceCurrentInput(const QString &text);
    void navigateHistory(int direction);
    void autocomplete();
    void submitCommand();

    void moveCursorToEnd();
    void protectPreviousOutput();
};

class TerminalWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TerminalWindow(QWidget *parent = nullptr);

private:
    Shell shell;
    TerminalWidget *terminal = nullptr;

    void setupUI();
    void executeCommand(const QString &command);
};