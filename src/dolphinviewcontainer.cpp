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
#include <QtGui/QKeyEvent>
#include <QtGui/QItemSelection>
#include <QtGui/QBoxLayout>
#include <QtCore/QTimer>
#include <QtGui/QScrollBar>

#include <kdesktopfile.h>
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
#include <kfileitemlistproperties.h>
#include <konq_operations.h>
#include <kshell.h>
#include <kurl.h>
#include <kurlcombobox.h>
#include <kurlnavigator.h>
#include <krun.h>

#include "dolphin_generalsettings.h"
#include "dolphinmodel.h"
#include "dolphincolumnview.h"
#include "dolphinviewcontroller.h"
#include "dolphinmainwindow.h"
#include "dolphindirlister.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphindetailsview.h"
#include "dolphiniconsview.h"
#include "draganddrophelper.h"
#include "filterbar.h"
#include "settings/dolphinsettings.h"
#include "statusbar/dolphinstatusbar.h"
#include "viewmodecontroller.h"
#include "viewproperties.h"

DolphinViewContainer::DolphinViewContainer(const KUrl& url, QWidget* parent) :
    QWidget(parent),
    m_isFolderWritable(false),
    m_topLayout(0),
    m_urlNavigator(0),
    m_view(0),
    m_filterBar(0),
    m_statusBar(0),
    m_statusBarTimer(0),
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
    connect(m_urlNavigator->editor(), SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
            this, SLOT(saveUrlCompletionMode(KGlobalSettings::Completion)));

    const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_urlNavigator->setUrlEditable(settings->editableUrl());
    m_urlNavigator->setShowFullPath(settings->showFullPath());
    m_urlNavigator->setHomeUrl(KUrl(settings->homeUrl()));
    KUrlComboBox* editor = m_urlNavigator->editor();
    editor->setCompletionMode(KGlobalSettings::Completion(settings->urlCompletionMode()));

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

    connect(m_dirLister, SIGNAL(started(KUrl)),
            this, SLOT(initializeProgress()));
    connect(m_dirLister, SIGNAL(clear()),
            this, SLOT(delayedStatusBarUpdate()));
    connect(m_dirLister, SIGNAL(percent(int)),
            this, SLOT(updateProgress(int)));
    connect(m_dirLister, SIGNAL(itemsDeleted(const KFileItemList&)),
            this, SLOT(delayedStatusBarUpdate()));
    connect(m_dirLister, SIGNAL(completed()),
            this, SLOT(slotDirListerCompleted()));
    connect(m_dirLister, SIGNAL(infoMessage(const QString&)),
            this, SLOT(showInfoMessage(const QString&)));
    connect(m_dirLister, SIGNAL(errorMessage(const QString&)),
            this, SLOT(showErrorMessage(const QString&)));
    connect(m_dirLister, SIGNAL(urlIsFileError(const KUrl&)),
            this, SLOT(openFile(const KUrl&)));

    m_view = new DolphinView(this, url, m_proxyModel);
    connect(m_view, SIGNAL(urlChanged(const KUrl&)),
            m_urlNavigator, SLOT(setUrl(const KUrl&)));
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
    connect(m_view, SIGNAL(redirection(KUrl, KUrl)),
            this, SLOT(redirect(KUrl, KUrl)));
    connect(m_view, SIGNAL(selectionChanged(const KFileItemList&)),
            this, SLOT(delayedStatusBarUpdate()));

    connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(slotUrlNavigatorLocationChanged(const KUrl&)));
    connect(m_urlNavigator, SIGNAL(urlAboutToBeChanged(const KUrl&)),
            this, SLOT(saveViewState()));
    connect(m_urlNavigator, SIGNAL(historyChanged()),
            this, SLOT(slotHistoryChanged()));

    // initialize status bar
    m_statusBar = new DolphinStatusBar(this, m_view);
    m_statusBarTimer = new QTimer(this);
    m_statusBarTimer->setSingleShot(true);
    m_statusBarTimer->setInterval(300);
    connect(m_statusBarTimer, SIGNAL(timeout()),
            this, SLOT(updateStatusBar()));

    KIO::FileUndoManager* undoManager = KIO::FileUndoManager::self();
    connect(undoManager, SIGNAL(jobRecordingFinished(CommandType)),
            this, SLOT(delayedStatusBarUpdate()));

    // initialize filter bar
    m_filterBar = new FilterBar(this);
    m_filterBar->setVisible(settings->filterBar());
    connect(m_filterBar, SIGNAL(filterChanged(const QString&)),
            this, SLOT(setNameFilter(const QString&)));
    connect(m_filterBar, SIGNAL(closeRequest()),
            this, SLOT(closeFilterBar()));
    connect(m_view, SIGNAL(urlChanged(const KUrl&)),
            m_filterBar, SLOT(clear()));

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

KUrl DolphinViewContainer::url() const
{
    return m_urlNavigator->locationUrl();
}

void DolphinViewContainer::setActive(bool active)
{
    m_urlNavigator->setActive(active);
    m_view->setActive(active);
    if (active) {
        emit writeStateChanged(m_isFolderWritable);
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

void DolphinViewContainer::setUrl(const KUrl& newUrl)
{
    if (newUrl != m_urlNavigator->locationUrl()) {
        m_urlNavigator->setLocationUrl(newUrl);
        // Temporary disable the 'File'->'Create New...' menu until
        // the write permissions can be checked in a fast way at
        // DolphinViewContainer::slotDirListerCompleted().
        m_isFolderWritable = false;
        if (isActive()) {
            emit writeStateChanged(false);
        }
    }
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

void DolphinViewContainer::delayedStatusBarUpdate()
{
    // Invoke updateStatusBar() with a small delay. This assures that
    // when a lot of delayedStatusBarUpdates() are done in a short time,
    // no bottleneck is given.
    m_statusBarTimer->start();
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
    const QString msg = m_statusBar->message();
    const bool updateStatusBarMsg = (msg.isEmpty()
                                     || (msg == m_statusBar->defaultText())
                                     || (m_statusBar->type() == DolphinStatusBar::Information))
                                    && (m_statusBar->progress() == 100);

    const QString text = m_view->statusBarText();
    m_statusBar->setDefaultText(text);

    if (updateStatusBarMsg) {
        m_statusBar->setMessage(text, DolphinStatusBar::Default);
    }
}

void DolphinViewContainer::initializeProgress()
{
    if (url().protocol() == "nepomuksearch") {
        // The Nepomuk IO-slave does not provide any progress information. Give
        // an immediate hint to the user that a searching is done:
        m_statusBar->setProgressText(i18nc("@info", "Searching..."));
        m_statusBar->setProgress(-1);
    }
 }

void DolphinViewContainer::updateProgress(int percent)
{
    if (m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(i18nc("@info:progress", "Loading folder..."));
    }
    m_statusBar->setProgress(percent);
}

void DolphinViewContainer::slotDirListerCompleted()
{
    if (!m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(QString());
        m_statusBar->setProgress(100);
    }

    if ((url().protocol() == "nepomuksearch") && (m_dirLister->items().count() == 0)) {
        // The dir lister has been completed on a Nepomuk-URI and no items have been found. Instead
        // of showing the default status bar information ("0 items") a more helpful information is given:
        m_statusBar->setMessage(i18nc("@info:status", "No items found."), DolphinStatusBar::Information);
    } else {
        updateStatusBar();
    }

    // Enable the 'File'->'Create New...' menu only if the directory
    // supports writing.
    KFileItem item = m_dirLister->rootItem();
    if (item.isNull()) {
        // it is unclear whether writing is supported
        m_isFolderWritable = true;
    } else {
        KFileItemListProperties capabilities(KFileItemList() << item);
        m_isFolderWritable = capabilities.supportsWriting();
    }

    if (isActive()) {
        emit writeStateChanged(m_isFolderWritable);
    }
}

void DolphinViewContainer::showItemInfo(const KFileItem& item)
{
    if (item.isNull()) {
        // Only clear the status bar if unimportant messages are shown.
        // This prevents that information- or error-messages get hidden
        // by moving the mouse above the viewport or when closing the
        // context menu.
        if (m_statusBar->type() == DolphinStatusBar::Default) {
            m_statusBar->clear();
        }
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

void DolphinViewContainer::setNameFilter(const QString& nameFilter)
{
    m_view->setNameFilter(nameFilter);
    delayedStatusBarUpdate();
}

void DolphinViewContainer::activate()
{
    setActive(true);
}

void DolphinViewContainer::saveViewState()
{
    QByteArray locationState;
    QDataStream stream(&locationState, QIODevice::WriteOnly);
    m_view->saveState(stream);
    m_urlNavigator->saveLocationState(locationState);
}

void DolphinViewContainer::slotUrlNavigatorLocationChanged(const KUrl& url)
{
    if (KProtocolManager::supportsListing(url)) {
        m_view->setUrl(url);
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
                if (app.startsWith('!')) {
                    // a literal command has been configured, remove the '!' prefix
                    app = app.mid(1);
                }
            }
        } else {
            showErrorMessage(i18nc("@info:status",
                                   "Protocol not supported by Dolphin, Konqueror has been launched"));
        }

        const QString secureUrl = KShell::quoteArg(url.pathOrUrl());
        const QString command = app + ' ' + secureUrl;
        KRun::runCommand(command, app, app, this);
    } else {
        showErrorMessage(i18nc("@info:status", "Invalid protocol"));
    }
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
    m_urlNavigator->setLocationUrl(newUrl);
    m_urlNavigator->blockSignals(block);
}

void DolphinViewContainer::requestFocus()
{
    m_view->setFocus();
}

void DolphinViewContainer::saveUrlCompletionMode(KGlobalSettings::Completion completion)
{
    DolphinSettings& settings = DolphinSettings::instance();
    settings.generalSettings()->setUrlCompletionMode(completion);
    settings.save();
}

void DolphinViewContainer::slotHistoryChanged()
{
    QByteArray locationState = m_urlNavigator->locationState();

    if (!locationState.isEmpty()) {
        QDataStream stream(&locationState, QIODevice::ReadOnly);
        m_view->restoreState(stream);
    }
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

    if (item.mimetype() == "application/x-desktop") {
        // redirect to the url in Type=Link desktop files
        KDesktopFile desktopFile(url.toLocalFile());
        if (desktopFile.hasLinkType()) {
            url = desktopFile.readUrl();
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
