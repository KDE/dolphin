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

#include <QDropEvent>
#include <QTimer>
#include <QMimeData>
#include <QVBoxLayout>
#include <QLoggingCategory>

#include <KFileItemActions>
#include <KFilePlacesModel>
#include <KLocalizedString>
#include <KIO/PreviewJob>
#include <kio_version.h>
#include <KMessageWidget>
#include <KShell>
#include <QUrl>
#include <KUrlComboBox>
#include <KUrlNavigator>
#include <KRun>

#ifdef KActivities_FOUND
#endif

#include "global.h"
#include "dolphindebug.h"
#include "dolphin_generalsettings.h"
#include "filterbar/filterbar.h"
#include "search/dolphinsearchbox.h"
#include "statusbar/dolphinstatusbar.h"
#include "views/viewmodecontroller.h"
#include "views/viewproperties.h"

DolphinViewContainer::DolphinViewContainer(const QUrl& url, QWidget* parent) :
    QWidget(parent),
    m_topLayout(0),
    m_urlNavigator(0),
    m_searchBox(0),
    m_messageWidget(0),
    m_view(0),
    m_filterBar(0),
    m_statusBar(0),
    m_statusBarTimer(0),
    m_statusBarTimestamp(),
    m_autoGrabFocus(true)
#ifdef KActivities_FOUND
    , m_activityResourceInstance(0)
#endif
{
    hide();

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);

    m_urlNavigator = new KUrlNavigator(new KFilePlacesModel(this), url, this);
    connect(m_urlNavigator, &KUrlNavigator::activated,
            this, &DolphinViewContainer::activate);
    connect(m_urlNavigator->editor(), &KUrlComboBox::completionModeChanged,
            this, &DolphinViewContainer::saveUrlCompletionMode);

    const GeneralSettings* settings = GeneralSettings::self();
    m_urlNavigator->setUrlEditable(settings->editableUrl());
    m_urlNavigator->setShowFullPath(settings->showFullPath());
    m_urlNavigator->setHomeUrl(Dolphin::homeUrl());
    KUrlComboBox* editor = m_urlNavigator->editor();
    editor->setCompletionMode(KCompletion::CompletionMode(settings->urlCompletionMode()));

    m_searchBox = new DolphinSearchBox(this);
    m_searchBox->hide();
    connect(m_searchBox, &DolphinSearchBox::activated, this, &DolphinViewContainer::activate);
    connect(m_searchBox, &DolphinSearchBox::closeRequest, this, &DolphinViewContainer::closeSearchBox);
    connect(m_searchBox, &DolphinSearchBox::searchRequest, this, &DolphinViewContainer::startSearching);
    connect(m_searchBox, &DolphinSearchBox::returnPressed, this, &DolphinViewContainer::requestFocus);

    m_messageWidget = new KMessageWidget(this);
    m_messageWidget->setCloseButtonVisible(true);
    m_messageWidget->hide();

    m_view = new DolphinView(url, this);
    connect(m_view, &DolphinView::urlChanged,
            m_urlNavigator, &KUrlNavigator::setLocationUrl);
    connect(m_view, &DolphinView::urlChanged,
            m_messageWidget, &KMessageWidget::hide);
    connect(m_view, &DolphinView::writeStateChanged,
            this, &DolphinViewContainer::writeStateChanged);
    connect(m_view, &DolphinView::requestItemInfo,
            this, &DolphinViewContainer::showItemInfo);
    connect(m_view, &DolphinView::itemActivated,
            this, &DolphinViewContainer::slotItemActivated);
    connect(m_view, &DolphinView::itemsActivated,
            this, &DolphinViewContainer::slotItemsActivated);
    connect(m_view, &DolphinView::redirection,
            this, &DolphinViewContainer::redirect);
    connect(m_view, &DolphinView::directoryLoadingStarted,
            this, &DolphinViewContainer::slotDirectoryLoadingStarted);
    connect(m_view, &DolphinView::directoryLoadingCompleted,
            this, &DolphinViewContainer::slotDirectoryLoadingCompleted);
    connect(m_view, &DolphinView::directoryLoadingCanceled,
            this, &DolphinViewContainer::slotDirectoryLoadingCanceled);
    connect(m_view, &DolphinView::itemCountChanged,
            this, &DolphinViewContainer::delayedStatusBarUpdate);
    connect(m_view, &DolphinView::directoryLoadingProgress,
            this, &DolphinViewContainer::updateDirectoryLoadingProgress);
    connect(m_view, &DolphinView::directorySortingProgress,
            this, &DolphinViewContainer::updateDirectorySortingProgress);
    connect(m_view, &DolphinView::selectionChanged,
            this, &DolphinViewContainer::delayedStatusBarUpdate);
    connect(m_view, &DolphinView::errorMessage,
            this, &DolphinViewContainer::showErrorMessage);
    connect(m_view, &DolphinView::urlIsFileError,
            this, &DolphinViewContainer::slotUrlIsFileError);
    connect(m_view, &DolphinView::activated,
            this, &DolphinViewContainer::activate);

    connect(m_urlNavigator, &KUrlNavigator::urlAboutToBeChanged,
            this, &DolphinViewContainer::slotUrlNavigatorLocationAboutToBeChanged);
    connect(m_urlNavigator, &KUrlNavigator::urlChanged,
            this, &DolphinViewContainer::slotUrlNavigatorLocationChanged);
    connect(m_urlNavigator, &KUrlNavigator::urlSelectionRequested,
            this, &DolphinViewContainer::slotUrlSelectionRequested);
    connect(m_urlNavigator, &KUrlNavigator::returnPressed,
            this, &DolphinViewContainer::slotReturnPressed);
    connect(m_urlNavigator, &KUrlNavigator::urlsDropped, this, [=](const QUrl &destination, QDropEvent *event) {
#if KIO_VERSION >= QT_VERSION_CHECK(5, 37, 0)
        m_view->dropUrls(destination, event, m_urlNavigator->dropWidget());
#else
        // TODO: remove as soon as we can hard-depend of KF5 >= 5.37
        m_view->dropUrls(destination, event, m_view);
#endif
    });

    // Initialize status bar
    m_statusBar = new DolphinStatusBar(this);
    m_statusBar->setUrl(m_view->url());
    m_statusBar->setZoomLevel(m_view->zoomLevel());
    connect(m_view, &DolphinView::urlChanged,
            m_statusBar, &DolphinStatusBar::setUrl);
    connect(m_view, &DolphinView::zoomLevelChanged,
            m_statusBar, &DolphinStatusBar::setZoomLevel);
    connect(m_view, &DolphinView::infoMessage,
            m_statusBar, &DolphinStatusBar::setText);
    connect(m_view, &DolphinView::operationCompletedMessage,
            m_statusBar, &DolphinStatusBar::setText);
    connect(m_statusBar, &DolphinStatusBar::stopPressed,
            this, &DolphinViewContainer::stopDirectoryLoading);
    connect(m_statusBar, &DolphinStatusBar::zoomLevelChanged,
            this, &DolphinViewContainer::slotStatusBarZoomLevelChanged);

    m_statusBarTimer = new QTimer(this);
    m_statusBarTimer->setSingleShot(true);
    m_statusBarTimer->setInterval(300);
    connect(m_statusBarTimer, &QTimer::timeout, this, &DolphinViewContainer::updateStatusBar);

    KIO::FileUndoManager* undoManager = KIO::FileUndoManager::self();
    connect(undoManager, &KIO::FileUndoManager::jobRecordingFinished,
            this, &DolphinViewContainer::delayedStatusBarUpdate);

    // Initialize filter bar
    m_filterBar = new FilterBar(this);
    m_filterBar->setVisible(settings->filterBar());
    connect(m_filterBar, &FilterBar::filterChanged,
            this, &DolphinViewContainer::setNameFilter);
    connect(m_filterBar, &FilterBar::closeRequest,
            this, &DolphinViewContainer::closeFilterBar);
    connect(m_filterBar, &FilterBar::focusViewRequest,
            this, &DolphinViewContainer::requestFocus);
    connect(m_view, &DolphinView::urlChanged,
            m_filterBar, &FilterBar::slotUrlChanged);

    m_topLayout->addWidget(m_urlNavigator);
    m_topLayout->addWidget(m_searchBox);
    m_topLayout->addWidget(m_messageWidget);
    m_topLayout->addWidget(m_view);
    m_topLayout->addWidget(m_filterBar);
    m_topLayout->addWidget(m_statusBar);

    setSearchModeEnabled(isSearchUrl(url));

    // Initialize kactivities resource instance

    #ifdef KActivities_FOUND
    m_activityResourceInstance = new KActivities::ResourceInstance(
            window()->winId(), url);
    m_activityResourceInstance->setParent(this);
    #endif
}

DolphinViewContainer::~DolphinViewContainer()
{
}

QUrl DolphinViewContainer::url() const
{
    return m_view->url();
}

void DolphinViewContainer::setActive(bool active)
{
    m_searchBox->setActive(active);
    m_urlNavigator->setActive(active);
    m_view->setActive(active);

    #ifdef KActivities_FOUND
    if (active) {
        m_activityResourceInstance->notifyFocusedIn();
    } else {
        m_activityResourceInstance->notifyFocusedOut();
    }
    #endif
}

bool DolphinViewContainer::isActive() const
{
    Q_ASSERT(m_view->isActive() == m_urlNavigator->isActive());
    return m_view->isActive();
}

void DolphinViewContainer::setAutoGrabFocus(bool grab)
{
    m_autoGrabFocus = grab;
}

bool DolphinViewContainer::autoGrabFocus() const
{
    return m_autoGrabFocus;
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

void DolphinViewContainer::showMessage(const QString& msg, MessageType type)
{
    if (msg.isEmpty()) {
        return;
    }

    m_messageWidget->setText(msg);

    switch (type) {
    case Information: m_messageWidget->setMessageType(KMessageWidget::Information); break;
    case Warning:     m_messageWidget->setMessageType(KMessageWidget::Warning); break;
    case Error:       m_messageWidget->setMessageType(KMessageWidget::Error); break;
    default:
        Q_ASSERT(false);
        break;
    }

    m_messageWidget->setWordWrap(false);
    const int unwrappedWidth = m_messageWidget->sizeHint().width();
    m_messageWidget->setWordWrap(unwrappedWidth > size().width());

    if (m_messageWidget->isVisible()) {
        m_messageWidget->hide();
    }
    m_messageWidget->animatedShow();
}

void DolphinViewContainer::readSettings()
{
    if (GeneralSettings::modifiedStartupSettings()) {
        // The startup settings should only get applied if they have been
        // modified by the user. Otherwise keep the (possibly) different current
        // settings of the URL navigator and the filterbar.
        m_urlNavigator->setUrlEditable(GeneralSettings::editableUrl());
        m_urlNavigator->setShowFullPath(GeneralSettings::showFullPath());
        m_urlNavigator->setHomeUrl(Dolphin::homeUrl());
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
        const QUrl& locationUrl = m_urlNavigator->locationUrl();
        m_searchBox->fromSearchUrl(locationUrl);
    } else {
        m_view->setViewPropertiesContext(QString());

        // Restore the URL for the URL navigator. If Dolphin has been
        // started with a search-URL, the home URL is used as fallback.
        QUrl url = m_searchBox->searchPath();
        if (url.isEmpty() || !url.isValid() || isSearchUrl(url)) {
            url = Dolphin::homeUrl();
        }
        m_urlNavigator->setLocationUrl(url);
    }
}

bool DolphinViewContainer::isSearchModeEnabled() const
{
    return m_searchBox->isVisible();
}

QString DolphinViewContainer::placesText() const
{
    QString text;

    if (isSearchModeEnabled()) {
        text = m_searchBox->searchPath().fileName() + QLatin1String(": ") + m_searchBox->text();
    } else {
        text = url().fileName();
        if (text.isEmpty()) {
            text = url().host();
        }
        if (text.isEmpty()) {
            text = url().scheme();
        }
    }

    return text;
}

void DolphinViewContainer::reload()
{
    view()->reload();
    m_messageWidget->hide();
}

void DolphinViewContainer::setUrl(const QUrl& newUrl)
{
    if (newUrl != m_urlNavigator->locationUrl()) {
        m_urlNavigator->setLocationUrl(newUrl);
    }

    #ifdef KActivities_FOUND
    m_activityResourceInstance->setUri(newUrl);
    #endif
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

    const QString text = m_view->statusBarText();
    m_statusBar->setDefaultText(text);
    m_statusBar->resetToDefaultText();
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
        // The dir lister has been completed on a Baloo-URI and no items have been found. Instead
        // of showing the default status bar information ("0 items") a more helpful information is given:
        m_statusBar->setText(i18nc("@info:status", "No items found."));
    } else {
        updateStatusBar();
    }
}

void DolphinViewContainer::slotDirectoryLoadingCanceled()
{
    if (!m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(QString());
        m_statusBar->setProgress(100);
    }

    m_statusBar->setText(QString());
}

void DolphinViewContainer::slotUrlIsFileError(const QUrl& url)
{
    const KFileItem item(url);

    // Find out if the file can be opened in the view (for example, this is the
    // case if the file is an archive). The mime type must be known for that.
    item.determineMimeType();
    const QUrl& folderUrl = DolphinView::openItemAsFolderUrl(item, true);
    if (!folderUrl.isEmpty()) {
        setUrl(folderUrl);
    } else {
        slotItemActivated(item);
    }
}

void DolphinViewContainer::slotItemActivated(const KFileItem& item)
{
    // It is possible to activate items on inactive views by
    // drag & drop operations. Assure that activating an item always
    // results in an active view.
    m_view->setActive(true);

    const QUrl& url = DolphinView::openItemAsFolderUrl(item, GeneralSettings::browseThroughArchives());
    if (!url.isEmpty()) {
        setUrl(url);
        return;
    }

    KRun *run = new KRun(item.targetUrl(), this);
    run->setShowScriptExecutionPrompt(true);
}

void DolphinViewContainer::slotItemsActivated(const KFileItemList& items)
{
    Q_ASSERT(items.count() >= 2);

    KFileItemActions fileItemActions(this);
    fileItemActions.runPreferredApplications(items, QString());
}

void DolphinViewContainer::showItemInfo(const KFileItem& item)
{
    if (item.isNull()) {
        m_statusBar->resetToDefaultText();
    } else {
        m_statusBar->setText(item.getStatusBarInfo());
    }
}

void DolphinViewContainer::closeFilterBar()
{
    m_filterBar->closeFilterBar();
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

void DolphinViewContainer::slotUrlNavigatorLocationAboutToBeChanged(const QUrl&)
{
    saveViewState();
}

void DolphinViewContainer::slotUrlNavigatorLocationChanged(const QUrl& url)
{
    slotReturnPressed();

    if (KProtocolManager::supportsListing(url)) {
        setSearchModeEnabled(isSearchUrl(url));
        m_view->setUrl(url);
        tryRestoreViewState();

        if (m_autoGrabFocus && isActive() && !isSearchUrl(url)) {
            // When an URL has been entered, the view should get the focus.
            // The focus must be requested asynchronously, as changing the URL might create
            // a new view widget.
            QTimer::singleShot(0, this, &DolphinViewContainer::requestFocus);
        }
    } else if (KProtocolManager::isSourceProtocol(url)) {
        QString app = QStringLiteral("konqueror");
        if (url.scheme().startsWith(QLatin1String("http"))) {
            showMessage(i18nc("@info:status", // krazy:exclude=qmethods
                              "Dolphin does not support web pages, the web browser has been launched"),
                        Information);

            const KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "General");
            const QString browser = config.readEntry("BrowserApplication");
            if (!browser.isEmpty()) {
                app = browser;
                if (app.startsWith('!')) {
                    // a literal command has been configured, remove the '!' prefix
                    app = app.mid(1);
                }
            }
        } else {
            showMessage(i18nc("@info:status",
                              "Protocol not supported by Dolphin, Konqueror has been launched"),
                        Information);
        }

        const QString secureUrl = KShell::quoteArg(url.toDisplayString(QUrl::PreferLocalFile));
        const QString command = app + ' ' + secureUrl;
        KRun::runCommand(command, app, app, this);
    } else {
        showMessage(i18nc("@info:status", "Invalid protocol"), Error);
    }
}

void DolphinViewContainer::slotUrlSelectionRequested(const QUrl& url)
{
    qCDebug(DolphinDebug) << "slotUrlSelectionRequested: " << url;
    m_view->markUrlsAsSelected({url});
    m_view->markUrlAsCurrent(url); // makes the item scroll into view
}

void DolphinViewContainer::redirect(const QUrl& oldUrl, const QUrl& newUrl)
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

void DolphinViewContainer::saveUrlCompletionMode(KCompletion::CompletionMode completion)
{
    GeneralSettings::setUrlCompletionMode(completion);
}

void DolphinViewContainer::slotReturnPressed()
{
    if (!GeneralSettings::editableUrl()) {
        m_urlNavigator->setUrlEditable(false);
    }
}

void DolphinViewContainer::startSearching()
{
    const QUrl url = m_searchBox->urlForSearching();
    if (url.isValid() && !url.isEmpty()) {
        m_view->setViewPropertiesContext(QStringLiteral("search"));
        m_urlNavigator->setLocationUrl(url);
    }
}

void DolphinViewContainer::closeSearchBox()
{
    setSearchModeEnabled(false);
}

void DolphinViewContainer::stopDirectoryLoading()
{
    m_view->stopLoading();
    m_statusBar->setProgress(100);
}

void DolphinViewContainer::slotStatusBarZoomLevelChanged(int zoomLevel)
{
    m_view->setZoomLevel(zoomLevel);
}

void DolphinViewContainer::showErrorMessage(const QString& msg)
{
    showMessage(msg, Error);
}

bool DolphinViewContainer::isSearchUrl(const QUrl& url) const
{
    return url.scheme().contains(QStringLiteral("search"));
}

void DolphinViewContainer::saveViewState()
{
    QByteArray locationState;
    QDataStream stream(&locationState, QIODevice::WriteOnly);
    m_view->saveState(stream);
    m_urlNavigator->saveLocationState(locationState);
}

void DolphinViewContainer::tryRestoreViewState()
{
    QByteArray locationState = m_urlNavigator->locationState();
    if (!locationState.isEmpty()) {
        QDataStream stream(&locationState, QIODevice::ReadOnly);
        m_view->restoreState(stream);
    }
}
