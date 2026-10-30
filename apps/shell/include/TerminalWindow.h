#pragma once

#include "Shell.h"

#include <QMainWindow>

class QPlainTextEdit;
class QLineEdit;
class QLabel;
class QPushButton;

class TerminalWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TerminalWindow(QWidget *parent = nullptr);

private:
    Shell shell;

    QPlainTextEdit *terminalOutput;
    QLineEdit *commandInput;
    QLabel *promptLabel;
    QPushButton *stopButton;

    void setupUI();
    void executeCommand();
    void appendOutput(const QString &text);
};