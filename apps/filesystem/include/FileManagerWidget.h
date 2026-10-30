#pragma once

#include <QFileSystemModel>
#include <QWidget>
#include <QIcon>
#include <QFileInfo>

class QLineEdit;
class QTreeView;
class QToolButton;

class FileManagerWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit FileManagerWidget(QWidget *parent = nullptr);
    void setRootPath(const QString &path);
    QString rootPath() const;

signals:
    void fileOpenRequested(const QString &absolutePath);

private:
    QFileSystemModel *model = nullptr;
    QTreeView *tree = nullptr;
    QLineEdit *pathEdit = nullptr;
    void navigate(const QString &path);
    void updatePathLabel(const QModelIndex &index);
};
