#include "FileManagerWidget.h"

#include <QFileInfo>
#include <QDir>
#include <QLabel>
#include <QItemSelectionModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QToolBar>
#include <QTreeView>
#include <QToolButton>
#include <QVBoxLayout>

FileManagerWidget::FileManagerWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("fileManager");
    setMinimumWidth(280);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *header = new QHBoxLayout;
    auto *title = new QLabel("Files", this);
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #F2F3F5;");
    auto *refresh = new QToolButton(this);
    refresh->setText("↻");
    refresh->setToolTip("Refresh");
    refresh->setStyleSheet("QToolButton { color: #D7DCE5; font-size: 18px; border: 0; padding: 4px; }");
    header->addWidget(title);
    header->addStretch();
    header->addWidget(refresh);
    layout->addLayout(header);

    auto *locationRow = new QHBoxLayout;
    pathEdit = new QLineEdit(this);
    pathEdit->setPlaceholderText("Folder path");
    pathEdit->setStyleSheet("QLineEdit { background: #20232D; color: #E8ECF2; border: 1px solid #363D4B; border-radius: 6px; padding: 7px; }");
    auto *go = new QPushButton("Go", this);
    go->setStyleSheet("QPushButton { background: #30394A; color: #F2F3F5; border: 0; border-radius: 6px; padding: 7px 10px; }");
    locationRow->addWidget(pathEdit, 1);
    locationRow->addWidget(go);
    layout->addLayout(locationRow);

    model = new QFileSystemModel(this);
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable);
    model->setRootPath(QDir::rootPath());
    tree = new QTreeView(this);
    tree->setModel(model);
    tree->setRootIndex(model->index(QDir::currentPath()));
    tree->setAnimated(true);
    tree->setIndentation(18);
    tree->setSortingEnabled(true);
    tree->sortByColumn(0, Qt::AscendingOrder);
    tree->setUniformRowHeights(true);
    tree->setHeaderHidden(false);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tree->setStyleSheet(R"(
        QTreeView { background: #20232D; color: #E8ECF2; border: 1px solid #343B49; border-radius: 7px; padding: 4px; }
        QTreeView::item { padding: 5px 3px; }
        QTreeView::item:hover { background: #2A3140; }
        QTreeView::item:selected { background: #39445A; color: white; }
        QHeaderView::section { background: #292E3A; color: #AEB8C8; border: 0; padding: 6px; }
    )");
    layout->addWidget(tree, 1);

    connect(go, &QPushButton::clicked, this, [this] { navigate(pathEdit->text()); });
    connect(pathEdit, &QLineEdit::returnPressed, this, [this] { navigate(pathEdit->text()); });
    connect(refresh, &QToolButton::clicked, this, [this] {
        const QString path = rootPath();
        setRootPath(path);
    });
    connect(tree, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        const QFileInfo info = model->fileInfo(index);
        if (info.isDir()) {
            setRootPath(info.absoluteFilePath());
        } else if (info.isFile()) {
            emit fileOpenRequested(info.absoluteFilePath());
        }
    });
    connect(tree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &FileManagerWidget::updatePathLabel);

    setStyleSheet("#fileManager { background: #252A35; }");
    setRootPath(QDir::currentPath());
}

void FileManagerWidget::setRootPath(const QString &path)
{
    const QString absolute = QDir(path).absolutePath();
    model->setRootPath(absolute);
    const QModelIndex index = model->index(absolute);
    if (!index.isValid())
        return;
    tree->setRootIndex(index);
    pathEdit->setText(absolute);
}

QString FileManagerWidget::rootPath() const
{
    return model->filePath(tree->rootIndex());
}

void FileManagerWidget::navigate(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        pathEdit->setStyleSheet("QLineEdit { background: #20232D; color: #FF6972; border: 1px solid #FF5C65; border-radius: 6px; padding: 7px; }");
        return;
    }
    pathEdit->setStyleSheet("QLineEdit { background: #20232D; color: #E8ECF2; border: 1px solid #363D4B; border-radius: 6px; padding: 7px; }");
    setRootPath(info.absoluteFilePath());
}

void FileManagerWidget::updatePathLabel(const QModelIndex &index)
{
    if (index.isValid())
        pathEdit->setToolTip(model->filePath(index));
}
