/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "dolphinviewcontainer.h"

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QItemSelection>
#include <QtGui/QBoxLayout>
#include <QtCore/QTimer>
#include <QtGui/QScrollBar>

#include <kdirmodel.h>
#include <kfileitemdelegate.h>
#include <kfileplacesmodel.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kiconeffect.h>
#include <kio/netaccess.h>
#include <kio/renamedialog.h>
#include <kio/previewjob.h>
#include <kmimetyperesolver.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <kurl.h>

#include "dolphincolumnview.h"
#include "dolphincontroller.h"
#include "dolphinstatusbar.h"
#include "dolphinmainwindow.h"
#include "dolphindirlister.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "dolphincontextmenu.h"
#include "dolphinitemcategorizer.h"
#include "filterbar.h"
#include "renamedialog.h"
#include "kurlnavigator.h"
#include "viewproperties.h"
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

DolphinViewContainer::DolphinViewContainer(DolphinMainWindow* mainWindow,
                                           QWidget* parent,
                                           const KUrl& url,
                                           DolphinView::Mode mode,
                                           bool showHiddenFiles) :
    QWidget(parent),
    m_showProgress(false),
    m_folderCount(0),
    m_fileCount(0),
    m_mainWindow(mainWindow),
    m_topLayout(0),
    m_urlNavigator(0),
    m_view(0),
    m_filterBar(0),
    m_statusBar(0),
    m_dirModel(0),
    m_dirLister(0),
    m_proxyModel(0)
{
    hide();
    setFocusPolicy(Qt::StrongFocus);
    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    connect(m_mainWindow, SIGNAL(activeViewChanged()),
            this, SLOT(updateActivationState()));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updateCutItems()));

    m_urlNavigator = new KUrlNavigator(DolphinSettings::instance().placesModel(), url, this);

    const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_urlNavigator->setUrlEditable(settings->editableUrl());
    m_urlNavigator->setHomeUrl(settings->homeUrl());

    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setShowingDotFiles(showHiddenFiles);
    m_dirLister->setDelayedMimeTypes(true);

    m_dirModel = new KDirModel();
    m_dirModel->setDirLister(m_dirLister);
    m_dirModel->setDropsAllowed(KDirModel::DropOnDirectory);

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dirModel);

    connect(m_dirLister, SIGNAL(clear()),
            this, SLOT(updateStatusBar()));
    connect(m_dirLister, SIGNAL(percent(int)),
            this, SLOT(updateProgress(int)));
    connect(m_dirLister, SIGNAL(deleteItem(KFileItem*)),
            this, SLOT(updateStatusBar()));
    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(updateItemCount()));
    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(updateCutItems()));
    connect(m_dirLister, SIGNAL(newItems(const KFileItemList&)),
            this, SLOT(generatePreviews(const KFileItemList&)));
    connect(m_dirLister, SIGNAL(infoMessage(const QString&)),
            this, SLOT(showInfoMessage(const QString&)));
    connect(m_dirLister, SIGNAL(errorMessage(const QString&)),
            this, SLOT(showErrorMessage(const QString&)));

    m_view = new DolphinView(this,
                             url,
                             m_dirLister,
                             m_dirModel,
                             m_proxyModel,
                             mode,
                             showHiddenFiles);
    connect(m_view, SIGNAL(urlChanged(const KUrl&)),
            m_urlNavigator, SLOT(setUrl(const KUrl&)));
    connect(m_view, SIGNAL(requestContextMenu(KFileItem*, const KUrl&)),
            this, SLOT(openContextMenu(KFileItem*, const KUrl&)));
    connect(m_view, SIGNAL(urlsDropped(const KUrl::List&, const KUrl&)),
            m_mainWindow, SLOT(dropUrls(const KUrl::List&, const KUrl&)));
    connect(m_view, SIGNAL(requestItemInfo(const KUrl&)),
            this, SLOT(showItemInfo(const KUrl&)));

    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            m_view, SLOT(setUrl(const KUrl&)));

    m_statusBar = new DolphinStatusBar(this, url);

    m_filterBar = new FilterBar(this);
    m_filterBar->setVisible(settings->filterBar());
    connect(m_filterBar, SIGNAL(filterChanged(const QString&)),
            this, SLOT(changeNameFilter(const QString&)));
    connect(m_filterBar, SIGNAL(closeRequest()),
            this, SLOT(closeFilterBar()));

    m_topLayout->addWidget(m_urlNavigator);
    m_topLayout->addWidget(m_view);
    m_topLayout->addWidget(m_filterBar);
    m_topLayout->addWidget(m_statusBar);
}

DolphinViewContainer::~DolphinViewContainer()
{
    delete m_dirLister;
    m_dirLister = 0;
}

void DolphinViewContainer::setUrl(const KUrl& url)
{
    m_urlNavigator->setUrl(url);
}

const KUrl& DolphinViewContainer::url() const
{
    return m_urlNavigator->url();
}

void DolphinViewContainer::setActive(bool active)
{
    m_urlNavigator->setActive(active);
    m_view->setActive(active);
}

bool DolphinViewContainer::isActive() const
{
    Q_ASSERT(m_view->isActive() == m_urlNavigator->isActive());
    return m_view->isActive();
}

void DolphinViewContainer::renameSelectedItems()
{
    DolphinViewContainer* view = m_mainWindow->activeViewContainer();
    const KUrl::List urls = m_view->selectedUrls();
    if (urls.count() > 1) {
        // More than one item has been selected for renaming. Open
        // a rename dialog and rename all items afterwards.
        RenameDialog dialog(urls);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        const QString& newName = dialog.newName();
        if (newName.isEmpty()) {
            view->statusBar()->setMessage(dialog.errorString(),
                                          DolphinStatusBar::Error);
        } else {
            // TODO: check how this can be integrated into KonqUndoManager/KonqOperations
            // as one operation instead of n rename operations like it is done now...
            Q_ASSERT(newName.contains('#'));

            // iterate through all selected items and rename them...
            const int replaceIndex = newName.indexOf('#');
            Q_ASSERT(replaceIndex >= 0);
            int index = 1;

            KUrl::List::const_iterator it = urls.begin();
            KUrl::List::const_iterator end = urls.end();
            while (it != end) {
                const KUrl& oldUrl = *it;
                QString number;
                number.setNum(index++);

                QString name(newName);
                name.replace(replaceIndex, 1, number);

                if (oldUrl.fileName() != name) {
                    KUrl newUrl = oldUrl;
                    newUrl.setFileName(name);
                    m_mainWindow->rename(oldUrl, newUrl);
                }
                ++it;
            }
        }
    } else {
        // Only one item has been selected for renaming. Use the custom
        // renaming mechanism from the views.
        Q_ASSERT(urls.count() == 1);

        // TODO: Think about using KFileItemDelegate as soon as it supports editing.
        // Currently the RenameDialog is used, but I'm not sure whether inline renaming
        // is a benefit for the user at all -> let's wait for some input first...
        RenameDialog dialog(urls);
        if (dialog.exec() == QDialog::Rejected) {
            return;
        }

        const QString& newName = dialog.newName();
        if (newName.isEmpty()) {
            view->statusBar()->setMessage(dialog.errorString(),
                                          DolphinStatusBar::Error);
        } else {
            const KUrl& oldUrl = urls.first();
            KUrl newUrl = oldUrl;
            newUrl.setFileName(newName);
            m_mainWindow->rename(oldUrl, newUrl);
        }
    }
}

DolphinStatusBar* DolphinViewContainer::statusBar() const
{
    return m_statusBar;
}

bool DolphinViewContainer::isFilterBarVisible() const
{
    return m_filterBar->isVisible();
}

bool DolphinViewContainer::isUrlEditable() const
{
    return m_urlNavigator->isUrlEditable();
}

KFileItem* DolphinViewContainer::fileItem(const QModelIndex index) const
{
    const QModelIndex dirModelIndex = m_proxyModel->mapToSource(index);
    return m_dirModel->itemForIndex(dirModelIndex);
}

DolphinMainWindow* DolphinViewContainer::mainWindow() const
{
    return m_mainWindow;
}

void DolphinViewContainer::updateProgress(int percent)
{
    if (m_showProgress) {
        m_statusBar->setProgress(percent);
    }
}

void DolphinViewContainer::updateItemCount()
{
    if (m_showProgress) {
        m_statusBar->setProgressText(QString());
        m_statusBar->setProgress(100);
        m_showProgress = false;
    }

    KFileItemList items(m_dirLister->items());
    KFileItemList::const_iterator it = items.begin();
    const KFileItemList::const_iterator end = items.end();

    m_fileCount = 0;
    m_folderCount = 0;

    while (it != end) {
        KFileItem* item = *it;
        if (item->isDir()) {
            ++m_folderCount;
        } else {
            ++m_fileCount;
        }
        ++it;
    }

    updateStatusBar();

    QTimer::singleShot(0, this, SLOT(restoreContentsPos()));
}

void DolphinViewContainer::showItemInfo(const KUrl& url)
{
    if (url.isEmpty()) {
        m_statusBar->clear();
        return;
    }

    const QModelIndex index = m_dirModel->indexForUrl(url);
    const KFileItem* item = m_dirModel->itemForIndex(index);
    if (item != 0) {
        m_statusBar->setMessage(item->getStatusBarInfo(), DolphinStatusBar::Default);
    }
}

void DolphinViewContainer::showInfoMessage(const QString& msg)
{
    m_statusBar->setMessage(msg, DolphinStatusBar::Information);
}

void DolphinViewContainer::showErrorMessage(const QString& msg)
{
    m_statusBar->setMessage(msg, DolphinStatusBar::Error);
}

void DolphinViewContainer::closeFilterBar()
{
    m_filterBar->hide();
    emit showFilterBarChanged(false);
}

QString DolphinViewContainer::defaultStatusBarText() const
{
    return KIO::itemsSummaryString(m_fileCount + m_folderCount,
                                   m_fileCount,
                                   m_folderCount,
                                   0, false);
}

QString DolphinViewContainer::selectionStatusBarText() const
{
    QString text;
    const KFileItemList list = m_view->selectedItems();
    if (list.isEmpty()) {
        // when an item is triggered, it is temporary selected but selectedItems()
        // will return an empty list
        return QString();
    }

    int fileCount = 0;
    int folderCount = 0;
    KIO::filesize_t byteSize = 0;
    KFileItemList::const_iterator it = list.begin();
    const KFileItemList::const_iterator end = list.end();
    while (it != end) {
        KFileItem* item = *it;
        if (item->isDir()) {
            ++folderCount;
        } else {
            ++fileCount;
            byteSize += item->size();
        }
        ++it;
    }

    if (folderCount > 0) {
        text = i18np("1 Folder selected", "%1 Folders selected", folderCount);
        if (fileCount > 0) {
            text += ", ";
        }
    }

    if (fileCount > 0) {
        const QString sizeText(KIO::convertSize(byteSize));
        text += i18np("1 File selected (%2)", "%1 Files selected (%2)", fileCount, sizeText);
    }

    return text;
}

void DolphinViewContainer::showFilterBar(bool show)
{
    Q_ASSERT(m_filterBar != 0);
    m_filterBar->setVisible(show);
}

void DolphinViewContainer::updateStatusBar()
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

    const QString text(m_view->hasSelection() ? selectionStatusBarText() : defaultStatusBarText());
    m_statusBar->setDefaultText(text);

    if (updateStatusBarMsg) {
        m_statusBar->setMessage(text, DolphinStatusBar::Default);
    }
}

void DolphinViewContainer::changeNameFilter(const QString& nameFilter)
{
    // The name filter of KDirLister does a 'hard' filtering, which
    // means that only the items are shown where the names match
    // exactly the filter. This is non-transparent for the user, which
    // just wants to have a 'soft' filtering: does the name contain
    // the filter string?
    QString adjustedFilter(nameFilter);
    adjustedFilter.insert(0, '*');
    adjustedFilter.append('*');

    // Use the ProxyModel to filter:
    // This code is #ifdefed as setNameFilter behaves
    // slightly different than the QSortFilterProxyModel
    // as it will not remove directories. I will ask
    // our beloved usability experts for input
    // -- z.
#if 0
    m_dirLister->setNameFilter(adjustedFilter);
    m_dirLister->emitChanges();
#else
    m_proxyModel->setFilterRegExp(nameFilter);
#endif
}

void DolphinViewContainer::openContextMenu(KFileItem* item,
                                           const KUrl& url)
{
    DolphinContextMenu contextMenu(m_mainWindow, item, url);
    contextMenu.open();
}

#include "dolphinviewcontainer.moc"
