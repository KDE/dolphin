/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz19@gmail.com>             *
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
#include <KProtocolManager>

#include <QApplication>
#include <QKeyEvent>
#include <QItemSelection>
#include <QBoxLayout>
#include <QTimer>
#include <QScrollBar>

#include <KDesktopFile>
#include <KFileItemDelegate>
#include <KFilePlacesModel>
#include <KLocale>
#include <KIconEffect>
#include <KIO/NetAccess>
#include <KIO/PreviewJob>
#include <KNewFileMenu>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <KShell>
#include <KUrl>
#include <KUrlComboBox>
#include <KUrlNavigator>
#include <KRun>

#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"
#include "filterbar/filterbar.h"
#include "search/dolphinsearchbox.h"
#include "statusbar/dolphinstatusbar.h"
#include "views/dolphinplacesmodel.h"
#include "views/draganddrophelper.h"
#include "views/viewmodecontroller.h"
#include "views/viewproperties.h"

DolphinViewContainer::DolphinViewContainer(const KUrl& url, QWidget* parent) :
    QWidget(parent),
    m_topLayout(0),
    m_urlNavigator(0),
    m_searchBox(0),
    m_view(0),
    m_filterBar(0),
    m_statusBar(0),
    m_statusBarTimer(0),
    m_statusBarTimestamp()
{
    hide();

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    m_urlNavigator = new KUrlNavigator(DolphinPlacesModel::instance(), url, this);
    connect(m_urlNavigator, SIGNAL(urlsDropped(KUrl,QDropEvent*)),
            this, SLOT(dropUrls(KUrl,QDropEvent*)));
    connect(m_urlNavigator, SIGNAL(activated()),
            this, SLOT(activate()));
    connect(m_urlNavigator->editor(), SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
            this, SLOT(saveUrlCompletionMode(KGlobalSettings::Completion)));

    const GeneralSettings* settings = GeneralSettings::self();
    m_urlNavigator->setUrlEditable(settings->editableUrl());
    m_urlNavigator->setShowFullPath(settings->showFullPath());
    m_urlNavigator->setHomeUrl(KUrl(settings->homeUrl()));
    KUrlComboBox* editor = m_urlNavigator->editor();
    editor->setCompletionMode(KGlobalSettings::Completion(settings->urlCompletionMode()));

    m_searchBox = new DolphinSearchBox(this);
    m_searchBox->hide();
    connect(m_searchBox, SIGNAL(closeRequest()), this, SLOT(closeSearchBox()));
    connect(m_searchBox, SIGNAL(search(QString)), this, SLOT(startSearching(QString)));
    connect(m_searchBox, SIGNAL(returnPressed(QString)), this, SLOT(requestFocus()));

    m_view = new DolphinView(url, this);
    connect(m_view, SIGNAL(urlChanged(KUrl)),                   m_urlNavigator, SLOT(setUrl(KUrl)));
    connect(m_view, SIGNAL(writeStateChanged(bool)),            this, SIGNAL(writeStateChanged(bool)));
    connect(m_view, SIGNAL(requestItemInfo(KFileItem)),         this, SLOT(showItemInfo(KFileItem)));
    connect(m_view, SIGNAL(errorMessage(QString)),              this, SLOT(showErrorMessage(QString)));
    connect(m_view, SIGNAL(infoMessage(QString)),               this, SLOT(showInfoMessage(QString)));
    connect(m_view, SIGNAL(itemActivated(KFileItem)),           this, SLOT(slotItemActivated(KFileItem)));
    connect(m_view, SIGNAL(redirection(KUrl,KUrl)),             this, SLOT(redirect(KUrl,KUrl)));
    connect(m_view, SIGNAL(directoryLoadingStarted()),          this, SLOT(slotDirectoryLoadingStarted()));
    connect(m_view, SIGNAL(directoryLoadingCompleted()),        this, SLOT(slotDirectoryLoadingCompleted()));
    connect(m_view, SIGNAL(itemCountChanged()),                 this, SLOT(delayedStatusBarUpdate()));
    connect(m_view, SIGNAL(directoryLoadingProgress(int)),      this, SLOT(updateDirectoryLoadingProgress(int)));
    connect(m_view, SIGNAL(directorySortingProgress(int)),      this, SLOT(updateDirectorySortingProgress(int)));
    connect(m_view, SIGNAL(infoMessage(QString)),               this, SLOT(showInfoMessage(QString)));
    connect(m_view, SIGNAL(errorMessage(QString)),              this, SLOT(showErrorMessage(QString)));
    connect(m_view, SIGNAL(selectionChanged(KFileItemList)),    this, SLOT(delayedStatusBarUpdate()));
    connect(m_view, SIGNAL(operationCompletedMessage(QString)), this, SLOT(showOperationCompletedMessage(QString)));
    connect(m_view, SIGNAL(urlAboutToBeChanged(KUrl)),          this, SLOT(slotViewUrlAboutToBeChanged(KUrl)));

    connect(m_urlNavigator, SIGNAL(urlAboutToBeChanged(KUrl)),
            this, SLOT(slotUrlNavigatorLocationAboutToBeChanged(KUrl)));
    connect(m_urlNavigator, SIGNAL(urlChanged(KUrl)),
            this, SLOT(slotUrlNavigatorLocationChanged(KUrl)));
    connect(m_urlNavigator, SIGNAL(historyChanged()),
            this, SLOT(slotHistoryChanged()));

    // initialize status bar
    m_statusBar = new DolphinStatusBar(this, m_view);
    connect(m_statusBar, SIGNAL(stopPressed()), this, SLOT(stopLoading()));

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
    connect(m_filterBar, SIGNAL(filterChanged(QString)),
            this, SLOT(setNameFilter(QString)));
    connect(m_filterBar, SIGNAL(closeRequest()),
            this, SLOT(closeFilterBar()));
    connect(m_view, SIGNAL(urlChanged(KUrl)),
            m_filterBar, SLOT(clear()));

    m_topLayout->addWidget(m_urlNavigator);
    m_topLayout->addWidget(m_searchBox);
    m_topLayout->addWidget(m_view);
    m_topLayout->addWidget(m_filterBar);
    m_topLayout->addWidget(m_statusBar);

    setSearchModeEnabled(isSearchUrl(url));
}

DolphinViewContainer::~DolphinViewContainer()
{
}

KUrl DolphinViewContainer::url() const
{
    return m_view->url();
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

const DolphinStatusBar* DolphinViewContainer::statusBar() const
{
    return m_statusBar;
}

DolphinStatusBar* DolphinViewContainer::statusBar()
{
    return m_statusBar;
}

const KUrlNavigator* DolphinViewContainer::urlNavigator() const
{
    return m_urlNavigator;
}

KUrlNavigator* DolphinViewContainer::urlNavigator()
{
    return m_urlNavigator;
}

const DolphinView* DolphinViewContainer::view() const
{
    return m_view;
}

DolphinView* DolphinViewContainer::view()
{
    return m_view;
}

const DolphinSearchBox* DolphinViewContainer::searchBox() const
{
    return m_searchBox;
}

DolphinSearchBox* DolphinViewContainer::searchBox()
{
    return m_searchBox;
}

void DolphinViewContainer::readSettings()
{
    if (GeneralSettings::modifiedStartupSettings()) {
        // The startup settings should only get applied if they have been
        // modified by the user. Otherwise keep the (possibly) different current
        // settings of the URL navigator and the filterbar.
        m_urlNavigator->setUrlEditable(GeneralSettings::editableUrl());
        m_urlNavigator->setShowFullPath(GeneralSettings::showFullPath());
        m_urlNavigator->setHomeUrl(KUrl(GeneralSettings::homeUrl()));
        setFilterBarVisible(GeneralSettings::filterBar());
    }

    m_view->readSettings();
    m_statusBar->readSettings();
}

bool DolphinViewContainer::isFilterBarVisible() const
{
    return m_filterBar->isVisible();
}

void DolphinViewContainer::setSearchModeEnabled(bool enabled)
{
    if (enabled == isSearchModeEnabled()) {
        if (enabled && !m_searchBox->hasFocus()) {
            m_searchBox->setFocus();
            m_searchBox->selectAll();
        }
        return;
    }

    m_searchBox->setVisible(enabled);
    m_urlNavigator->setVisible(!enabled);

    if (enabled) {
        KUrl url = m_urlNavigator->locationUrl();
        m_searchBox->setText(QString());
        m_searchBox->setReadOnly(isSearchUrl(url), url);

        // Remember the most recent non-search URL as search path
        // of the search-box, so that it can be restored
        // when switching back to the URL navigator.
        int index = m_urlNavigator->historyIndex();
        const int historySize = m_urlNavigator->historySize();
        while (isSearchUrl(url) && (index < historySize)) {
            ++index;
            url = m_urlNavigator->locationUrl(index);
        }

        if (!isSearchUrl(url)) {
            m_searchBox->setSearchPath(url);
        }
    } else {
        // Restore the URL for the URL navigator. If Dolphin has been
        // started with a search-URL, the home URL is used as fallback.
        const KUrl url = m_searchBox->searchPath();
        if (url.isValid() && !url.isEmpty()) {
            if (isSearchUrl(url)) {
                m_urlNavigator->goHome();
            } else {
                m_urlNavigator->setLocationUrl(url);
            }
        }
    }

    emit searchModeChanged(enabled);
}

bool DolphinViewContainer::isSearchModeEnabled() const
{
    return m_searchBox->isVisible();
}

void DolphinViewContainer::setUrl(const KUrl& newUrl)
{
    if (newUrl != m_urlNavigator->locationUrl()) {
        m_urlNavigator->setLocationUrl(newUrl);
    }
}

void DolphinViewContainer::setFilterBarVisible(bool visible)
{
    Q_ASSERT(m_filterBar);
    if (visible) {
        m_filterBar->show();
        m_filterBar->setFocus();
        m_filterBar->selectAll();
    } else {
        closeFilterBar();
    }
}

void DolphinViewContainer::delayedStatusBarUpdate()
{
    if (m_statusBarTimer->isActive() && (m_statusBarTimestamp.elapsed() > 2000)) {
        // No update of the statusbar has been done during the last 2 seconds,
        // although an update has been requested. Trigger an immediate update.
        m_statusBarTimer->stop();
        updateStatusBar();
    } else {
        // Invoke updateStatusBar() with a small delay. This assures that
        // when a lot of delayedStatusBarUpdates() are done in a short time,
        // no bottleneck is given.
        m_statusBarTimer->start();
    }
}

void DolphinViewContainer::updateStatusBar()
{
    m_statusBarTimestamp.start();

    const QString newMessage = m_view->statusBarText();
    m_statusBar->setDefaultText(newMessage);

    // We don't want to override errors. Other messages are only protected by
    // the Statusbar itself depending on timings (see DolphinStatusBar::setMessage).
    if (m_statusBar->type() != DolphinStatusBar::Error) {
        m_statusBar->setMessage(newMessage, DolphinStatusBar::Default);
    }
}

void DolphinViewContainer::updateDirectoryLoadingProgress(int percent)
{
    if (m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(i18nc("@info:progress", "Loading folder..."));
    }
    m_statusBar->setProgress(percent);
}

void DolphinViewContainer::updateDirectorySortingProgress(int percent)
{
    if (m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(i18nc("@info:progress", "Sorting..."));
    }
    m_statusBar->setProgress(percent);
}

void DolphinViewContainer::slotDirectoryLoadingStarted()
{
    if (isSearchUrl(url())) {
        // Search KIO-slaves usually don't provide any progress information. Give
        // a hint to the user that a searching is done:
        updateStatusBar();
        m_statusBar->setProgressText(i18nc("@info", "Searching..."));
        m_statusBar->setProgress(-1);
    } else {
        // Trigger an undetermined progress indication. The progress
        // information in percent will be triggered by the percent() signal
        // of the directory lister later.
        updateDirectoryLoadingProgress(-1);
    }
}

void DolphinViewContainer::slotDirectoryLoadingCompleted()
{
    if (!m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(QString());
        m_statusBar->setProgress(100);
    }

    if (isSearchUrl(url()) && m_view->itemsCount() == 0) {
        // The dir lister has been completed on a Nepomuk-URI and no items have been found. Instead
        // of showing the default status bar information ("0 items") a more helpful information is given:
        m_statusBar->setMessage(i18nc("@info:status", "No items found."), DolphinStatusBar::Information);
    } else {
        updateStatusBar();
    }
}

void DolphinViewContainer::slotItemActivated(const KFileItem& item)
{
    // It is possible to activate items on inactive views by
    // drag & drop operations. Assure that activating an item always
    // results in an active view.
    m_view->setActive(true);

    KUrl url = item.targetUrl();

    if (item.isDir()) {
        m_view->setUrl(url);
        return;
    }

    if (GeneralSettings::browseThroughArchives() && item.isFile() && url.isLocalFile()) {
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

    if (item.mimetype() == QLatin1String("application/x-desktop")) {
        // Redirect to the URL in Type=Link desktop files
        KDesktopFile desktopFile(url.toLocalFile());
        if (desktopFile.hasLinkType()) {
            url = desktopFile.readUrl();
            m_view->setUrl(url);
            return;
        }
    }

    item.run();
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
        const QString message = item.isDir() ? item.text() : item.getStatusBarInfo();
        m_statusBar->setMessage(message, DolphinStatusBar::Default);
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

void DolphinViewContainer::slotViewUrlAboutToBeChanged(const KUrl& url)
{
    // URL changes of the view can happen in two ways:
    // 1. The URL navigator gets changed and will trigger the view to update its URL
    // 2. The URL of the view gets changed and will trigger the URL navigator to update
    //    its URL (e.g. by clicking on an item)
    // In this scope the view-state may only get saved in case 2:
    if (url != m_urlNavigator->locationUrl()) {
        saveViewState();
    }
}

void DolphinViewContainer::slotUrlNavigatorLocationAboutToBeChanged(const KUrl& url)
{
    // URL changes of the view can happen in two ways:
    // 1. The URL navigator gets changed and will trigger the view to update its URL
    // 2. The URL of the view gets changed and will trigger the URL navigator to update
    //    its URL (e.g. by clicking on an item)
    // In this scope the view-state may only get saved in case 1:
    if (url != m_view->url()) {
        saveViewState();
    }
}

void DolphinViewContainer::slotUrlNavigatorLocationChanged(const KUrl& url)
{
    if (KProtocolManager::supportsListing(url)) {
        setSearchModeEnabled(isSearchUrl(url));
        m_view->setUrl(url);

        if (isActive() && !isSearchUrl(url)) {
            // When an URL has been entered, the view should get the focus.
            // The focus must be requested asynchronously, as changing the URL might create
            // a new view widget.
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
    DragAndDropHelper::dropUrls(KFileItem(), destination, event);
}

void DolphinViewContainer::redirect(const KUrl& oldUrl, const KUrl& newUrl)
{
    Q_UNUSED(oldUrl);
    const bool block = m_urlNavigator->signalsBlocked();
    m_urlNavigator->blockSignals(true);

    // Assure that the location state is reset for redirection URLs. This
    // allows to skip redirection URLs when going back or forward in the
    // URL history.
    m_urlNavigator->saveLocationState(QByteArray());
    m_urlNavigator->setLocationUrl(newUrl);
    setSearchModeEnabled(isSearchUrl(newUrl));

    m_urlNavigator->blockSignals(block);
}

void DolphinViewContainer::requestFocus()
{
    m_view->setFocus();
}

void DolphinViewContainer::saveUrlCompletionMode(KGlobalSettings::Completion completion)
{
    GeneralSettings::setUrlCompletionMode(completion);
}

void DolphinViewContainer::slotHistoryChanged()
{
    QByteArray locationState = m_urlNavigator->locationState();
    if (!locationState.isEmpty()) {
        QDataStream stream(&locationState, QIODevice::ReadOnly);
        m_view->restoreState(stream);
    }
}

void DolphinViewContainer::startSearching(const QString &text)
{
    Q_UNUSED(text);
    const KUrl url = m_searchBox->urlForSearching();
    if (url.isValid() && !url.isEmpty()) {
        m_urlNavigator->setLocationUrl(url);
    }
}

void DolphinViewContainer::closeSearchBox()
{
    setSearchModeEnabled(false);
}

void DolphinViewContainer::stopLoading()
{
    m_view->stopLoading();
    m_statusBar->setProgress(100);
}

bool DolphinViewContainer::isSearchUrl(const KUrl& url) const
{
    const QString protocol = url.protocol();
    return protocol.contains("search") || (protocol == QLatin1String("nepomuk"));
}

void DolphinViewContainer::saveViewState()
{
    QByteArray locationState;
    QDataStream stream(&locationState, QIODevice::WriteOnly);
    m_view->saveState(stream);
    m_urlNavigator->saveLocationState(locationState);
}

#include "dolphinviewcontainer.moc"
