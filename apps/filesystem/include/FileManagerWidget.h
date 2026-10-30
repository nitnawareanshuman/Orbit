#pragma once

#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QSortFilterProxyModel>
#include <QWidget>

class QLineEdit;
class QTreeView;
class QLabel;
class QToolButton;
class QSortFilterProxyModel;

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
    QSortFilterProxyModel *proxy = nullptr;
    QFileSystemWatcher *watcher = nullptr;
    QTreeView *tree = nullptr;
    QLineEdit *pathEdit = nullptr;
    QLineEdit *searchEdit = nullptr;
    QLabel *statusLabel = nullptr;

    void navigate(const QString &path);
    void refreshCurrentDirectory();
    void createFolder();
    void createFile();
    void updateStatus();
};
