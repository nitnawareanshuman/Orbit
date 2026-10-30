#include "FileManagerWidget.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QAbstractItemView>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QFileIconProvider>

namespace
{
    class FileFilterModel final : public QSortFilterProxyModel
    {
    public:
        using QSortFilterProxyModel::QSortFilterProxyModel;

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
        {
            if (filterRegularExpression().pattern().isEmpty())
                return true;

            const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
            const auto *fsModel = qobject_cast<const QFileSystemModel *>(sourceModel());
            if (!fsModel)
                return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);

            const QString name = fsModel->fileName(index);
            if (name.contains(filterRegularExpression()))
                return true;

            // Keep parent folders visible when a matching descendant exists.
            for (int row = 0; row < sourceModel()->rowCount(index); ++row)
                if (filterAcceptsRow(row, index))
                    return true;

            return false;
        }
    };
}

static QToolButton *newButton(const QString &label, QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setText(label);
    button->setCursor(Qt::PointingHandCursor);
    button->setObjectName("toolbarButton");
    return button;
}

FileManagerWidget::FileManagerWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("fileManager");
    setMinimumSize(520, 360);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(16, 14, 16, 14);
    outer->setSpacing(12);

    auto *top = new QHBoxLayout;
    auto *title = new QLabel("Orbit Files", this);
    title->setObjectName("title");
    auto *subtitle = new QLabel("Browse and manage your files", this);
    subtitle->setObjectName("subtitle");
    auto *titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(2);
    titleColumn->addWidget(title);
    titleColumn->addWidget(subtitle);
    top->addLayout(titleColumn);
    top->addStretch();

    auto makeButton = [this](const QString &label, const QString &tip)
    {
        auto *button = new QToolButton(this);
        button->setText(label);
        button->setToolTip(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setObjectName("toolbarButton");
        return button;
    };

    auto *newFolderButton = makeButton("+ Folder", "Create a folder here");
    auto *newFileButton = makeButton("+ File", "Create an empty file here");
    auto *refreshButton = makeButton("↻", "Refresh this directory");
    top->addWidget(newFolderButton);
    top->addWidget(newFileButton);
    top->addWidget(refreshButton);
    outer->addLayout(top);

    auto *navigation = new QHBoxLayout;
    auto *homeButton = newButton("Home", this);
    auto *upButton = newButton("↑", this);
    pathEdit = new QLineEdit(this);
    pathEdit->setPlaceholderText("Enter a folder path and press Return");
    pathEdit->setClearButtonEnabled(true);
    navigation->addWidget(homeButton);
    navigation->addWidget(upButton);
    navigation->addWidget(pathEdit, 1);
    outer->addLayout(navigation);

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Search this directory…");
    searchEdit->setClearButtonEnabled(true);
    outer->addWidget(searchEdit);

    model = new QFileSystemModel(this);
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    model->setReadOnly(false);
    model->setRootPath(QDir::currentPath());

    proxy = new FileFilterModel(this);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);
    proxy->setRecursiveFilteringEnabled(true);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    tree = new QTreeView(this);
    tree->setModel(proxy);
    tree->setRootIndex(proxy->mapFromSource(model->index(QDir::currentPath())));
    tree->setAnimated(true);
    tree->setIndentation(20);
    tree->setSortingEnabled(true);
    tree->sortByColumn(0, Qt::AscendingOrder);
    tree->setUniformRowHeights(true);
    tree->setAlternatingRowColors(true);
    tree->setExpandsOnDoubleClick(true);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setColumnWidth(0, 330);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    outer->addWidget(tree, 1);

    statusLabel = new QLabel(this);
    statusLabel->setObjectName("status");
    outer->addWidget(statusLabel);

    watcher = new QFileSystemWatcher(this);

    connect(homeButton, &QToolButton::clicked, this, [this]
            { setRootPath(QDir::homePath()); });
    connect(upButton, &QToolButton::clicked, this, [this]
            {
        const QDir current(rootPath());
        setRootPath(current.filePath("..")); });
    connect(pathEdit, &QLineEdit::returnPressed, this, [this]
            { navigate(pathEdit->text()); });
    connect(searchEdit, &QLineEdit::textChanged, this, [this](const QString &text)
            {
        proxy->setFilterRegularExpression(QRegularExpression(
            QRegularExpression::escape(text), QRegularExpression::CaseInsensitiveOption));
        updateStatus(); });
    connect(refreshButton, &QToolButton::clicked, this, &FileManagerWidget::refreshCurrentDirectory);
    connect(newFolderButton, &QToolButton::clicked, this, &FileManagerWidget::createFolder);
    connect(newFileButton, &QToolButton::clicked, this, &FileManagerWidget::createFile);
    connect(watcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString &)
            { refreshCurrentDirectory(); });

    connect(tree, &QTreeView::doubleClicked, this, [this](const QModelIndex &proxyIndex)
            {
        const QModelIndex sourceIndex = proxy->mapToSource(proxyIndex);
        const QFileInfo info = model->fileInfo(sourceIndex);
        if (info.isFile())
            emit fileOpenRequested(info.absoluteFilePath()); });

    setStyleSheet(R"(
        QWidget#fileManager { background: #171B24; color: #E8ECF4; }
        QLabel#title { color: #F4F6FA; font-size: 20px; font-weight: 700; }
        QLabel#subtitle, QLabel#status { color: #929DAF; font-size: 11px; }
        QLineEdit {
            background: #222936; color: #E8ECF4; border: 1px solid #354052;
            border-radius: 7px; padding: 9px 11px; selection-background-color: #405779;
        }
        QLineEdit:focus { border: 1px solid #6C9FE8; }
        QToolButton {
            background: #293447; color: #EAF1FC; border: 1px solid #3B4A61;
            border-radius: 7px; padding: 8px 11px; font-weight: 600;
        }
        QToolButton:hover { background: #35445C; }
        QTreeView {
            background: #1D2330; alternate-background-color: #202735;
            color: #E1E7F0; border: 1px solid #303B4D; border-radius: 9px;
            padding: 7px; outline: 0; show-decoration-selected: 1;
        }
        QTreeView::item { padding: 7px 4px; border-radius: 4px; }
        QTreeView::item:hover { background: #2B3546; }
        QTreeView::item:selected { background: #344C6B; color: #FFFFFF; }
        QHeaderView::section {
            background: #252E3D; color: #AEB9C9; border: 0;
            border-bottom: 1px solid #394456; padding: 8px; font-weight: 600;
        }
    )");

    setRootPath(QDir::currentPath());
}

void FileManagerWidget::setRootPath(const QString &path)
{
    const QString absolute = QDir(path).absolutePath();
    const QFileInfo info(absolute);
    if (!info.exists() || !info.isDir())
    {
        QMessageBox::warning(this, "Orbit Files", "The selected folder does not exist or cannot be opened.");
        return;
    }

    const QString previous = rootPath();
    if (!previous.isEmpty() && watcher->directories().contains(previous))
        watcher->removePath(previous);

    model->setRootPath(absolute);
    const QModelIndex sourceRoot = model->index(absolute);
    if (!sourceRoot.isValid())
        return;

    tree->setRootIndex(proxy->mapFromSource(sourceRoot));
    pathEdit->setText(absolute);
    watcher->addPath(absolute);
    updateStatus();
}

QString FileManagerWidget::rootPath() const
{
    return pathEdit ? pathEdit->text() : QDir::currentPath();
}

void FileManagerWidget::navigate(const QString &path)
{
    QString expanded = path.trimmed();
    if (expanded == "~")
        expanded = QDir::homePath();
    else if (expanded.startsWith("~/"))
        expanded = QDir::home().filePath(expanded.mid(2));
    setRootPath(expanded);
}

void FileManagerWidget::refreshCurrentDirectory()
{
    const QString path = rootPath();
    if (path.isEmpty())
        return;

    // QFileSystemModel watches directories itself. Re-setting the root also
    // re-establishes the visible root after an external filesystem change.
    const QModelIndex index = model->index(path);
    if (index.isValid())
        tree->setRootIndex(proxy->mapFromSource(index));
    updateStatus();
}

void FileManagerWidget::createFolder()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(
                             this, "New Folder", "Folder name:", QLineEdit::Normal, QString(), &accepted)
                             .trimmed();
    if (!accepted || name.isEmpty())
        return;

    if (name == "." || name == ".." || name.contains('/'))
    {
        QMessageBox::warning(this, "New Folder", "Enter a valid folder name.");
        return;
    }

    if (!model->mkdir(model->index(rootPath()), name).isValid())
        QMessageBox::warning(this, "New Folder", "Could not create the folder. It may already exist.");
    refreshCurrentDirectory();
}

void FileManagerWidget::createFile()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(
                             this, "New File", "File name:", QLineEdit::Normal, QString(), &accepted)
                             .trimmed();
    if (!accepted || name.isEmpty())
        return;

    if (name == "." || name == ".." || name.contains('/'))
    {
        QMessageBox::warning(this, "New File", "Enter a valid file name.");
        return;
    }

    const QString path = QDir(rootPath()).filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly))
    {
        QMessageBox::warning(this, "New File", "Could not create the file. It may already exist or the folder is not writable.");
        return;
    }
    file.close();
    refreshCurrentDirectory();
}

void FileManagerWidget::updateStatus()
{
    const QModelIndex root = model->index(rootPath());
    const int count = model->rowCount(root);
    statusLabel->setText(QString("%1 items   •   %2").arg(count).arg(rootPath()));
}
