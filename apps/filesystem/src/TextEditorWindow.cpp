#include "TextEditorWindow.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QTextStream>
#include <QToolBar>
#include <QFontDatabase>
#include <QAction>
#include <QTimer>
#include <QTextDocument>
#include <QKeySequence>

TextEditorWindow::TextEditorWindow(const QString &filePath, QWidget *parent)
    : QMainWindow(parent), path(QFileInfo(filePath).absoluteFilePath())
{
    resize(850, 620);
    editor = new QPlainTextEdit(this);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(12);
    editor->setFont(font);
    editor->setTabStopDistance(4 * editor->fontMetrics().horizontalAdvance(' '));
    editor->setStyleSheet("QPlainTextEdit { background: #20232D; color: #E8ECF2; border: 0; padding: 14px; selection-background-color: #39445A; }");
    setCentralWidget(editor);

    auto *toolbar = addToolBar("Editor");
    toolbar->setMovable(false);
    QAction *saveAction = toolbar->addAction("Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] { saveFile(); });
    connect(editor, &QPlainTextEdit::modificationChanged, this, [this](bool modified) { updateTitle(modified); });

    if (!loadFile()) {
        QTimer::singleShot(0, this, &QWidget::close);
        return;
    }
    updateTitle();
}

bool TextEditorWindow::loadFile()
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Cannot open file", "Unable to read " + path + ":\n" + file.errorString());
        return false;
    }
    const QByteArray bytes = file.readAll();
    editor->setPlainText(QString::fromUtf8(bytes));
    editor->document()->setModified(false);
    return true;
}

bool TextEditorWindow::saveFile()
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Cannot save file", "Unable to save " + path + ":\n" + file.errorString());
        return false;
    }
    const QByteArray data = editor->toPlainText().toUtf8();
    if (file.write(data) != data.size() || !file.commit()) {
        QMessageBox::critical(this, "Cannot save file", "Failed to write " + path + ".");
        return false;
    }
    editor->document()->setModified(false);
    updateTitle();
    return true;
}

void TextEditorWindow::updateTitle(bool modified)
{
    setWindowTitle(QString("%1%2 — Orbit Text Editor")
                       .arg(QFileInfo(path).fileName(), modified ? " •" : ""));
}
