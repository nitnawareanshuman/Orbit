#pragma once

#include <QMainWindow>

class QPlainTextEdit;

class TextEditorWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit TextEditorWindow(const QString &filePath, QWidget *parent = nullptr);

private:
    QString path;
    QPlainTextEdit *editor = nullptr;
    bool loadFile();
    bool saveFile();
    void updateTitle(bool modified = false);
};
