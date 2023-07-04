/*
 * SPDX-FileCopyrightText: 2006-2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "treeviewcontextmenu.h"

#include "folderspanel.h"
#include "global.h"

#include <KConfigGroup>
#include <KFileItemListProperties>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Paste>
#include <KIO/PasteJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KPropertiesDialog>
#include <KSharedConfig>
#include <KUrlMimeData>

#include <kio_version.h>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 100, 0)
#include <KIO/DeleteOrTrashJob>
#else
#include <KIO/FileUndoManager>
#include <KIO/JobUiDelegate>
#endif

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QMimeData>
#include <QPointer>

TreeViewContextMenu::TreeViewContextMenu(FoldersPanel *parent, const KFileItem &fileInfo)
    : QObject(parent)
    , m_parent(parent)
    , m_fileItem(fileInfo)
{
}

TreeViewContextMenu::~TreeViewContextMenu()
{
}

void TreeViewContextMenu::open(const QPoint &pos)
{
    QMenu *popup = new QMenu(m_parent);

    if (!m_fileItem.isNull()) {
        KFileItemListProperties capabilities(KFileItemList() << m_fileItem);

        // insert 'Cut', 'Copy' and 'Paste'
        QAction *cutAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-cut")), i18nc("@action:inmenu", "Cut"), this);
        cutAction->setEnabled(capabilities.supportsMoving());
        connect(cutAction, &QAction::triggered, this, &TreeViewContextMenu::cut);

        QAction *copyAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18nc("@action:inmenu", "Copy"), this);
        connect(copyAction, &QAction::triggered, this, &TreeViewContextMenu::copy);

        const QMimeData *mimeData = QApplication::clipboard()->mimeData();
        bool canPaste;
        const QString text = KIO::pasteActionText(mimeData, &canPaste, m_fileItem);
        QAction *pasteAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-paste")), text, this);
        connect(pasteAction, &QAction::triggered, this, &TreeViewContextMenu::paste);
        pasteAction->setEnabled(canPaste);

        popup->addAction(cutAction);
        popup->addAction(copyAction);
        popup->addAction(pasteAction);
        popup->addSeparator();

        // insert 'Rename'
        QAction *renameAction = new QAction(i18nc("@action:inmenu", "Rename…"), this);
        renameAction->setEnabled(capabilities.supportsMoving());
        renameAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
        connect(renameAction, &QAction::triggered, this, &TreeViewContextMenu::rename);
        popup->addAction(renameAction);

        // insert 'Move to Trash' and (optionally) 'Delete'
        KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::IncludeGlobals);
        KConfigGroup configGroup(globalConfig, "KDE");
        bool showDeleteCommand = configGroup.readEntry("ShowDeleteCommand", false);

        const QUrl url = m_fileItem.url();
        if (url.isLocalFile()) {
            QAction *moveToTrashAction = new QAction(QIcon::fromTheme(QStringLiteral("user-trash")), i18nc("@action:inmenu", "Move to Trash"), this);
            const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();
            moveToTrashAction->setEnabled(enableMoveToTrash);
            connect(moveToTrashAction, &QAction::triggered, this, &TreeViewContextMenu::moveToTrash);
            popup->addAction(moveToTrashAction);
        } else {
            showDeleteCommand = true;
        }

        if (showDeleteCommand) {
            QAction *deleteAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action:inmenu", "Delete"), this);
            deleteAction->setEnabled(capabilities.supportsDeleting());
            connect(deleteAction, &QAction::triggered, this, &TreeViewContextMenu::deleteItem);
            popup->addAction(deleteAction);
        }

        popup->addSeparator();
    }

    // insert 'Show Hidden Files'
    QAction *showHiddenFilesAction = new QAction(i18nc("@action:inmenu", "Show Hidden Files"), this);
    showHiddenFilesAction->setCheckable(true);
    showHiddenFilesAction->setChecked(m_parent->showHiddenFiles());
    popup->addAction(showHiddenFilesAction);
    connect(showHiddenFilesAction, &QAction::toggled, this, &TreeViewContextMenu::setShowHiddenFiles);

    if (!m_fileItem.isNull()) {
        // insert 'Limit to Home Directory'
        const QUrl url = m_fileItem.url();
        const bool enableLimitToHomeDirectory = url.isLocalFile();
        QAction *limitFoldersPanelToHomeAction = new QAction(i18nc("@action:inmenu", "Limit to Home Directory"), this);
        limitFoldersPanelToHomeAction->setCheckable(true);
        limitFoldersPanelToHomeAction->setEnabled(enableLimitToHomeDirectory);
        limitFoldersPanelToHomeAction->setChecked(m_parent->limitFoldersPanelToHome());
        popup->addAction(limitFoldersPanelToHomeAction);
        connect(limitFoldersPanelToHomeAction, &QAction::toggled, this, &TreeViewContextMenu::setLimitFoldersPanelToHome);
    }

    // insert 'Automatic Scrolling'
    QAction *autoScrollingAction = new QAction(i18nc("@action:inmenu", "Automatic Scrolling"), this);
    autoScrollingAction->setCheckable(true);
    autoScrollingAction->setChecked(m_parent->autoScrolling());
    // TODO: Temporary disabled. Horizontal autoscrolling will be implemented later either
    // in KItemViews or manually as part of the FoldersPanel
    //popup->addAction(autoScrollingAction);
    connect(autoScrollingAction, &QAction::toggled, this, &TreeViewContextMenu::setAutoScrolling);

    if (!m_fileItem.isNull()) {
        // insert 'Properties' entry
        QAction *propertiesAction = new QAction(i18nc("@action:inmenu", "Properties"), this);
        propertiesAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
        connect(propertiesAction, &QAction::triggered, this, &TreeViewContextMenu::showProperties);
        popup->addAction(propertiesAction);
    }

    const QList<QAction *> customActions = m_parent->customContextMenuActions();
    if (!customActions.isEmpty()) {
        popup->addSeparator();
        for (QAction *action : customActions) {
            popup->addAction(action);
        }
    }

    QPointer<QMenu> popupPtr = popup;
    popup->exec(pos);
    if (popupPtr.data()) {
        popupPtr.data()->deleteLater();
    }
}

void TreeViewContextMenu::populateMimeData(QMimeData *mimeData, bool cut)
{
    QList<QUrl> kdeUrls;
    kdeUrls.append(m_fileItem.url());
    QList<QUrl> mostLocalUrls;
    bool dummy;
    mostLocalUrls.append(m_fileItem.mostLocalUrl(&dummy));
    KIO::setClipboardDataCut(mimeData, cut);
    KUrlMimeData::exportUrlsToPortal(mimeData);
    KUrlMimeData::setUrls(kdeUrls, mostLocalUrls, mimeData);
}

void TreeViewContextMenu::cut()
{
    QMimeData *mimeData = new QMimeData();
    populateMimeData(mimeData, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::copy()
{
    QMimeData *mimeData = new QMimeData();
    populateMimeData(mimeData, false);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::paste()
{
    KIO::PasteJob *job = KIO::paste(QApplication::clipboard()->mimeData(), m_fileItem.url());
    KJobWidgets::setWindow(job, m_parent);
}

void TreeViewContextMenu::rename()
{
    m_parent->rename(m_fileItem);
}

void TreeViewContextMenu::moveToTrash()
{
#if KIO_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    using Iface = KIO::AskUserActionInterface;
    auto *deleteJob = new KIO::DeleteOrTrashJob(QList{m_fileItem.url()}, Iface::Trash, Iface::DefaultConfirmation, m_parent);
    deleteJob->start();
#else
    const QList<QUrl> list{m_fileItem.url()};
    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(m_parent);
    if (uiDelegate.askDeleteConfirmation(list, KIO::JobUiDelegate::Trash, KIO::JobUiDelegate::DefaultConfirmation)) {
        KIO::Job *job = KIO::trash(list);
        KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Trash, list, QUrl(QStringLiteral("trash:/")), job);
        KJobWidgets::setWindow(job, m_parent);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
#endif
}

void TreeViewContextMenu::deleteItem()
{
#if KIO_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    using Iface = KIO::AskUserActionInterface;
    auto *deleteJob = new KIO::DeleteOrTrashJob(QList{m_fileItem.url()}, Iface::Delete, Iface::DefaultConfirmation, m_parent);
    deleteJob->start();
#else
    const QList<QUrl> list{m_fileItem.url()};
    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(m_parent);
    if (uiDelegate.askDeleteConfirmation(list, KIO::JobUiDelegate::Delete, KIO::JobUiDelegate::DefaultConfirmation)) {
        KIO::Job *job = KIO::del(list);
        KJobWidgets::setWindow(job, m_parent);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
#endif
}

void TreeViewContextMenu::showProperties()
{
    KPropertiesDialog *dialog = new KPropertiesDialog(m_fileItem.url(), m_parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TreeViewContextMenu::setShowHiddenFiles(bool show)
{
    m_parent->setShowHiddenFiles(show);
}

void TreeViewContextMenu::setLimitFoldersPanelToHome(bool enable)
{
    m_parent->setLimitFoldersPanelToHome(enable);
}

void TreeViewContextMenu::setAutoScrolling(bool enable)
{
    m_parent->setAutoScrolling(enable);
}

#include "moc_treeviewcontextmenu.cpp"
