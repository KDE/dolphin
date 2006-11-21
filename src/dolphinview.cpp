/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "dolphinview.h"

#include <qlayout.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <QDropEvent>
#include <QMouseEvent>
#include <Q3VBoxLayout>
#include <kurl.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kio/renamedlg.h>
#include <assert.h>

#include "urlnavigator.h"
#include "dolphinstatusbar.h"
#include "dolphin.h"
#include "dolphindirlister.h"
#include "viewproperties.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "dolphincontextmenu.h"
#include "undomanager.h"
#include "renamedialog.h"
#include "progressindicator.h"

#include "filterbar.h"

DolphinView::DolphinView(QWidget *parent,
                         const KUrl& url,
                         Mode mode,
                         bool showHiddenFiles) :
    QWidget(parent),
    m_refreshing(false),
    m_showProgress(false),
    m_mode(mode),
    m_iconsView(0),
    m_detailsView(0),
    m_statusBar(0),
    m_iconSize(0),
    m_folderCount(0),
    m_fileCount(0),
    m_filterBar(0)
{
    setFocusPolicy(Qt::StrongFocus);
    m_topLayout = new Q3VBoxLayout(this);

    Dolphin& dolphin = Dolphin::mainWin();

    connect(this, SIGNAL(signalModeChanged()),
            &dolphin, SLOT(slotViewModeChanged()));
    connect(this, SIGNAL(signalShowHiddenFilesChanged()),
            &dolphin, SLOT(slotShowHiddenFilesChanged()));
    connect(this, SIGNAL(signalSortingChanged(DolphinView::Sorting)),
            &dolphin, SLOT(slotSortingChanged(DolphinView::Sorting)));
    connect(this, SIGNAL(signalSortOrderChanged(Qt::SortOrder)),
            &dolphin, SLOT(slotSortOrderChanged(Qt::SortOrder)));

    m_urlNavigator = new URLNavigator(url, this);
    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(slotURLChanged(const KUrl&)));
    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            &dolphin, SLOT(slotURLChanged(const KUrl&)));
    connect(m_urlNavigator, SIGNAL(historyChanged()),
            &dolphin, SLOT(slotHistoryChanged()));

    m_statusBar = new DolphinStatusBar(this);

    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setShowingDotFiles(showHiddenFiles);
    connect(m_dirLister, SIGNAL(clear()),
            this, SLOT(slotClear()));
    connect(m_dirLister, SIGNAL(percent(int)),
            this, SLOT(slotPercent(int)));
    connect(m_dirLister, SIGNAL(deleteItem(KFileItem*)),
            this, SLOT(slotDeleteItem(KFileItem*)));
    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(slotCompleted()));
    connect(m_dirLister, SIGNAL(infoMessage(const QString&)),
            this, SLOT(slotInfoMessage(const QString&)));
    connect(m_dirLister, SIGNAL(errorMessage(const QString&)),
            this, SLOT(slotErrorMessage(const QString&)));
    connect(m_dirLister, SIGNAL(refreshItems(const KFileItemList&)),
            this, SLOT(slotRefreshItems(const KFileItemList&)));
    connect(m_dirLister, SIGNAL(redirection(const KUrl&, const KUrl&)),
            this, SIGNAL(redirection(const KUrl&, const KUrl&)));
    connect(m_dirLister, SIGNAL(newItems(const KFileItemList&)),
           this, SLOT(slotAddItems(const KFileItemList&)));

    m_iconSize = K3Icon::SizeMedium;

    m_topLayout->addWidget(m_urlNavigator);
    createView();

    m_filterBar = new FilterBar(this);
    m_filterBar->hide();
    m_topLayout->addWidget(m_filterBar);
    connect(m_filterBar, SIGNAL(signalFilterChanged(const QString&)),
           this, SLOT(slotChangeNameFilter(const QString&)));

    m_topLayout->addWidget(m_statusBar);
}

DolphinView::~DolphinView()
{
    delete m_dirLister;
    m_dirLister = 0;
}

void DolphinView::setURL(const KUrl& url)
{
    m_urlNavigator->setURL(url);
}

const KUrl& DolphinView::url() const
{
    return m_urlNavigator->url();
}

void DolphinView::requestActivation()
{
    Dolphin::mainWin().setActiveView(this);
}

bool DolphinView::isActive() const
{
    return (Dolphin::mainWin().activeView() == this);
}

void DolphinView::setMode(Mode mode)
{
    if (mode == m_mode) {
        return;         // the wished mode is already set
    }

    QWidget* view = (m_iconsView != 0) ? static_cast<QWidget*>(m_iconsView) :
                                         static_cast<QWidget*>(m_detailsView);
    if (view != 0) {
        m_topLayout->remove(view);
        view->close();
        view->deleteLater();
        m_iconsView = 0;
        m_detailsView = 0;
    }

    m_mode = mode;

    createView();

    ViewProperties props(m_urlNavigator->url());
    props.setViewMode(m_mode);

    emit signalModeChanged();
}

DolphinView::Mode DolphinView::mode() const
{
    return m_mode;
}

void DolphinView::setShowHiddenFilesEnabled(bool show)
{
    if (m_dirLister->showingDotFiles() == show) {
        return;
    }

    ViewProperties props(m_urlNavigator->url());
    props.setShowHiddenFilesEnabled(show);
    props.save();

    m_dirLister->setShowingDotFiles(show);

    emit signalShowHiddenFilesChanged();

    reload();
}

bool DolphinView::isShowHiddenFilesEnabled() const
{
    return m_dirLister->showingDotFiles();
}

void DolphinView::setViewProperties(const ViewProperties& props)
{
    setMode(props.viewMode());
    setSorting(props.sorting());
    setSortOrder(props.sortOrder());
    setShowHiddenFilesEnabled(props.isShowHiddenFilesEnabled());
}

void DolphinView::renameSelectedItems()
{
    const KUrl::List urls = selectedURLs();
    if (urls.count() > 1) {
        // More than one item has been selected for renaming. Open
        // a rename dialog and rename all items afterwards.
        RenameDialog dialog(urls);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        DolphinView* view = Dolphin::mainWin().activeView();
        const QString& newName = dialog.newName();
        if (newName.isEmpty()) {
            view->statusBar()->setMessage(i18n("The new item name is invalid."),
                                          DolphinStatusBar::Error);
        }
        else {
            UndoManager& undoMan = UndoManager::instance();
            undoMan.beginMacro();

            assert(newName.contains('#'));

            const int urlsCount = urls.count();
            ProgressIndicator* progressIndicator =
                new  ProgressIndicator(i18n("Renaming items..."),
                                       i18n("Renaming finished."),
                                       urlsCount);

            // iterate through all selected items and rename them...
            const int replaceIndex = newName.find('#');
            assert(replaceIndex >= 0);
            for (int i = 0; i < urlsCount; ++i) {
                const KUrl& source = urls[i];
                QString name(newName);
                name.replace(replaceIndex, 1, renameIndexPresentation(i + 1, urlsCount));

                if (source.fileName() != name) {
                    KUrl dest(source.upUrl());
                    dest.addPath(name);

                    const bool destExists = KIO::NetAccess::exists(dest, false, view);
                    if (destExists) {
                        delete progressIndicator;
                        progressIndicator = 0;
                        view->statusBar()->setMessage(i18n("Renaming failed (item '%1' already exists).").arg(name),
                                                      DolphinStatusBar::Error);
                        break;
                    }
                    else if (KIO::NetAccess::file_move(source, dest)) {
                        // TODO: From the users point of view he executed one 'rename n files' operation,
                        // but internally we store it as n 'rename 1 file' operations for the undo mechanism.
                        DolphinCommand command(DolphinCommand::Rename, source, dest);
                        undoMan.addCommand(command);
                    }
                }

                progressIndicator->execOperation();
            }
            delete progressIndicator;
            progressIndicator = 0;

            undoMan.endMacro();
        }
    }
    else {
        // Only one item has been selected for renaming. Use the custom
        // renaming mechanism from the views.
        assert(urls.count() == 1);
        if (m_mode == DetailsView) {
            Q3ListViewItem* item = m_detailsView->firstChild();
            while (item != 0) {
                if (item->isSelected()) {
                    m_detailsView->rename(item, DolphinDetailsView::NameColumn);
                    break;
                }
                item = item->nextSibling();
            }
        }
        else {
            KFileIconViewItem* item = static_cast<KFileIconViewItem*>(m_iconsView->firstItem());
            while (item != 0) {
                if (item->isSelected()) {
                    item->rename();
                    break;
                }
                item = static_cast<KFileIconViewItem*>(item->nextItem());
            }
        }
    }
}

void DolphinView::selectAll()
{
    fileView()->selectAll();
}

void DolphinView::invertSelection()
{
    fileView()->invertSelection();
}

DolphinStatusBar* DolphinView::statusBar() const
{
    return m_statusBar;
}

int DolphinView::contentsX() const
{
    return scrollView()->contentsX();
}

int DolphinView::contentsY() const
{
    return scrollView()->contentsY();
}

void DolphinView::refreshSettings()
{
    if (m_iconsView != 0) {
        m_iconsView->refreshSettings();
    }

    if (m_detailsView != 0) {
        // TODO: There is no usable interface in QListView/KFileDetailView
        // to hide/show columns. The easiest approach is to delete
        // the current instance and recreate a new one, which automatically
        // refreshs the settings. If a proper interface is available in Qt4
        // m_detailsView->refreshSettings() would be enough.
        m_topLayout->remove(m_detailsView);
        m_detailsView->close();
        m_detailsView->deleteLater();
        m_detailsView = 0;

        createView();
    }
}

void DolphinView::updateStatusBar()
{
    // As the item count information is less important
    // in comparison with other messages, it should only
    // be shown if:
    // - the status bar is empty or
    // - shows already the item count information or
    // - shows only a not very important information
    // - if any progress is given don't show the item count info at all
    const QString msg(m_statusBar->message());
    const bool updateStatusBarMsg = (msg.isEmpty() ||
                                     (msg == m_statusBar->defaultText()) ||
                                     (m_statusBar->type() == DolphinStatusBar::Information)) &&
                                    (m_statusBar->progress() == 100);

    const QString text(hasSelection() ? selectionStatusBarText() : defaultStatusBarText());
    m_statusBar->setDefaultText(text);

    if (updateStatusBarMsg) {
        m_statusBar->setMessage(text, DolphinStatusBar::Default);
    }
}

void DolphinView::requestItemInfo(const KUrl& url)
{
    emit signalRequestItemInfo(url);
}

bool DolphinView::isURLEditable() const
{
    return m_urlNavigator->isURLEditable();
}

void DolphinView::zoomIn()
{
    itemEffectsManager()->zoomIn();
}

void DolphinView::zoomOut()
{
    itemEffectsManager()->zoomOut();
}

bool DolphinView::isZoomInPossible() const
{
    return itemEffectsManager()->isZoomInPossible();
}

bool DolphinView::isZoomOutPossible() const
{
    return itemEffectsManager()->isZoomOutPossible();
}

void DolphinView::setSorting(Sorting sorting)
{
    if (sorting != this->sorting()) {
        KFileView* view = fileView();
        int spec = view->sorting() & ~QDir::Name & ~QDir::Size & ~QDir::Time & ~QDir::Unsorted;

        switch (sorting) {
            case SortByName: spec = spec | QDir::Name; break;
            case SortBySize: spec = spec | QDir::Size; break;
            case SortByDate: spec = spec | QDir::Time; break;
            default: break;
        }

        ViewProperties props(url());
        props.setSorting(sorting);

        view->setSorting(static_cast<QDir::SortSpec>(spec));

        emit signalSortingChanged(sorting);
    }
}

DolphinView::Sorting DolphinView::sorting() const
{
    const QDir::SortSpec spec = fileView()->sorting();

    if (spec & QDir::Time) {
        return SortByDate;
    }

    if (spec & QDir::Size) {
        return SortBySize;
    }

    return SortByName;
}

void DolphinView::setSortOrder(Qt::SortOrder order)
{
    if (sortOrder() != order) {
        KFileView* view = fileView();
        int sorting = view->sorting();
        sorting = (order == Qt::Ascending) ? (sorting & ~QDir::Reversed) :
                                             (sorting | QDir::Reversed);

        ViewProperties props(url());
        props.setSortOrder(order);

        view->setSorting(static_cast<QDir::SortSpec>(sorting));

        emit signalSortOrderChanged(order);
    }
}

Qt::SortOrder DolphinView::sortOrder() const
{
    return fileView()->isReversed() ? Qt::Descending : Qt::Ascending;
}

void DolphinView::goBack()
{
    m_urlNavigator->goBack();
}

void DolphinView::goForward()
{
    m_urlNavigator->goForward();
}

void DolphinView::goUp()
{
    m_urlNavigator->goUp();
}

void DolphinView::goHome()
{
    m_urlNavigator->goHome();
}

void DolphinView::setURLEditable(bool editable)
{
    m_urlNavigator->editURL(editable);
}

const Q3ValueList<URLNavigator::HistoryElem> DolphinView::urlHistory(int& index) const
{
    return m_urlNavigator->history(index);
}

bool DolphinView::hasSelection() const
{
    const KFileItemList* list = selectedItems();
    return (list != 0) && !list->isEmpty();
}

const KFileItemList* DolphinView::selectedItems() const
{
    return fileView()->selectedItems();
}

KUrl::List DolphinView::selectedURLs() const
{
    KUrl::List urls;

    const KFileItemList* list = fileView()->selectedItems();
    if (list != 0) {
        KFileItemList::const_iterator it = list->begin();
        const KFileItemList::const_iterator end = list->end();
        KFileItem* item = 0;
        while (it != end) {
            urls.append(item->url());
            ++it;
        }
    }

    return urls;
}

const KFileItem* DolphinView::currentFileItem() const
{
    return fileView()->currentFileItem();
}

void DolphinView::openContextMenu(KFileItem* fileInfo, const QPoint& pos)
{
    DolphinContextMenu contextMenu(this, fileInfo, pos);
    contextMenu.open();
}

void DolphinView::rename(const KUrl& source, const QString& newName)
{
    bool ok = false;

    if (newName.isEmpty() || (source.fileName() == newName)) {
        return;
    }

    KUrl dest(source.upUrl());
    dest.addPath(newName);

    const bool destExists = KIO::NetAccess::exists(dest,
                                                   false,
                                                   Dolphin::mainWin().activeView());
    if (destExists) {
        // the destination already exists, hence ask the user
        // how to proceed...
        KIO::RenameDlg renameDialog(this,
                                    i18n("File Already Exists"),
                                    source.path(),
                                    dest.path(),
                                    KIO::M_OVERWRITE);
        switch (renameDialog.exec()) {
            case KIO::R_OVERWRITE:
                // the destination should be overwritten
                ok = KIO::NetAccess::file_move(source, dest, -1, true);
                break;

            case KIO::R_RENAME: {
                // a new name for the destination has been used
                KUrl newDest(renameDialog.newDestUrl());
                ok = KIO::NetAccess::file_move(source, newDest);
                break;
            }

            default:
                // the renaming operation has been canceled
                reload();
                return;
        }
    }
    else {
        // no destination exists, hence just move the file to
        // do the renaming
        ok = KIO::NetAccess::file_move(source, dest);
    }

    if (ok) {
        m_statusBar->setMessage(i18n("Renamed file '%1' to '%2'.").arg(source.fileName(), dest.fileName()),
                                DolphinStatusBar::OperationCompleted);

        DolphinCommand command(DolphinCommand::Rename, source, dest);
        UndoManager::instance().addCommand(command);
    }
    else {
        m_statusBar->setMessage(i18n("Renaming of file '%1' to '%2' failed.").arg(source.fileName(), dest.fileName()),
                                DolphinStatusBar::Error);
        reload();
    }
}

void DolphinView::reload()
{
    startDirLister(m_urlNavigator->url(), true);
}

void DolphinView::slotURLListDropped(QDropEvent* /* event */,
                                     const KUrl::List& urls,
                                     const KUrl& url)
{
    KUrl destination(url);
    if (destination.isEmpty()) {
        destination = m_urlNavigator->url();
    }
    else {
        // Check whether the destination URL is a directory. If this is not the
        // case, use the navigator URL as destination (otherwise the destination,
        // which represents a file, would be replaced by a copy- or move-operation).
        KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, destination);
        if (!fileItem.isDir()) {
            destination = m_urlNavigator->url();
        }
    }

    Dolphin::mainWin().dropURLs(urls, destination);
}

void DolphinView::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    Dolphin::mainWin().setActiveView(this);
}

void DolphinView::slotURLChanged(const KUrl& url)
{
    const ViewProperties props(url);
    setMode(props.viewMode());

    const bool showHiddenFiles = props.isShowHiddenFilesEnabled();
    setShowHiddenFilesEnabled(showHiddenFiles);
    m_dirLister->setShowingDotFiles(showHiddenFiles);

    setSorting(props.sorting());
    setSortOrder(props.sortOrder());

    startDirLister(url);

    // The selectionChanged signal is not emitted when a new view object is
    // created. The application does not care whether a view is represented by a
    // different instance, hence inform the application that the selection might have
    // changed so that it can update it's actions.
    Dolphin::mainWin().slotSelectionChanged();

    emit signalURLChanged(url);
}

void DolphinView::triggerIconsViewItem(Q3IconViewItem* item)
{
    /* KDE4-TODO:
    const Qt::ButtonState keyboardState = KApplication::keyboardMouseState();
    const bool isSelectionActive = ((keyboardState & Qt::ShiftModifier) > 0) ||
                                   ((keyboardState & Qt::ControlModifier) > 0);*/
    const bool isSelectionActive = false;
    if ((item != 0) && !isSelectionActive) {
        // Updating the URL must be done outside the scope of this slot,
        // as iconview items will get deleted.
        QTimer::singleShot(0, this, SLOT(updateURL()));
        Dolphin::mainWin().setActiveView(this);
    }
}

void DolphinView::triggerDetailsViewItem(Q3ListViewItem* item,
                                         const QPoint& pos,
                                         int /* column */)
{
    if (item == 0) {
        return;
    }

    if (m_detailsView->isOnFilename(item, pos)) {
        // Updating the URL must be done outside the scope of this slot,
        // as listview items will get deleted.
        QTimer::singleShot(0, this, SLOT(updateURL()));
        Dolphin::mainWin().setActiveView(this);
    }
    else {
        m_detailsView->clearSelection();
    }
}

void DolphinView::triggerDetailsViewItem(Q3ListViewItem* item)
{
    const QPoint pos(0, item->itemPos());
    triggerDetailsViewItem(item, pos, 0);
}

void DolphinView::updateURL()
{
    KFileView* fileView = (m_iconsView != 0) ? static_cast<KFileView*>(m_iconsView) :
                                               static_cast<KFileView*>(m_detailsView);

    KFileItem* fileItem = fileView->currentFileItem();
    if (fileItem == 0) {
        return;
    }

    if (fileItem->isDir()) {
        // Prefer the local path over the URL. This assures that the
        // volume space information is correct. Assuming that the URL is media:/sda1,
        // and the local path is /windows/C: For the URL the space info is related
        // to the root partition (and hence wrong) and for the local path the space
        // info is related to the windows partition (-> correct).
        const QString localPath(fileItem->localPath());
        if (localPath.isEmpty()) {
            setURL(fileItem->url());
        }
        else {
            setURL(KUrl(localPath));
        }
    }
    else {
        fileItem->run();
    }
}

void DolphinView::slotPercent(int percent)
{
    if (m_showProgress) {
        m_statusBar->setProgress(percent);
    }
}

void DolphinView::slotClear()
{
    fileView()->clearView();
    updateStatusBar();
}

void DolphinView::slotDeleteItem(KFileItem* item)
{
    fileView()->removeItem(item);
    updateStatusBar();
}

void DolphinView::slotCompleted()
{
    m_refreshing = true;

    KFileView* view = fileView();
    view->clearView();

    // TODO: in Qt4 the code should get a lot
    // simpler and nicer due to Interview...
    if (m_iconsView != 0) {
        m_iconsView->beginItemUpdates();
    }
    if (m_detailsView != 0) {
        m_detailsView->beginItemUpdates();
    }

    if (m_showProgress) {
        m_statusBar->setProgressText(QString::null);
        m_statusBar->setProgress(100);
        m_showProgress = false;
    }

    KFileItemList items(m_dirLister->items());
    KFileItemList::const_iterator it = items.begin();
    const KFileItemList::const_iterator end = items.end();

    m_fileCount = 0;
    m_folderCount = 0;

    KFileItem* item = 0;
    while (it != end) {
        view->insertItem(item);
        if (item->isDir()) {
            ++m_folderCount;
        }
        else {
            ++m_fileCount;
        }
        ++it;
    }

    updateStatusBar();

    if (m_iconsView != 0) {
        // Prevent a flickering of the icon view widget by giving a small
        // timeslot to swallow asynchronous update events.
        m_iconsView->setUpdatesEnabled(false);
        QTimer::singleShot(10, this, SLOT(slotDelayedUpdate()));
    }

    if (m_detailsView != 0) {
        m_detailsView->endItemUpdates();
        m_refreshing = false;
    }
}

void DolphinView::slotDelayedUpdate()
{
    if (m_iconsView != 0) {
        m_iconsView->setUpdatesEnabled(true);
        m_iconsView->endItemUpdates();
    }
    m_refreshing = false;
}

void DolphinView::slotInfoMessage(const QString& msg)
{
    m_statusBar->setMessage(msg, DolphinStatusBar::Information);
}

void DolphinView::slotErrorMessage(const QString& msg)
{
    m_statusBar->setMessage(msg, DolphinStatusBar::Error);
}

void DolphinView::slotRefreshItems(const KFileItemList& /* list */)
{
    QTimer::singleShot(0, this, SLOT(reload()));
}

void DolphinView::slotAddItems(const KFileItemList& list)
{
  fileView()->addItemList(list);
  fileView()->updateView();
}

void DolphinView::slotGrabActivation()
{
    Dolphin::mainWin().setActiveView(this);
}

void DolphinView::slotContentsMoving(int x, int y)
{
    if (!m_refreshing) {
        // Only emit a 'contents moved' signal if the user
        // moved the content by adjusting the sliders. Adjustments
        // resulted by refreshing a directory should not be respected.
        emit contentsMoved(x, y);
    }
}

void DolphinView::createView()
{
    assert(m_iconsView == 0);
    assert(m_detailsView == 0);

    switch (m_mode) {
    case IconsView:
    case PreviewsView: {
        const DolphinIconsView::LayoutMode layoutMode = (m_mode == IconsView) ?
                                                        DolphinIconsView::Icons :
                                                        DolphinIconsView::Previews;
        m_iconsView = new DolphinIconsView(this, layoutMode);
        m_topLayout->insertWidget(1, m_iconsView);
        setFocusProxy(m_iconsView);

        connect(m_iconsView, SIGNAL(executed(Q3IconViewItem*)),
                this, SLOT(triggerIconsViewItem(Q3IconViewItem*)));
        connect(m_iconsView, SIGNAL(returnPressed(Q3IconViewItem*)),
                this, SLOT(triggerIconsViewItem(Q3IconViewItem*)));
        connect(m_iconsView, SIGNAL(signalRequestActivation()),
                this, SLOT(slotGrabActivation()));

        m_iconsView->endItemUpdates();
        m_iconsView->show();
        m_iconsView->setFocus();
        break;
    }

    case DetailsView: {
        m_detailsView = new DolphinDetailsView(this);
        m_topLayout->insertWidget(1, m_detailsView);
        setFocusProxy(m_detailsView);

        connect(m_detailsView, SIGNAL(executed(Q3ListViewItem*, const QPoint&, int)),
                this, SLOT(triggerDetailsViewItem(Q3ListViewItem*, const QPoint&, int)));
        connect(m_detailsView, SIGNAL(returnPressed(Q3ListViewItem*)),
                this, SLOT(triggerDetailsViewItem(Q3ListViewItem*)));
        connect(m_detailsView, SIGNAL(signalRequestActivation()),
                this, SLOT(slotGrabActivation()));
        m_detailsView->show();
        m_detailsView->setFocus();
        break;
    }

    default:
        break;
    }

    connect(scrollView(), SIGNAL(contentsMoving(int, int)),
            this, SLOT(slotContentsMoving(int, int)));

    startDirLister(m_urlNavigator->url());
}

KFileView* DolphinView::fileView() const
{
    return (m_mode == DetailsView) ? static_cast<KFileView*>(m_detailsView) :
                                     static_cast<KFileView*>(m_iconsView);
}

Q3ScrollView* DolphinView::scrollView() const
{
    return (m_mode == DetailsView) ? static_cast<Q3ScrollView*>(m_detailsView) :
                                     static_cast<Q3ScrollView*>(m_iconsView);
}

ItemEffectsManager* DolphinView::itemEffectsManager() const
{
    return (m_mode == DetailsView) ? static_cast<ItemEffectsManager*>(m_detailsView) :
                                     static_cast<ItemEffectsManager*>(m_iconsView);
}

void DolphinView::startDirLister(const KUrl& url, bool reload)
{
    if (!url.isValid()) {
        const QString location(url.pathOrUrl());
        if (location.isEmpty()) {
            m_statusBar->setMessage(i18n("The location is empty."), DolphinStatusBar::Error);
        }
        else {
            m_statusBar->setMessage(i18n("The location '%1' is invalid.").arg(location),
                                    DolphinStatusBar::Error);
        }
        return;
    }

    // Only show the directory loading progress if the status bar does
    // not contain another progress information. This means that
    // the directory loading progress information has the lowest priority.
    const QString progressText(m_statusBar->progressText());
    m_showProgress = progressText.isEmpty() ||
                     (progressText == i18n("Loading directory..."));
    if (m_showProgress) {
        m_statusBar->setProgressText(i18n("Loading directory..."));
        m_statusBar->setProgress(0);
    }

    m_refreshing = true;
    m_dirLister->stop();
    m_dirLister->openUrl(url, false, reload);
}

QString DolphinView::defaultStatusBarText() const
{
    // TODO: the following code is not suitable for languages where multiple forms
    // of plurals are given (e. g. in Poland three forms of plurals exist).
    const int itemCount = m_folderCount + m_fileCount;

    QString text;
    if (itemCount == 1) {
        text = i18n("1 Item");
    }
    else {
        text = i18n("%1 Items").arg(itemCount);
    }

    text += " (";

    if (m_folderCount == 1) {
        text += i18n("1 Folder");
    }
    else {
        text += i18n("%1 Folders").arg(m_folderCount);
    }

    text += ", ";

    if (m_fileCount == 1) {
        text += i18n("1 File");
    }
    else {
        text += i18n("%1 Files").arg(m_fileCount);
    }

    text += ")";

    return text;
}

QString DolphinView::selectionStatusBarText() const
{
    // TODO: the following code is not suitable for languages where multiple forms
    // of plurals are given (e. g. in Poland three forms of plurals exist).
    QString text;
    const KFileItemList* list = selectedItems();
    assert((list != 0) && !list->isEmpty());

    int fileCount = 0;
    int folderCount = 0;
    KIO::filesize_t byteSize = 0;
    KFileItemList::const_iterator it = list->begin();
    const KFileItemList::const_iterator end = list->end();
    while (it != end){
        KFileItem* item = *it;
        if (item->isDir()) {
            ++folderCount;
        }
        else {
            ++fileCount;
            byteSize += item->size();
        }
        ++it;
    }

    if (folderCount == 1) {
        text = i18n("1 Folder selected");
    }
    else if (folderCount > 1) {
        text = i18n("%1 Folders selected").arg(folderCount);
    }

    if ((fileCount > 0) && (folderCount > 0)) {
        text += ", ";
    }

    const QString sizeText(KIO::convertSize(byteSize));
    if (fileCount == 1) {
        text += i18n("1 File selected (%1)").arg(sizeText);
    }
    else if (fileCount > 1) {
        text += i18n("%1 Files selected (%1)").arg(fileCount).arg(sizeText);
    }

    return text;
}

QString DolphinView::renameIndexPresentation(int index, int itemCount) const
{
    // assure that the string reprentation for all indicess have the same
    // number of characters based on the given number of items
    QString str(QString::number(index));
    int chrCount = 1;
    while (itemCount >= 10) {
        ++chrCount;
        itemCount /= 10;
    }
    str.reserve(chrCount);

    const int insertCount = chrCount - str.length();
    for (int i = 0; i < insertCount; ++i) {
        str.insert(0, '0');
    }
    return str;
}

void DolphinView::slotShowFilterBar(bool show)
{
    assert(m_filterBar != 0);
    if (show) {
        m_filterBar->show();
    }
    else {
        m_filterBar->hide();
    }
}

void DolphinView::slotChangeNameFilter(const QString& nameFilter)
{
    // The name filter of KDirLister does a 'hard' filtering, which
    // means that only the items are shown where the names match
    // exactly the filter. This is non-transparent for the user, which
    // just wants to have a 'soft' filtering: does the name contain
    // the filter string?
    QString adjustedFilter(nameFilter);
    adjustedFilter.insert(0, '*');
    adjustedFilter.append('*');

    m_dirLister->setNameFilter(adjustedFilter);
    m_dirLister->emitChanges();

    // TODO: this is a workaround for QIconView: the item position
    // stay as they are by filtering, only an inserting of an item
    // results to an automatic adjusting of the item position. In Qt4/KDE4
    // this workaround should get obsolete due to Interview.
    KFileView* view = fileView();
    if (view == m_iconsView) {
        KFileItem* first = view->firstFileItem();
        if (first != 0) {
            view->removeItem(first);
            view->insertItem(first);
        }
    }
}

bool DolphinView::isFilterBarVisible()
{
  return m_filterBar->isVisible();
}

#include "dolphinview.moc"
