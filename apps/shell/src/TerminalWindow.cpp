#include "TerminalWindow.h"

#include <QTextCursor>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QScrollBar>
#include <QFontDatabase>

TerminalWindow::TerminalWindow(QWidget *parent)
    : QMainWindow(parent),
      shell(this),
      terminalOutput(new QPlainTextEdit(this)),
      commandInput(new QLineEdit(this)),
      promptLabel(new QLabel(this)),
      stopButton(new QPushButton("Stop", this))
{
    setupUI();

    connect(
        commandInput,
        &QLineEdit::returnPressed,
        this,
        &TerminalWindow::executeCommand);

    connect(
        &shell,
        &Shell::outputReady,
        this,
        &TerminalWindow::appendOutput);

    connect(
        &shell,
        &Shell::errorReady,
        this,
        [this](const QString &text)
        {
            appendOutput(text);
        });

    connect(
        &shell,
        &Shell::promptChanged,
        this,
        [this](const QString &directory)
        {
            promptLabel->setText(
                QString("orbit:%1 $").arg(directory));
        });

    connect(
        stopButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            shell.stop();
            commandInput->setEnabled(false);
        });

    promptLabel->setText(
        "orbit:" + shell.currentDirectory() + " $");

    appendOutput("Welcome to Orbit Shell.\n");
}

void TerminalWindow::setupUI()
{
    setWindowTitle("Orbit Shell");
    resize(1050, 700);

    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);

    terminalOutput->setReadOnly(true);
    terminalOutput->setLineWrapMode(
        QPlainTextEdit::NoWrap);

    QFont terminalFont =
        QFontDatabase::systemFont(
            QFontDatabase::FixedFont);

    terminalFont.setPointSize(11);
    terminalOutput->setFont(terminalFont);
    commandInput->setFont(terminalFont);

    promptLabel->setFont(terminalFont);

    QHBoxLayout *inputLayout = new QHBoxLayout();

    inputLayout->addWidget(promptLabel);
    inputLayout->addWidget(commandInput, 1);
    inputLayout->addWidget(stopButton);

    layout->addWidget(terminalOutput, 1);
    layout->addLayout(inputLayout);

    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow {
            background-color: #111318;
        }

        QPlainTextEdit {
            background-color: #111318;
            color: #E5E7EB;
            border: none;
            padding: 12px;
            selection-background-color: #334155;
        }

        QLabel {
            color: #55D6A5;
            padding-left: 8px;
        }

        QLineEdit {
            background-color: #1B1F27;
            color: #FFFFFF;
            border: 1px solid #343B48;
            border-radius: 5px;
            padding: 9px;
        }

        QPushButton {
            background-color: #252B35;
            color: white;
            padding: 8px 15px;
            border: none;
            border-radius: 5px;
        }

        QPushButton:hover {
            background-color: #394454;
        }
    )");
}

void TerminalWindow::executeCommand()
{
    const QString input = commandInput->text();

    if (input.trimmed().isEmpty())
        return;

    appendOutput(promptLabel->text() + " " + input + "\n");

    commandInput->clear();

    if (input.trimmed() == "exit")
    {
        shell.executeLine(input);
        commandInput->setEnabled(false);
        return;
    }

    shell.executeLine(input);
}

void TerminalWindow::appendOutput(const QString &text)
{
    terminalOutput->moveCursor(QTextCursor::End);
    terminalOutput->insertPlainText(text);
    terminalOutput->verticalScrollBar()->setValue(
        terminalOutput->verticalScrollBar()->maximum());
}