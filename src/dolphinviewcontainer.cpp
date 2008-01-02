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

#include <kfileitemdelegate.h>
#include <kfileplacesmodel.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kiconeffect.h>
#include <kio/netaccess.h>
#include <kio/previewjob.h>
#include <kmimetyperesolver.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <kurl.h>

#include "dolphinmodel.h"
#include "dolphincolumnview.h"
#include "dolphincontroller.h"
#include "dolphinstatusbar.h"
#include "dolphinmainwindow.h"
#include "dolphindirlister.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "dolphincontextmenu.h"
#include "filterbar.h"
#include "kurlnavigator.h"
#include "viewproperties.h"
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

DolphinViewContainer::DolphinViewContainer(DolphinMainWindow* mainWindow,
                                           QWidget* parent,
                                           const KUrl& url) :
    QWidget(parent),
    m_showProgress(false),
    m_mainWindow(mainWindow),
    m_topLayout(0),
    m_urlNavigator(0),
    m_view(0),
    m_filterBar(0),
    m_statusBar(0),
    m_dirLister(0),
    m_proxyModel(0)
{
    hide();
    setFocusPolicy(Qt::StrongFocus);
    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    m_urlNavigator = new KUrlNavigator(DolphinSettings::instance().placesModel(), url, this);
    connect(m_urlNavigator, SIGNAL(urlsDropped(const KUrl::List&, const KUrl&)),
            m_mainWindow, SLOT(dropUrls(const KUrl::List&, const KUrl&)));
    connect(m_urlNavigator, SIGNAL(activated()),
            this, SLOT(activate()));

    const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_urlNavigator->setUrlEditable(settings->editableUrl());
    m_urlNavigator->setHomeUrl(settings->homeUrl());

    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setDelayedMimeTypes(true);

    m_dolphinModel = new DolphinModel(this);
    m_dolphinModel->setDirLister(m_dirLister);
    m_dolphinModel->setDropsAllowed(DolphinModel::DropOnDirectory);

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dolphinModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(m_dirLister, SIGNAL(clear()),
            this, SLOT(updateStatusBar()));
    connect(m_dirLister, SIGNAL(percent(int)),
            this, SLOT(updateProgress(int)));
    connect(m_dirLister, SIGNAL(deleteItem(const KFileItem&)),
            this, SLOT(updateStatusBar()));
    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(slotDirListerCompleted()));
    connect(m_dirLister, SIGNAL(infoMessage(const QString&)),
            this, SLOT(showInfoMessage(const QString&)));
    connect(m_dirLister, SIGNAL(errorMessage(const QString&)),
            this, SLOT(showErrorMessage(const QString&)));

    m_view = new DolphinView(this,
                             url,
                             m_dirLister,
                             m_dolphinModel,
                             m_proxyModel);
    connect(m_view, SIGNAL(urlChanged(const KUrl&)),
            m_urlNavigator, SLOT(setUrl(const KUrl&)));
    connect(m_view, SIGNAL(requestContextMenu(KFileItem, const KUrl&)),
            this, SLOT(openContextMenu(KFileItem, const KUrl&)));
    connect(m_view, SIGNAL(contentsMoved(int, int)),
            this, SLOT(saveContentsPos(int, int)));
    connect(m_view, SIGNAL(requestItemInfo(KFileItem)),
            this, SLOT(showItemInfo(KFileItem)));
    connect(m_view, SIGNAL(errorMessage(const QString&)),
            this, SLOT(showErrorMessage(const QString&)));
    connect(m_view, SIGNAL(infoMessage(const QString&)),
            this, SLOT(showInfoMessage(const QString&)));
    connect(m_view, SIGNAL(operationCompletedMessage(const QString&)),
            this, SLOT(showOperationCompletedMessage(const QString&)));
    connect(m_view, SIGNAL(itemTriggered(KFileItem)),
            this, SLOT(slotItemTriggered(KFileItem)));
    connect(m_view, SIGNAL(startedPathLoading(const KUrl&)),
            this, SLOT(saveRootUrl(const KUrl&)));

    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(restoreView(const KUrl&)));

    m_statusBar = new DolphinStatusBar(this, url);
    connect(m_view, SIGNAL(urlChanged(const KUrl&)),
            m_statusBar, SLOT(updateSpaceInfoContent(const KUrl&)));

    m_filterBar = new FilterBar(this);
    m_filterBar->setVisible(settings->filterBar());
    connect(m_filterBar, SIGNAL(filterChanged(const QString&)),
            this, SLOT(setNameFilter(const QString&)));
    connect(m_filterBar, SIGNAL(closeRequest()),
            this, SLOT(closeFilterBar()));

    m_topLayout->addWidget(m_urlNavigator);
    m_topLayout->addWidget(m_view);
    m_topLayout->addWidget(m_filterBar);
    m_topLayout->addWidget(m_statusBar);
}

DolphinViewContainer::~DolphinViewContainer()
{
    m_dirLister->disconnect();

    delete m_proxyModel;
    m_proxyModel = 0;
    delete m_dolphinModel;
    m_dolphinModel = 0;
    m_dirLister = 0; // deleted by m_dolphinModel
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

bool DolphinViewContainer::isFilterBarVisible() const
{
    return m_filterBar->isVisible();
}

bool DolphinViewContainer::isUrlEditable() const
{
    return m_urlNavigator->isUrlEditable();
}

KFileItem DolphinViewContainer::fileItem(const QModelIndex& index) const
{
    const QModelIndex dolphinModelIndex = m_proxyModel->mapToSource(index);
    return m_dolphinModel->itemForIndex(dolphinModelIndex);
}

void DolphinViewContainer::updateProgress(int percent)
{
    if (!m_showProgress) {
        // Only show the directory loading progress if the status bar does
        // not contain another progress information. This means that
        // the directory loading progress information has the lowest priority.
        const QString progressText(m_statusBar->progressText());
        const QString loadingText(i18nc("@info:progress", "Loading folder..."));
        m_showProgress = progressText.isEmpty() || (progressText == loadingText);
        if (m_showProgress) {
            m_statusBar->setProgressText(loadingText);
            m_statusBar->setProgress(0);
        }
    }

    if (m_showProgress) {
        m_statusBar->setProgress(percent);
    }
}

void DolphinViewContainer::slotDirListerCompleted()
{
    if (m_showProgress) {
        m_statusBar->setProgressText(QString());
        m_statusBar->setProgress(100);
        m_showProgress = false;
    }

    updateStatusBar();

    QTimer::singleShot(100, this, SLOT(restoreContentsPos()));
}

void DolphinViewContainer::showItemInfo(const KFileItem& item)
{
    if (item.isNull()) {
        m_statusBar->clear();
    } else {
        m_statusBar->setMessage(item.getStatusBarInfo(), DolphinStatusBar::Default);
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

void DolphinViewContainer::showOperationCompletedMessage(const QString& msg)
{
    m_statusBar->setMessage(msg, DolphinStatusBar::OperationCompleted);
}

void DolphinViewContainer::closeFilterBar()
{
    m_filterBar->hide();
    emit showFilterBarChanged(false);
}

QString DolphinViewContainer::defaultStatusBarText() const
{
    int folderCount = 0;
    int fileCount = 0;
    m_view->calculateItemCount(fileCount, folderCount);
    return KIO::itemsSummaryString(fileCount + folderCount,
                                   fileCount,
                                   folderCount,
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
        const KFileItem& item = *it;
        if (item.isDir()) {
            ++folderCount;
        } else {
            ++fileCount;
            byteSize += item.size();
        }
        ++it;
    }

    if (folderCount > 0) {
        text = i18ncp("@info:status", "1 Folder selected", "%1 Folders selected", folderCount);
        if (fileCount > 0) {
            text += ", ";
        }
    }

    if (fileCount > 0) {
        const QString sizeText(KIO::convertSize(byteSize));
        text += i18ncp("@info:status", "1 File selected (%2)", "%1 Files selected (%2)", fileCount, sizeText);
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

void DolphinViewContainer::setNameFilter(const QString& nameFilter)
{
    m_view->setNameFilter(nameFilter);
    updateStatusBar();
}

void DolphinViewContainer::openContextMenu(const KFileItem& item,
                                           const KUrl& url)
{
    DolphinContextMenu contextMenu(m_mainWindow, item, url);
    contextMenu.open();
}

void DolphinViewContainer::saveContentsPos(int x, int y)
{
    m_urlNavigator->savePosition(x, y);
}

void DolphinViewContainer::restoreContentsPos()
{
    if (!url().isEmpty()) {
        const QPoint pos = m_urlNavigator->savedPosition();
        m_view->setContentsPosition(pos.x(), pos.y());
    }
}

void DolphinViewContainer::activate()
{
    setActive(true);
}

void DolphinViewContainer::restoreView(const KUrl& url)
{
    m_view->updateView(url, m_urlNavigator->savedRootUrl());
}

void DolphinViewContainer::saveRootUrl(const KUrl& url)
{
    Q_UNUSED(url);
    m_urlNavigator->saveRootUrl(m_view->rootUrl());
}

void DolphinViewContainer::slotItemTriggered(const KFileItem& item)
{
    // Prefer the local path over the URL.
    bool isLocal;
    KUrl url = item.mostLocalUrl(isLocal);

    if (item.isDir()) {
        m_view->setUrl(url);
    } else if (item.isFile() && url.isLocalFile()) {
        // allow to browse through ZIP and tar files
        // TODO: make this configurable for Dolphin in KDE 4.1

        KMimeType::Ptr mime = item.mimeTypePtr();

        // Don't use mime->is("application/zip"), as this would
        // also browse through Open Office files:
        if (mime->name() == "application/zip") {
            url.setProtocol("zip");
            m_view->setUrl(url);
        } else if (mime->is("application/x-tar") ||
                   mime->is("application/x-tarz") ||
                   mime->is("application/x-bzip-compressed-tar") ||
                   mime->is("application/x-compressed-tar") ||
                   mime->is("application/x-tzo")) {
            url.setProtocol("tar");
            m_view->setUrl(url);
        } else {
            item.run();
        }
    } else {
        item.run();
    }
}

#include "dolphinviewcontainer.moc"
