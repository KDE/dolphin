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
#include <kprotocolmanager.h>

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
#include <kmenu.h>
#include <kmimetyperesolver.h>
#include <knewmenu.h>
#include <konqmimedata.h>
#include <konq_fileitemcapabilities.h>
#include <konq_operations.h>
#include <kurl.h>
#include <krun.h>

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
#include "draganddrophelper.h"
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
    m_isFolderWritable(false),
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

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    m_urlNavigator = new KUrlNavigator(DolphinSettings::instance().placesModel(), url, this);
    connect(m_urlNavigator, SIGNAL(urlsDropped(const KUrl&, QDropEvent*)),
            this, SLOT(dropUrls(const KUrl&, QDropEvent*)));
    connect(m_urlNavigator, SIGNAL(activated()),
            this, SLOT(activate()));

    const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_urlNavigator->setUrlEditable(settings->editableUrl());
    m_urlNavigator->setShowFullPath(settings->showFullPath());
    m_urlNavigator->setHomeUrl(settings->homeUrl());

    m_dirLister = new DolphinDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(window());
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
    connect(m_dirLister, SIGNAL(urlIsFileError(const KUrl&)),
            this, SLOT(openFile(const KUrl&)));

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
    connect(m_view, SIGNAL(redirection(KUrl, KUrl)),
            this, SLOT(redirect(KUrl, KUrl)));

    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(restoreView(const KUrl&)));

    m_statusBar = new DolphinStatusBar(this, m_view);

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

void DolphinViewContainer::setUrl(const KUrl& newUrl)
{
    m_urlNavigator->setUrl(newUrl);
    if (newUrl != m_urlNavigator->url()) {
        // Temporary disable the 'File'->'Create New...' menu until
        // the write permissions can be checked in a fast way at
        // DolphinViewContainer::slotDirListerCompleted().
        m_isFolderWritable = false;
        if (isActive()) {
            m_mainWindow->newMenu()->menu()->setEnabled(false);
        }
    }
}

const KUrl& DolphinViewContainer::url() const
{
    return m_urlNavigator->url();
}

void DolphinViewContainer::setActive(bool active)
{
    m_urlNavigator->setActive(active);
    m_view->setActive(active);
    if (active) {
        m_mainWindow->newMenu()->menu()->setEnabled(m_isFolderWritable);
    }
}

bool DolphinViewContainer::isActive() const
{
    Q_ASSERT(m_view->isActive() == m_urlNavigator->isActive());
    return m_view->isActive();
}

void DolphinViewContainer::refresh()
{
    m_view->refresh();
    m_statusBar->refresh();
}

bool DolphinViewContainer::isFilterBarVisible() const
{
    return m_filterBar->isVisible();
}

void DolphinViewContainer::showFilterBar(bool show)
{
    Q_ASSERT(m_filterBar != 0);
    if (show) {
        m_filterBar->show();
    } else {
        closeFilterBar();
    }
}

bool DolphinViewContainer::isUrlEditable() const
{
    return m_urlNavigator->isUrlEditable();
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
    QMetaObject::invokeMethod(this, "restoreContentsPos", Qt::QueuedConnection);

    // Enable the 'File'->'Create New...' menu only if the directory
    // supports writing.
    KFileItem item = m_dirLister->rootItem();
    if (item.isNull()) {
        // it is unclear whether writing is supported
        m_isFolderWritable = true;
    } else {
        KonqFileItemCapabilities capabilities(KFileItemList() << item);
        m_isFolderWritable = capabilities.supportsWriting();
    }

    if (isActive()) {
        m_mainWindow->newMenu()->menu()->setEnabled(m_isFolderWritable);
    }
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
    m_filterBar->clear();
    m_view->setFocus();
    emit showFilterBarChanged(false);
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

    const QString text(m_view->statusBarText());
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
    if (KProtocolManager::supportsListing(url)) {
        m_view->updateView(url, m_urlNavigator->savedRootUrl());
        if (isActive()) {
            // When an URL has been entered, the view should get the focus.
            // The focus must be requested asynchronously, as changing the URL might create
            // a new view widget. Using QTimer::singleShow() works reliable, however
            // QMetaObject::invokeMethod() with a queued connection does not work, which might
            // indicate that we should pass a hint to DolphinView::updateView()
            // regarding the focus instead. To test: Enter an URL and press CTRL+Enter.
            // Expected result: The view should get the focus.
            QTimer::singleShot(0, this, SLOT(requestFocus()));
        }
    } else if (KProtocolManager::isSourceProtocol(url)) {
        QString app = "konqueror";
        if (url.protocol().startsWith(QLatin1String("http"))) {
            showErrorMessage(i18nc("@info:status",
                                   "Dolphin does not support web pages, the web browser has been launched"));
            const KConfigGroup config(KSharedConfig::openConfig("kdeglobals"), "General");
            const QString browser = config.readEntry("BrowserApplication");
            if (!browser.isEmpty()) {
                app = browser;
            }
        } else {
            showErrorMessage(i18nc("@info:status",
                                   "Protocol not supported by Dolphin, Konqueror has been launched"));
        }
        const QString command = app + ' ' + url.pathOrUrl();
        KRun::runCommand(command, app, app, this);
    } else {
        showErrorMessage(i18nc("@info:status", "Invalid protocol"));
    }
}

void DolphinViewContainer::saveRootUrl(const KUrl& url)
{
    Q_UNUSED(url);
    m_urlNavigator->saveRootUrl(m_view->rootUrl());
}

void DolphinViewContainer::dropUrls(const KUrl& destination, QDropEvent* event)
{
    DragAndDropHelper::instance().dropUrls(KFileItem(), destination, event, this);
}

void DolphinViewContainer::redirect(const KUrl& oldUrl, const KUrl& newUrl)
{
    Q_UNUSED(oldUrl);
    const bool block = m_urlNavigator->signalsBlocked();
    m_urlNavigator->blockSignals(true);
    m_urlNavigator->setUrl(newUrl);
    m_urlNavigator->blockSignals(block);
}

void DolphinViewContainer::requestFocus()
{
    m_view->setFocus();
}

void DolphinViewContainer::slotItemTriggered(const KFileItem& item)
{
    KUrl url = item.targetUrl();

    if (item.isDir()) {
        m_view->setUrl(url);
        return;
    }

    const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    const bool browseThroughArchives = settings->browseThroughArchives();
    if (browseThroughArchives && item.isFile() && url.isLocalFile()) {
        // Generic mechanism for redirecting to tar:/<path>/ when clicking on a tar file,
        // zip:/<path>/ when clicking on a zip file, etc.
        // The .protocol file specifies the mimetype that the kioslave handles.
        // Note that we don't use mimetype inheritance since we don't want to
        // open OpenDocument files as zip folders...
        const QString protocol = KProtocolManager::protocolForArchiveMimetype(item.mimetype());
        if (!protocol.isEmpty()) {
            url.setProtocol(protocol);
            m_view->setUrl(url);
            return;
        }
    }

    item.run();
}

void DolphinViewContainer::openFile(const KUrl& url)
{
    // Using m_dolphinModel for getting the file item instance is not possible
    // here: openFile() is triggered by an error of the directory lister
    // job, so the file item must be received "manually".
    const KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
    slotItemTriggered(item);
}

#include "dolphinviewcontainer.moc"
