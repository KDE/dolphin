/*
 * SPDX-FileCopyrightText: 2007 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinviewcontainer.h"

#include "dolphin_compactmodesettings.h"
#include "dolphin_contentdisplaysettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphindebug.h"
#include "dolphinplacesmodelsingleton.h"
#include "filterbar/filterbar.h"
#include "global.h"
#include "search/dolphinsearchbox.h"
#include "selectionmode/topbar.h"
#include "statusbar/dolphinstatusbar.h"

#include <KActionCollection>
#if HAVE_KACTIVITIES
#include <KActivities/ResourceInstance>
#endif
#include <KFileItemActions>
#include <KFilePlacesModel>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KMessageWidget>
#include <KProtocolManager>
#include <KShell>

#include <QApplication>
#include <QDesktopServices>
#include <QDropEvent>
#include <QGridLayout>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

// An overview of the widgets contained by this ViewContainer
struct LayoutStructure {
    int searchBox = 0;
    int messageWidget = 1;
    int selectionModeTopBar = 2;
    int view = 3;
    int selectionModeBottomBar = 4;
    int filterBar = 5;
    int statusBar = 6;
};
constexpr LayoutStructure positionFor;

DolphinViewContainer::DolphinViewContainer(const QUrl &url, QWidget *parent)
    : QWidget(parent)
    , m_topLayout(nullptr)
    , m_urlNavigator{new DolphinUrlNavigator(url)}
    , m_urlNavigatorConnected{nullptr}
    , m_searchBox(nullptr)
    , m_searchModeEnabled(false)
    , m_messageWidget(nullptr)
    , m_selectionModeTopBar{nullptr}
    , m_view(nullptr)
    , m_filterBar(nullptr)
    , m_selectionModeBottomBar{nullptr}
    , m_statusBar(nullptr)
    , m_statusBarTimer(nullptr)
    , m_statusBarTimestamp()
    , m_autoGrabFocus(true)
#if HAVE_KACTIVITIES
    , m_activityResourceInstance(nullptr)
#endif
{
    hide();

    m_topLayout = new QGridLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setContentsMargins(0, 0, 0, 0);

    m_searchBox = new DolphinSearchBox(this);
    m_searchBox->hide();
    connect(m_searchBox, &DolphinSearchBox::activated, this, &DolphinViewContainer::activate);
    connect(m_searchBox, &DolphinSearchBox::closeRequest, this, &DolphinViewContainer::closeSearchBox);
    connect(m_searchBox, &DolphinSearchBox::searchRequest, this, &DolphinViewContainer::startSearching);
    connect(m_searchBox, &DolphinSearchBox::focusViewRequest, this, &DolphinViewContainer::requestFocus);
    m_searchBox->setWhatsThis(xi18nc("@info:whatsthis findbar",
                                     "<para>This helps you find files and folders. Enter a <emphasis>"
                                     "search term</emphasis> and specify search settings with the "
                                     "buttons at the bottom:<list><item>Filename/Content: "
                                     "Does the item you are looking for contain the search terms "
                                     "within its filename or its contents?<nl/>The contents of images, "
                                     "audio files and videos will not be searched.</item><item>"
                                     "From Here/Everywhere: Do you want to search in this "
                                     "folder and its sub-folders or everywhere?</item><item>"
                                     "More Options: Click this to search by media type, access "
                                     "time or rating.</item><item>More Search Tools: Install other "
                                     "means to find an item.</item></list></para>"));

    m_messageWidget = new KMessageWidget(this);
    m_messageWidget->setCloseButtonVisible(true);
    m_messageWidget->hide();

#ifndef Q_OS_WIN
    if (getuid() == 0) {
        // We must be logged in as the root user; show a big scary warning
        showMessage(i18n("Running Dolphin as root can be dangerous. Please be careful."), Warning);
    }
#endif

    // Initialize filter bar
    m_filterBar = new FilterBar(this);
    m_filterBar->setVisible(GeneralSettings::filterBar());

    connect(m_filterBar, &FilterBar::filterChanged, this, &DolphinViewContainer::setNameFilter);
    connect(m_filterBar, &FilterBar::closeRequest, this, &DolphinViewContainer::closeFilterBar);
    connect(m_filterBar, &FilterBar::focusViewRequest, this, &DolphinViewContainer::requestFocus);

    // Initialize the main view
    m_view = new DolphinView(url, this);
    connect(m_view, &DolphinView::urlChanged, m_filterBar, &FilterBar::clearIfUnlocked);
    connect(m_view, &DolphinView::urlChanged, m_messageWidget, &KMessageWidget::hide);
    // m_urlNavigator stays in sync with m_view's location changes and
    // keeps track of them so going back and forth in the history works.
    connect(m_view, &DolphinView::urlChanged, m_urlNavigator.get(), &DolphinUrlNavigator::setLocationUrl);
    connect(m_urlNavigator.get(), &DolphinUrlNavigator::urlChanged, this, &DolphinViewContainer::slotUrlNavigatorLocationChanged);
    connect(m_urlNavigator.get(), &DolphinUrlNavigator::urlAboutToBeChanged, this, &DolphinViewContainer::slotUrlNavigatorLocationAboutToBeChanged);
    connect(m_urlNavigator.get(), &DolphinUrlNavigator::urlSelectionRequested, this, &DolphinViewContainer::slotUrlSelectionRequested);
    connect(m_view, &DolphinView::writeStateChanged, this, &DolphinViewContainer::writeStateChanged);
    connect(m_view, &DolphinView::requestItemInfo, this, &DolphinViewContainer::showItemInfo);
    connect(m_view, &DolphinView::itemActivated, this, &DolphinViewContainer::slotItemActivated);
    connect(m_view, &DolphinView::itemsActivated, this, &DolphinViewContainer::slotItemsActivated);
    connect(m_view, &DolphinView::redirection, this, &DolphinViewContainer::redirect);
    connect(m_view, &DolphinView::directoryLoadingStarted, this, &DolphinViewContainer::slotDirectoryLoadingStarted);
    connect(m_view, &DolphinView::directoryLoadingCompleted, this, &DolphinViewContainer::slotDirectoryLoadingCompleted);
    connect(m_view, &DolphinView::directoryLoadingCanceled, this, &DolphinViewContainer::slotDirectoryLoadingCanceled);
    connect(m_view, &DolphinView::itemCountChanged, this, &DolphinViewContainer::delayedStatusBarUpdate);
    connect(m_view, &DolphinView::directoryLoadingProgress, this, &DolphinViewContainer::updateDirectoryLoadingProgress);
    connect(m_view, &DolphinView::directorySortingProgress, this, &DolphinViewContainer::updateDirectorySortingProgress);
    connect(m_view, &DolphinView::selectionChanged, this, &DolphinViewContainer::delayedStatusBarUpdate);
    connect(m_view, &DolphinView::errorMessage, this, &DolphinViewContainer::showErrorMessage);
    connect(m_view, &DolphinView::urlIsFileError, this, &DolphinViewContainer::slotUrlIsFileError);
    connect(m_view, &DolphinView::activated, this, &DolphinViewContainer::activate);
    connect(m_view, &DolphinView::hiddenFilesShownChanged, this, &DolphinViewContainer::slotHiddenFilesShownChanged);
    connect(m_view, &DolphinView::sortHiddenLastChanged, this, &DolphinViewContainer::slotSortHiddenLastChanged);
    connect(m_view, &DolphinView::currentDirectoryRemoved, this, &DolphinViewContainer::slotCurrentDirectoryRemoved);

    // Initialize status bar
    m_statusBar = new DolphinStatusBar(this);
    m_statusBar->setUrl(m_view->url());
    m_statusBar->setZoomLevel(m_view->zoomLevel());
    connect(m_view, &DolphinView::urlChanged, m_statusBar, &DolphinStatusBar::setUrl);
    connect(m_view, &DolphinView::zoomLevelChanged, m_statusBar, &DolphinStatusBar::setZoomLevel);
    connect(m_view, &DolphinView::infoMessage, m_statusBar, &DolphinStatusBar::setText);
    connect(m_view, &DolphinView::operationCompletedMessage, m_statusBar, &DolphinStatusBar::setText);
    connect(m_view, &DolphinView::statusBarTextChanged, m_statusBar, &DolphinStatusBar::setDefaultText);
    connect(m_view, &DolphinView::statusBarTextChanged, m_statusBar, &DolphinStatusBar::resetToDefaultText);
    connect(m_statusBar, &DolphinStatusBar::stopPressed, this, &DolphinViewContainer::stopDirectoryLoading);
    connect(m_statusBar, &DolphinStatusBar::zoomLevelChanged, this, &DolphinViewContainer::slotStatusBarZoomLevelChanged);

    m_statusBarTimer = new QTimer(this);
    m_statusBarTimer->setSingleShot(true);
    m_statusBarTimer->setInterval(300);
    connect(m_statusBarTimer, &QTimer::timeout, this, &DolphinViewContainer::updateStatusBar);

    KIO::FileUndoManager *undoManager = KIO::FileUndoManager::self();
    connect(undoManager, &KIO::FileUndoManager::jobRecordingFinished, this, &DolphinViewContainer::delayedStatusBarUpdate);

    m_topLayout->addWidget(m_searchBox, positionFor.searchBox, 0);
    m_topLayout->addWidget(m_messageWidget, positionFor.messageWidget, 0);
    m_topLayout->addWidget(m_view, positionFor.view, 0);
    m_topLayout->addWidget(m_filterBar, positionFor.filterBar, 0);
    m_topLayout->addWidget(m_statusBar, positionFor.statusBar, 0);

    setSearchModeEnabled(isSearchUrl(url));

    // Update view as the ContentDisplaySettings change
    // this happens here and not in DolphinView as DolphinviewContainer and DolphinView are not in the same build target ATM
    connect(ContentDisplaySettings::self(), &KCoreConfigSkeleton::configChanged, m_view, &DolphinView::reload);

    KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();
    connect(placesModel, &KFilePlacesModel::dataChanged, this, &DolphinViewContainer::slotPlacesModelChanged);
    connect(placesModel, &KFilePlacesModel::rowsInserted, this, &DolphinViewContainer::slotPlacesModelChanged);
    connect(placesModel, &KFilePlacesModel::rowsRemoved, this, &DolphinViewContainer::slotPlacesModelChanged);

    connect(this, &DolphinViewContainer::searchModeEnabledChanged, this, &DolphinViewContainer::captionChanged);

    // Initialize kactivities resource instance

#if HAVE_KACTIVITIES
    m_activityResourceInstance = new KActivities::ResourceInstance(window()->winId(), url);
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

KFileItem DolphinViewContainer::rootItem() const
{
    return m_view->rootItem();
}

void DolphinViewContainer::setActive(bool active)
{
    m_searchBox->setActive(active);
    if (m_urlNavigatorConnected) {
        m_urlNavigatorConnected->setActive(active);
    }
    m_view->setActive(active);

#if HAVE_KACTIVITIES
    if (active) {
        m_activityResourceInstance->notifyFocusedIn();
    } else {
        m_activityResourceInstance->notifyFocusedOut();
    }
#endif
}

bool DolphinViewContainer::isActive() const
{
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

QString DolphinViewContainer::currentSearchText() const
{
    return m_searchBox->text();
}

const DolphinStatusBar *DolphinViewContainer::statusBar() const
{
    return m_statusBar;
}

DolphinStatusBar *DolphinViewContainer::statusBar()
{
    return m_statusBar;
}

const DolphinUrlNavigator *DolphinViewContainer::urlNavigator() const
{
    return m_urlNavigatorConnected;
}

DolphinUrlNavigator *DolphinViewContainer::urlNavigator()
{
    return m_urlNavigatorConnected;
}

const DolphinUrlNavigator *DolphinViewContainer::urlNavigatorInternalWithHistory() const
{
    return m_urlNavigator.get();
}

DolphinUrlNavigator *DolphinViewContainer::urlNavigatorInternalWithHistory()
{
    return m_urlNavigator.get();
}

const DolphinView *DolphinViewContainer::view() const
{
    return m_view;
}

DolphinView *DolphinViewContainer::view()
{
    return m_view;
}

void DolphinViewContainer::connectUrlNavigator(DolphinUrlNavigator *urlNavigator)
{
    Q_CHECK_PTR(urlNavigator);
    Q_ASSERT(!m_urlNavigatorConnected);
    Q_ASSERT(m_urlNavigator.get() != urlNavigator);
    Q_CHECK_PTR(m_view);

    urlNavigator->setLocationUrl(m_view->url());
    urlNavigator->setShowHiddenFolders(m_view->hiddenFilesShown());
    urlNavigator->setSortHiddenFoldersLast(m_view->sortHiddenLast());
    if (m_urlNavigatorVisualState) {
        urlNavigator->setVisualState(*m_urlNavigatorVisualState.get());
        m_urlNavigatorVisualState.reset();
    }
    urlNavigator->setActive(isActive());

    // Url changes are still done via m_urlNavigator.
    connect(urlNavigator, &DolphinUrlNavigator::urlChanged, m_urlNavigator.get(), &DolphinUrlNavigator::setLocationUrl);
    connect(urlNavigator, &DolphinUrlNavigator::urlsDropped, this, [=](const QUrl &destination, QDropEvent *event) {
        m_view->dropUrls(destination, event, urlNavigator->dropWidget());
    });
    // Aside from these, only visual things need to be connected.
    connect(m_view, &DolphinView::urlChanged, urlNavigator, &DolphinUrlNavigator::setLocationUrl);
    connect(urlNavigator, &DolphinUrlNavigator::activated, this, &DolphinViewContainer::activate);

    m_urlNavigatorConnected = urlNavigator;
}

void DolphinViewContainer::disconnectUrlNavigator()
{
    if (!m_urlNavigatorConnected) {
        return;
    }

    disconnect(m_urlNavigatorConnected, &DolphinUrlNavigator::urlChanged, m_urlNavigator.get(), &DolphinUrlNavigator::setLocationUrl);
    disconnect(m_urlNavigatorConnected, &DolphinUrlNavigator::urlsDropped, this, nullptr);
    disconnect(m_view, &DolphinView::urlChanged, m_urlNavigatorConnected, &DolphinUrlNavigator::setLocationUrl);
    disconnect(m_urlNavigatorConnected, &DolphinUrlNavigator::activated, this, &DolphinViewContainer::activate);

    m_urlNavigatorVisualState = m_urlNavigatorConnected->visualState();
    m_urlNavigatorConnected = nullptr;
}

void DolphinViewContainer::setSelectionModeEnabled(bool enabled, KActionCollection *actionCollection, SelectionMode::BottomBar::Contents bottomBarContents)
{
    const bool wasEnabled = m_view->selectionMode();
    m_view->setSelectionModeEnabled(enabled);

    if (!enabled) {
        if (!wasEnabled) {
            return; // nothing to do here
        }
        Q_CHECK_PTR(m_selectionModeTopBar); // there is no point in disabling selectionMode when it wasn't even enabled once.
        Q_CHECK_PTR(m_selectionModeBottomBar);
        if (m_selectionModeTopBar->isAncestorOf(QApplication::focusWidget()) || m_selectionModeBottomBar->isAncestorOf(QApplication::focusWidget())) {
            m_view->setFocus();
        }
        m_selectionModeTopBar->setVisible(false, WithAnimation);
        m_selectionModeBottomBar->setVisible(false, WithAnimation);
        Q_EMIT selectionModeChanged(false);
        return;
    }

    if (!m_selectionModeTopBar) {
        // Changing the location will disable selection mode.
        connect(m_urlNavigator.get(), &DolphinUrlNavigator::urlChanged, this, [this]() {
            setSelectionModeEnabled(false);
        });

        m_selectionModeTopBar = new SelectionMode::TopBar(this); // will be created hidden
        connect(m_selectionModeTopBar, &SelectionMode::TopBar::selectionModeLeavingRequested, this, [this]() {
            setSelectionModeEnabled(false);
        });
        m_topLayout->addWidget(m_selectionModeTopBar, positionFor.selectionModeTopBar, 0);
    }

    if (!m_selectionModeBottomBar) {
        m_selectionModeBottomBar = new SelectionMode::BottomBar(actionCollection, this);
        connect(m_view, &DolphinView::selectionChanged, this, [this](const KFileItemList &selection) {
            m_selectionModeBottomBar->slotSelectionChanged(selection, m_view->url());
        });
        connect(m_selectionModeBottomBar, &SelectionMode::BottomBar::error, this, [this](const QString &errorMessage) {
            showErrorMessage(errorMessage);
        });
        connect(m_selectionModeBottomBar, &SelectionMode::BottomBar::selectionModeLeavingRequested, this, [this]() {
            setSelectionModeEnabled(false);
        });
        m_topLayout->addWidget(m_selectionModeBottomBar, positionFor.selectionModeBottomBar, 0);
    }
    m_selectionModeBottomBar->resetContents(bottomBarContents);
    if (bottomBarContents == SelectionMode::BottomBar::GeneralContents) {
        m_selectionModeBottomBar->slotSelectionChanged(m_view->selectedItems(), m_view->url());
    }

    if (!wasEnabled) {
        m_selectionModeTopBar->setVisible(true, WithAnimation);
        m_selectionModeBottomBar->setVisible(true, WithAnimation);
        Q_EMIT selectionModeChanged(true);
    }
}

bool DolphinViewContainer::isSelectionModeEnabled() const
{
    const bool isEnabled = m_view->selectionMode();
    Q_ASSERT((!isEnabled
              // We can't assert that the bars are invisible only because the selection mode is disabled because the hide animation might still be playing.
              && (!m_selectionModeBottomBar || !m_selectionModeBottomBar->isEnabled() || !m_selectionModeBottomBar->isVisible()
                  || m_selectionModeBottomBar->contents() == SelectionMode::BottomBar::PasteContents))
             || (isEnabled && m_selectionModeTopBar
                 && m_selectionModeTopBar->isVisible()
                 // The bottom bar is either visible or was hidden because it has nothing to show in GeneralContents mode e.g. because no items are selected.
                 && m_selectionModeBottomBar
                 && (m_selectionModeBottomBar->isVisible() || m_selectionModeBottomBar->contents() == SelectionMode::BottomBar::GeneralContents)));
    return isEnabled;
}

void DolphinViewContainer::slotSplitTabDisabled()
{
    if (m_selectionModeBottomBar) {
        m_selectionModeBottomBar->slotSplitTabDisabled();
    }
}

void DolphinViewContainer::showMessage(const QString &msg, MessageType type)
{
    if (msg.isEmpty()) {
        return;
    }

    m_messageWidget->setText(msg);

    // TODO: wrap at arbitrary character positions once QLabel can do this
    // https://bugreports.qt.io/browse/QTBUG-1276
    m_messageWidget->setWordWrap(true);

    switch (type) {
    case Information:
        m_messageWidget->setMessageType(KMessageWidget::Information);
        break;
    case Warning:
        m_messageWidget->setMessageType(KMessageWidget::Warning);
        break;
    case Error:
        m_messageWidget->setMessageType(KMessageWidget::Error);
        break;
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
    // The startup settings should (only) get applied if they have been
    // modified by the user. Otherwise keep the (possibly) different current
    // setting of the filterbar.
    if (GeneralSettings::modifiedStartupSettings()) {
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
    m_searchBox->setVisible(enabled);

    if (enabled) {
        const QUrl &locationUrl = m_urlNavigator->locationUrl();
        m_searchBox->fromSearchUrl(locationUrl);
    }

    if (enabled == isSearchModeEnabled()) {
        if (enabled && !m_searchBox->hasFocus()) {
            m_searchBox->setFocus();
            m_searchBox->selectAll();
        }
        return;
    }

    if (!enabled) {
        m_view->setViewPropertiesContext(QString());

        // Restore the URL for the URL navigator. If Dolphin has been
        // started with a search-URL, the home URL is used as fallback.
        QUrl url = m_searchBox->searchPath();
        if (url.isEmpty() || !url.isValid() || isSearchUrl(url)) {
            url = Dolphin::homeUrl();
        }
        m_urlNavigatorConnected->setLocationUrl(url);
    }

    m_searchModeEnabled = enabled;

    Q_EMIT searchModeEnabledChanged(enabled);
}

bool DolphinViewContainer::isSearchModeEnabled() const
{
    return m_searchModeEnabled;
}

QString DolphinViewContainer::placesText() const
{
    QString text;

    if (isSearchModeEnabled()) {
        text = i18n("Search for %1 in %2", m_searchBox->text(), m_searchBox->searchPath().fileName());
    } else {
        text = url().adjusted(QUrl::StripTrailingSlash).fileName();
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

QString DolphinViewContainer::captionWindowTitle() const
{
    if (GeneralSettings::showFullPathInTitlebar() && !isSearchModeEnabled()) {
        if (!url().isLocalFile()) {
            return url().adjusted(QUrl::StripTrailingSlash).toString();
        }
        return url().adjusted(QUrl::StripTrailingSlash).path();
    } else {
        return DolphinViewContainer::caption();
    }
}

QString DolphinViewContainer::caption() const
{
    if (isSearchModeEnabled()) {
        if (currentSearchText().isEmpty()) {
            return i18n("Search");
        } else {
            return i18n("Search for %1", currentSearchText());
        }
    }

    KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();
    const QString pattern = url().adjusted(QUrl::StripTrailingSlash).toString(QUrl::FullyEncoded).append("/?");
    const auto &matchedPlaces =
        placesModel->match(placesModel->index(0, 0), KFilePlacesModel::UrlRole, QRegularExpression::anchoredPattern(pattern), 1, Qt::MatchRegularExpression);

    if (!matchedPlaces.isEmpty()) {
        return placesModel->text(matchedPlaces.first());
    }

    if (!url().isLocalFile()) {
        QUrl adjustedUrl = url().adjusted(QUrl::StripTrailingSlash);
        QString caption;
        if (!adjustedUrl.fileName().isEmpty()) {
            caption = adjustedUrl.fileName();
        } else if (!adjustedUrl.path().isEmpty() && adjustedUrl.path() != "/") {
            caption = adjustedUrl.path();
        } else if (!adjustedUrl.host().isEmpty()) {
            caption = adjustedUrl.host();
        } else {
            caption = adjustedUrl.toString();
        }
        return caption;
    }

    QString fileName = url().adjusted(QUrl::StripTrailingSlash).fileName();
    if (fileName.isEmpty()) {
        fileName = '/';
    }

    return fileName;
}

void DolphinViewContainer::setUrl(const QUrl &newUrl)
{
    if (newUrl != m_urlNavigator->locationUrl()) {
        m_urlNavigator->setLocationUrl(newUrl);
    }

#if HAVE_KACTIVITIES
    m_activityResourceInstance->setUri(newUrl);
#endif
}

void DolphinViewContainer::setFilterBarVisible(bool visible)
{
    Q_ASSERT(m_filterBar);
    if (visible) {
        m_view->hideToolTip(ToolTipManager::HideBehavior::Instantly);
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
    m_view->requestStatusBarText();
}

void DolphinViewContainer::updateDirectoryLoadingProgress(int percent)
{
    if (m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(i18nc("@info:progress", "Loading folder…"));
    }
    m_statusBar->setProgress(percent);
}

void DolphinViewContainer::updateDirectorySortingProgress(int percent)
{
    if (m_statusBar->progressText().isEmpty()) {
        m_statusBar->setProgressText(i18nc("@info:progress", "Sorting…"));
    }
    m_statusBar->setProgress(percent);
}

void DolphinViewContainer::slotDirectoryLoadingStarted()
{
    if (isSearchUrl(url())) {
        // Search KIO-slaves usually don't provide any progress information. Give
        // a hint to the user that a searching is done:
        updateStatusBar();
        m_statusBar->setProgressText(i18nc("@info", "Searching…"));
        m_statusBar->setProgress(-1);
    } else {
        // Trigger an undetermined progress indication. The progress
        // information in percent will be triggered by the percent() signal
        // of the directory lister later.
        m_statusBar->setProgressText(QString());
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

void DolphinViewContainer::slotUrlIsFileError(const QUrl &url)
{
    const KFileItem item(url);

    // Find out if the file can be opened in the view (for example, this is the
    // case if the file is an archive). The mime type must be known for that.
    item.determineMimeType();
    const QUrl &folderUrl = DolphinView::openItemAsFolderUrl(item, true);
    if (!folderUrl.isEmpty()) {
        setUrl(folderUrl);
    } else {
        slotItemActivated(item);
    }
}

void DolphinViewContainer::slotItemActivated(const KFileItem &item)
{
    // It is possible to activate items on inactive views by
    // drag & drop operations. Assure that activating an item always
    // results in an active view.
    m_view->setActive(true);

    const QUrl &url = DolphinView::openItemAsFolderUrl(item, GeneralSettings::browseThroughArchives());
    if (!url.isEmpty()) {
        const auto modifiers = QGuiApplication::keyboardModifiers();
        // keep in sync with KUrlNavigator::slotNavigatorButtonClicked
        if (modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier) {
            Q_EMIT activeTabRequested(url);
        } else if (modifiers & Qt::ControlModifier) {
            Q_EMIT tabRequested(url);
        } else if (modifiers & Qt::ShiftModifier) {
            Dolphin::openNewWindow({KFilePlacesModel::convertedUrl(url)}, this);
        } else {
            setUrl(url);
        }
        return;
    }

    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(item.targetUrl(), item.mimetype());
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    job->setShowOpenOrExecuteDialog(true);
    connect(job, &KIO::OpenUrlJob::finished, this, &DolphinViewContainer::slotOpenUrlFinished);
    job->start();
}

void DolphinViewContainer::slotItemsActivated(const KFileItemList &items)
{
    Q_ASSERT(items.count() >= 2);

    KFileItemActions fileItemActions(this);
    fileItemActions.runPreferredApplications(items);
}

void DolphinViewContainer::showItemInfo(const KFileItem &item)
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
    Q_EMIT showFilterBarChanged(false);
}

void DolphinViewContainer::clearFilterBar()
{
    m_filterBar->clearIfUnlocked();
}

void DolphinViewContainer::setNameFilter(const QString &nameFilter)
{
    m_view->hideToolTip(ToolTipManager::HideBehavior::Instantly);
    m_view->setNameFilter(nameFilter);
    delayedStatusBarUpdate();
}

void DolphinViewContainer::activate()
{
    setActive(true);
}

void DolphinViewContainer::slotUrlNavigatorLocationAboutToBeChanged(const QUrl &)
{
    saveViewState();
}

void DolphinViewContainer::slotUrlNavigatorLocationChanged(const QUrl &url)
{
    if (m_urlNavigatorConnected) {
        m_urlNavigatorConnected->slotReturnPressed();
    }

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
        if (url.scheme().startsWith(QLatin1String("http"))) {
            showMessage(i18nc("@info:status", // krazy:exclude=qmethods
                              "Dolphin does not support web pages, the web browser has been launched"),
                        Information);
        } else {
            showMessage(i18nc("@info:status", "Protocol not supported by Dolphin, default application has been launched"), Information);
        }

        QDesktopServices::openUrl(url);
        redirect(QUrl(), m_urlNavigator->locationUrl(1));
    } else {
        showMessage(i18nc("@info:status", "Invalid protocol"), Error);
        m_urlNavigator->goBack();
    }
}

void DolphinViewContainer::slotUrlSelectionRequested(const QUrl &url)
{
    m_view->markUrlsAsSelected({url});
    m_view->markUrlAsCurrent(url); // makes the item scroll into view
}

void DolphinViewContainer::disableUrlNavigatorSelectionRequests()
{
    disconnect(m_urlNavigator.get(), &KUrlNavigator::urlSelectionRequested, this, &DolphinViewContainer::slotUrlSelectionRequested);
}

void DolphinViewContainer::enableUrlNavigatorSelectionRequests()
{
    connect(m_urlNavigator.get(), &KUrlNavigator::urlSelectionRequested, this, &DolphinViewContainer::slotUrlSelectionRequested);
}

void DolphinViewContainer::redirect(const QUrl &oldUrl, const QUrl &newUrl)
{
    Q_UNUSED(oldUrl)
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

void DolphinViewContainer::startSearching()
{
    Q_CHECK_PTR(m_urlNavigatorConnected);
    const QUrl url = m_searchBox->urlForSearching();
    if (url.isValid() && !url.isEmpty()) {
        m_view->setViewPropertiesContext(QStringLiteral("search"));
        m_urlNavigatorConnected->setLocationUrl(url);
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

void DolphinViewContainer::showErrorMessage(const QString &msg)
{
    showMessage(msg, Error);
}

void DolphinViewContainer::slotPlacesModelChanged()
{
    if (!GeneralSettings::showFullPathInTitlebar() && !isSearchModeEnabled()) {
        Q_EMIT captionChanged();
    }
}

void DolphinViewContainer::slotHiddenFilesShownChanged(bool showHiddenFiles)
{
    if (m_urlNavigatorConnected) {
        m_urlNavigatorConnected->setShowHiddenFolders(showHiddenFiles);
    }
}

void DolphinViewContainer::slotSortHiddenLastChanged(bool hiddenLast)
{
    if (m_urlNavigatorConnected) {
        m_urlNavigatorConnected->setSortHiddenFoldersLast(hiddenLast);
    }
}

void DolphinViewContainer::slotCurrentDirectoryRemoved()
{
    const QString location(url().toDisplayString(QUrl::PreferLocalFile));
    if (url().isLocalFile()) {
        const QString dirPath = url().toLocalFile();
        const QString newPath = getNearestExistingAncestorOfPath(dirPath);
        const QUrl newUrl = QUrl::fromLocalFile(newPath);
        setUrl(newUrl);
    }

    showMessage(xi18n("Current location changed, <filename>%1</filename> is no longer accessible.", location), Warning);
}

void DolphinViewContainer::slotOpenUrlFinished(KJob *job)
{
    if (job->error() && job->error() != KIO::ERR_USER_CANCELED) {
        showErrorMessage(job->errorString());
    }
}

bool DolphinViewContainer::isSearchUrl(const QUrl &url) const
{
    return url.scheme().contains(QLatin1String("search"));
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

QString DolphinViewContainer::getNearestExistingAncestorOfPath(const QString &path) const
{
    QDir dir(path);
    do {
        dir.setPath(QDir::cleanPath(dir.filePath(QStringLiteral(".."))));
    } while (!dir.exists() && !dir.isRoot());

    return dir.exists() ? dir.path() : QString{};
}

#include "moc_dolphinviewcontainer.cpp"
