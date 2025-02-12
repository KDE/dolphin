/*
 * SPDX-FileCopyrightText: 2007 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinviewcontainer.h"

#include "admin/bar.h"
#include "admin/workerintegration.h"
#include "dolphin_compactmodesettings.h"
#include "dolphin_contentdisplaysettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphindebug.h"
#include "dolphinplacesmodelsingleton.h"
#include "filterbar/filterbar.h"
#include "global.h"
#include "kitemviews/kitemlistcontainer.h"
#include "search/dolphinsearchbox.h"
#include "selectionmode/topbar.h"
#include "statusbar/dolphinstatusbar.h"

#include <KActionCollection>
#include <KApplicationTrader>
#include <KFileItemActions>
#include <KFilePlacesModel>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KMessageWidget>
#include <KProtocolManager>
#include <KShell>
#include <kio_version.h>

#ifndef QT_NO_ACCESSIBILITY
#include <QAccessible>
#endif
#include <QApplication>
#include <QDesktopServices>
#include <QDropEvent>
#include <QGridLayout>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStyle>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

// An overview of the widgets contained by this ViewContainer
struct LayoutStructure {
    int searchBox = 0;
    int adminBar = 1;
    int messageWidget = 2;
    int selectionModeTopBar = 3;
    int view = 4;
    int selectionModeBottomBar = 5;
    int filterBar = 6;
    int statusBar = 7;
};
constexpr LayoutStructure positionFor;

DolphinViewContainer::DolphinViewContainer(const QUrl &url, QWidget *parent)
    : QWidget(parent)
    , m_topLayout(nullptr)
    , m_urlNavigator{new DolphinUrlNavigator(url)}
    , m_urlNavigatorConnected{nullptr}
    , m_searchBox(nullptr)
    , m_searchModeEnabled(false)
    , m_adminBar{nullptr}
    , m_authorizeToEnterFolderAction{nullptr}
    , m_messageWidget(nullptr)
    , m_selectionModeTopBar{nullptr}
    , m_view(nullptr)
    , m_filterBar(nullptr)
    , m_selectionModeBottomBar{nullptr}
    , m_statusBar(nullptr)
    , m_statusBarTimer(nullptr)
    , m_statusBarTimestamp()
    , m_autoGrabFocus(true)
{
    hide();

    m_topLayout = new QGridLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setContentsMargins(0, 0, 0, 0);

    m_searchBox = new DolphinSearchBox(this);
    m_searchBox->setVisible(false, WithoutAnimation);
    connect(m_searchBox, &DolphinSearchBox::activated, this, &DolphinViewContainer::activate);
    connect(m_searchBox, &DolphinSearchBox::openRequest, this, &DolphinViewContainer::openSearchBox);
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
    m_messageWidget->setPosition(KMessageWidget::Header);
    m_messageWidget->hide();

#if !defined(Q_OS_WIN) && !defined(Q_OS_HAIKU)
    if (getuid() == 0) {
        // We must be logged in as the root user; show a big scary warning
        showMessage(i18n("Running Dolphin as root can be dangerous. Please be careful."), KMessageWidget::Warning);
    }
#endif

    // Initialize filter bar
    m_filterBar = new FilterBar(this);
    m_filterBar->setVisible(GeneralSettings::filterBar(), WithoutAnimation);

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
    connect(m_view, &DolphinView::fileMiddleClickActivated, this, &DolphinViewContainer::slotfileMiddleClickActivated);
    connect(m_view, &DolphinView::itemsActivated, this, &DolphinViewContainer::slotItemsActivated);
    connect(m_view, &DolphinView::redirection, this, &DolphinViewContainer::redirect);
    connect(m_view, &DolphinView::directoryLoadingStarted, this, &DolphinViewContainer::slotDirectoryLoadingStarted);
    connect(m_view, &DolphinView::directoryLoadingCompleted, this, &DolphinViewContainer::slotDirectoryLoadingCompleted);
    connect(m_view, &DolphinView::directoryLoadingCanceled, this, &DolphinViewContainer::slotDirectoryLoadingCanceled);
    connect(m_view, &DolphinView::itemCountChanged, this, &DolphinViewContainer::delayedStatusBarUpdate);
    connect(m_view, &DolphinView::selectionChanged, this, &DolphinViewContainer::delayedStatusBarUpdate);
    connect(m_view, &DolphinView::errorMessage, this, &DolphinViewContainer::slotErrorMessageFromView);
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
    connect(m_view, &DolphinView::directoryLoadingProgress, m_statusBar, [this](int percent) {
        m_statusBar->showProgress(i18nc("@info:progress", "Loading folder…"), percent);
    });
    connect(m_view, &DolphinView::directorySortingProgress, m_statusBar, [this](int percent) {
        m_statusBar->showProgress(i18nc("@info:progress", "Sorting…"), percent);
    });
    connect(m_statusBar, &DolphinStatusBar::stopPressed, this, &DolphinViewContainer::stopDirectoryLoading);
    connect(m_statusBar, &DolphinStatusBar::zoomLevelChanged, this, &DolphinViewContainer::slotStatusBarZoomLevelChanged);
    connect(m_statusBar, &DolphinStatusBar::showMessage, this, [this](const QString &message, KMessageWidget::MessageType messageType) {
        showMessage(message, messageType);
    });
    connect(m_statusBar, &DolphinStatusBar::widthUpdated, this, &DolphinViewContainer::updateStatusBarGeometry);
    connect(m_statusBar, &DolphinStatusBar::urlChanged, this, &DolphinViewContainer::updateStatusBar);
    connect(this, &DolphinViewContainer::showFilterBarChanged, this, &DolphinViewContainer::updateStatusBar);

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
    if (m_statusBar->mode() == DolphinStatusBar::StatusBarMode::FullWidth) {
        m_topLayout->addWidget(m_statusBar, positionFor.statusBar, 0);
    }
    connect(m_statusBar, &DolphinStatusBar::modeUpdated, this, [this]() {
        const bool statusBarInLayout = m_topLayout->itemAtPosition(positionFor.statusBar, 0);
        if (m_statusBar->mode() == DolphinStatusBar::StatusBarMode::FullWidth) {
            if (!statusBarInLayout) {
                m_topLayout->addWidget(m_statusBar, positionFor.statusBar, 0);
                m_statusBar->setUrl(m_view->url());
            }
        } else {
            if (statusBarInLayout) {
                m_topLayout->removeWidget(m_statusBar);
            }
        }
    });

    setSearchModeEnabled(isSearchUrl(url));

    // Update view as the ContentDisplaySettings change
    // this happens here and not in DolphinView as DolphinviewContainer and DolphinView are not in the same build target ATM
    connect(ContentDisplaySettings::self(), &KCoreConfigSkeleton::configChanged, m_view, &DolphinView::reload);

    KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();
    connect(placesModel, &KFilePlacesModel::dataChanged, this, &DolphinViewContainer::slotPlacesModelChanged);
    connect(placesModel, &KFilePlacesModel::rowsInserted, this, &DolphinViewContainer::slotPlacesModelChanged);
    connect(placesModel, &KFilePlacesModel::rowsRemoved, this, &DolphinViewContainer::slotPlacesModelChanged);

    connect(this, &DolphinViewContainer::searchModeEnabledChanged, this, &DolphinViewContainer::captionChanged);

    QApplication::instance()->installEventFilter(this);
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
    connect(urlNavigator, &DolphinUrlNavigator::urlsDropped, this, [=, this](const QUrl &destination, QDropEvent *event) {
        m_view->dropUrls(destination, event, urlNavigator->dropWidget());
    });
    // Aside from these, only visual things need to be connected.
    connect(m_view, &DolphinView::urlChanged, urlNavigator, &DolphinUrlNavigator::setLocationUrl);
    connect(urlNavigator, &DolphinUrlNavigator::activated, this, &DolphinViewContainer::activate);
    connect(urlNavigator, &DolphinUrlNavigator::requestToLoseFocus, m_view, [this]() {
        m_view->setFocus();
    });

    urlNavigator->setReadOnlyBadgeVisible(rootItem().isLocalFile() && !rootItem().isWritable());

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
    disconnect(m_urlNavigatorConnected, &DolphinUrlNavigator::requestToLoseFocus, m_view, nullptr);

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
        m_selectionModeTopBar->setVisible(false, WithAnimation);
        m_selectionModeBottomBar->setVisible(false, WithAnimation);
        Q_EMIT selectionModeChanged(false);

        if (!QApplication::focusWidget() || m_selectionModeTopBar->isAncestorOf(QApplication::focusWidget())
            || m_selectionModeBottomBar->isAncestorOf(QApplication::focusWidget())) {
            m_view->setFocus();
        }
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
        connect(m_selectionModeBottomBar, &SelectionMode::BottomBar::error, this, &DolphinViewContainer::showErrorMessage);
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

void DolphinViewContainer::showMessage(const QString &message, KMessageWidget::MessageType messageType, std::initializer_list<QAction *> buttonActions)
{
    if (message.isEmpty()) {
        return;
    }

    m_messageWidget->setText(message);

    // TODO: wrap at arbitrary character positions once QLabel can do this
    // https://bugreports.qt.io/browse/QTBUG-1276
    m_messageWidget->setWordWrap(true);
    m_messageWidget->setMessageType(messageType);

    const QList<QAction *> previousMessageWidgetActions = m_messageWidget->actions();
    for (auto action : previousMessageWidgetActions) {
        m_messageWidget->removeAction(action);
    }
    for (QAction *action : buttonActions) {
        m_messageWidget->addAction(action);
    }

    m_messageWidget->setWordWrap(false);
    const int unwrappedWidth = m_messageWidget->sizeHint().width();
    m_messageWidget->setWordWrap(unwrappedWidth > size().width());

    if (m_messageWidget->isVisible()) {
        m_messageWidget->hide();
    }
    m_messageWidget->animatedShow();

#ifndef QT_NO_ACCESSIBILITY
    if (QAccessible::isActive() && isActive()) {
        // To announce the new message keyboard focus must be moved to the message label. However, we do not have direct access to the label that is internal
        // to the KMessageWidget. Instead we setFocus() on the KMessageWidget and trust that it has set correct focus handling.
        m_messageWidget->setFocus();
    }
#endif
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
    return m_filterBar->isEnabled(); // Gets disabled in AnimatedHeightWidget while animating towards a hidden state.
}

void DolphinViewContainer::setSearchModeEnabled(bool enabled)
{
    m_searchBox->setVisible(enabled, WithAnimation);

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
        if (m_urlNavigatorConnected) {
            m_urlNavigatorConnected->setLocationUrl(url);
        }
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
    // see KUrlNavigatorPrivate::firstButtonText().
    if (url().path().isEmpty() || url().path() == QLatin1Char('/')) {
        QUrlQuery query(url());
        const QString title = query.queryItemValue(QStringLiteral("title"));
        if (!title.isEmpty()) {
            return title;
        }
    }

    if (isSearchModeEnabled()) {
        if (currentSearchText().isEmpty()) {
            return i18n("Search");
        } else {
            return i18n("Search for %1", currentSearchText());
        }
    }

    KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();

    QModelIndex url_index = placesModel->closestItem(url());

    if (url_index.isValid() && placesModel->url(url_index).matches(url(), QUrl::StripTrailingSlash)) {
        return placesModel->text(url_index);
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
}

void DolphinViewContainer::setFilterBarVisible(bool visible)
{
    Q_ASSERT(m_filterBar);
    if (visible) {
        m_view->hideToolTip(ToolTipManager::HideBehavior::Instantly);
        m_filterBar->setVisible(true, WithAnimation);
        m_filterBar->setFocus();
        m_filterBar->selectAll();
        Q_EMIT showFilterBarChanged(true);
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
    updateStatusBarGeometry();
}

void DolphinViewContainer::slotDirectoryLoadingStarted()
{
    if (isSearchUrl(url())) {
        // Search KIO-slaves usually don't provide any progress information. Give
        // a hint to the user that a searching is done:
        updateStatusBar();
        m_statusBar->showProgress(i18nc("@info", "Searching…"), -1);
    } else {
        // Trigger an undetermined progress indication. The progress
        // information in percent will be triggered by the percent() signal
        // of the directory lister later.
        m_statusBar->showProgress(QString(), -1);
    }

    if (m_urlNavigatorConnected) {
        m_urlNavigatorConnected->setReadOnlyBadgeVisible(false);
    }
}

void DolphinViewContainer::slotDirectoryLoadingCompleted()
{
    m_statusBar->showProgress(QString(), 100);

    if (isSearchUrl(url()) && m_view->itemsCount() == 0) {
        // The dir lister has been completed on a Baloo-URI and no items have been found. Instead
        // of showing the default status bar information ("0 items") a more helpful information is given:
        m_statusBar->setText(i18nc("@info:status", "No items found."));
    } else {
        updateStatusBar();
    }

    if (m_urlNavigatorConnected) {
        m_urlNavigatorConnected->setReadOnlyBadgeVisible(rootItem().isLocalFile() && !rootItem().isWritable());
    }

    // Update admin bar visibility
    if (m_view->url().scheme() == QStringLiteral("admin")) {
        if (!m_adminBar) {
            m_adminBar = new Admin::Bar(this);
            m_topLayout->addWidget(m_adminBar, positionFor.adminBar, 0);
        }
        m_adminBar->setVisible(true, WithAnimation);
    } else if (m_adminBar) {
        m_adminBar->setVisible(false, WithAnimation);
    }
}

void DolphinViewContainer::slotDirectoryLoadingCanceled()
{
    m_statusBar->showProgress(QString(), 100);
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
    // Auto*Warning*Handling, errors are put in a KMessageWidget by us in slotOpenUrlFinished.
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoWarningHandlingEnabled, this));
    job->setShowOpenOrExecuteDialog(true);
    connect(job, &KIO::OpenUrlJob::finished, this, &DolphinViewContainer::slotOpenUrlFinished);
    job->start();
}

void DolphinViewContainer::slotfileMiddleClickActivated(const KFileItem &item)
{
    KService::List services = KApplicationTrader::queryByMimeType(item.mimetype());

    int indexOfAppToOpenFileWith = 1;

    // executable scripts
    auto mimeType = item.currentMimeType();
    if (item.isLocalFile() && mimeType.inherits(QStringLiteral("application/x-executable")) && mimeType.inherits(QStringLiteral("text/plain"))
        && QFileInfo(item.localPath()).isExecutable()) {
        KConfigGroup cfgGroup(KSharedConfig::openConfig(QStringLiteral("kiorc")), QStringLiteral("Executable scripts"));
        const QString value = cfgGroup.readEntry("behaviourOnLaunch", "alwaysAsk");

        // in case KIO::WidgetsOpenOrExecuteFileHandler::promptUserOpenOrExecute would not open the file
        if (value != QLatin1String("open")) {
            indexOfAppToOpenFileWith = 0;
        }
    }

    if (services.length() >= indexOfAppToOpenFileWith + 1) {
        auto service = services.at(indexOfAppToOpenFileWith);

        KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(service, this);
        job->setUrls({item.url()});

        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        connect(job, &KIO::OpenUrlJob::finished, this, &DolphinViewContainer::slotOpenUrlFinished);
        job->start();
    } else {
        // If no 2nd service available, try to open archives in new tabs, regardless of the "Open archives as folder" setting.
        const QUrl &url = DolphinView::openItemAsFolderUrl(item);
        const auto modifiers = QGuiApplication::keyboardModifiers();
        if (!url.isEmpty()) {
            // keep in sync with KUrlNavigator::slotNavigatorButtonClicked
            if (modifiers & Qt::ShiftModifier) {
                Q_EMIT activeTabRequested(url);
            } else {
                Q_EMIT tabRequested(url);
            }
        }
    }
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
        const bool searchBoxInitialized = isSearchModeEnabled() && m_searchBox->text().isEmpty();
        setSearchModeEnabled(isSearchUrl(url) || searchBoxInitialized);

        m_view->setUrl(url);
        tryRestoreViewState();

        if (m_autoGrabFocus && isActive() && !isSearchModeEnabled()) {
            // When an URL has been entered, the view should get the focus.
            // The focus must be requested asynchronously, as changing the URL might create
            // a new view widget.
            QTimer::singleShot(0, this, &DolphinViewContainer::requestFocus);
        }
    } else if (KProtocolManager::isSourceProtocol(url)) {
        if (url.scheme().startsWith(QLatin1String("http"))) {
            showMessage(i18nc("@info:status", // krazy:exclude=qmethods
                              "Dolphin does not support web pages, the web browser has been launched"),
                        KMessageWidget::Information);
        } else {
            showMessage(i18nc("@info:status", "Protocol not supported by Dolphin, default application has been launched"), KMessageWidget::Information);
        }

        QDesktopServices::openUrl(url);
        redirect(QUrl(), m_urlNavigator->locationUrl(1));
    } else {
        if (!url.scheme().isEmpty()) {
            showMessage(i18nc("@info:status", "Invalid protocol '%1'", url.scheme()), KMessageWidget::Error);
        } else {
            showMessage(i18nc("@info:status", "Invalid protocol"), KMessageWidget::Error);
        }
        m_urlNavigator->goBack();
    }
}

void DolphinViewContainer::slotUrlSelectionRequested(const QUrl &url)
{
    // We do not want to select any item here because there is no reason to assume that the user wants to edit the folder we are emerging from. BUG: 424723

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
    if (m_searchBox->isActive()) {
        m_searchBox->setSearchPath(newUrl);
    }
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

void DolphinViewContainer::openSearchBox()
{
    setSearchModeEnabled(true);
}

void DolphinViewContainer::closeSearchBox()
{
    setSearchModeEnabled(false);
}

void DolphinViewContainer::stopDirectoryLoading()
{
    m_view->stopLoading();
    m_statusBar->showProgress(QString(), 100);
}

void DolphinViewContainer::slotStatusBarZoomLevelChanged(int zoomLevel)
{
    m_view->setZoomLevel(zoomLevel);
}

void DolphinViewContainer::slotErrorMessageFromView(const QString &message, const int kioErrorCode)
{
    if (kioErrorCode == KIO::ERR_CANNOT_ENTER_DIRECTORY && m_view->url().scheme() == QStringLiteral("file")
        && KProtocolInfo::isKnownProtocol(QStringLiteral("admin")) && !rootItem().isReadable()) {
        // Explain to users that they need authentication to see the folder contents.
        if (!m_authorizeToEnterFolderAction) { // This code is similar to parts of Admin::Bar::hideTheNextTimeAuthorizationExpires().
            // We should not simply use the actAsAdminAction() itself here because that one always refers to the active view instead of this->m_view.
            auto actAsAdminAction = Admin::WorkerIntegration::FriendAccess::actAsAdminAction();
            m_authorizeToEnterFolderAction = new QAction{actAsAdminAction->icon(), actAsAdminAction->text(), this};
            m_authorizeToEnterFolderAction->setToolTip(actAsAdminAction->toolTip());
            m_authorizeToEnterFolderAction->setWhatsThis(actAsAdminAction->whatsThis());
            connect(m_authorizeToEnterFolderAction, &QAction::triggered, this, [this, actAsAdminAction]() {
                setActive(true);
                actAsAdminAction->trigger();
            });
        }
        showMessage(i18nc("@info", "Authorization required to enter this folder."), KMessageWidget::Error, {m_authorizeToEnterFolderAction});
        return;
    }
    Q_EMIT showErrorMessage(message);
}

void DolphinViewContainer::showErrorMessage(const QString &message)
{
    showMessage(message, KMessageWidget::Error);
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
        // #473377: Delay changing the url to avoid modifying KCoreDirLister before KCoreDirListerCache::deleteDir() returns.
        QTimer::singleShot(0, this, [&, newUrl, location] {
            setUrl(newUrl);
            showMessage(xi18n("Current location changed, <filename>%1</filename> is no longer accessible.", location), KMessageWidget::Warning);
        });
    } else
        showMessage(xi18n("Current location changed, <filename>%1</filename> is no longer accessible.", location), KMessageWidget::Warning);
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

void DolphinViewContainer::updateStatusBarGeometry()
{
    if (m_statusBar && m_statusBar->mode() == DolphinStatusBar::StatusBarMode::Small) {
        QRect statusBarRect(statusBarGeometry());
        m_statusBarHideArea = statusBarRect.adjusted(0, -20, 0, 0);
        if (view()->layoutDirection() == Qt::RightToLeft) {
            const int splitterWidth = m_statusBar->clippingAmount();
            statusBarRect.setLeft(rect().width() - m_statusBar->width() + splitterWidth); // Add clipping amount
        }
        // Move statusbar to bottomLeft, or bottomRight with right-to-left-layout
        m_statusBar->setGeometry(statusBarRect);
    }
}

bool DolphinViewContainer::eventFilter(QObject *object, QEvent *event)
{
    // Hide statusbar if mouse is on top of it
    switch (event->type()) {
    case QEvent::MouseMove: {
        QMouseEvent *e = static_cast<QMouseEvent *>(event);
        if (m_statusBar->mode() == DolphinStatusBar::StatusBarMode::Small) {
            if (m_statusBarHideArea.contains(e->pos())) {
                m_statusBar->setHidden(true);
            } else {
                m_statusBar->setHidden(!m_mouseInView);
            }
            break;
        }
    }
    case QEvent::Enter: {
        if (object == this && !m_statusBar->isVisible() && m_statusBar->mode() == DolphinStatusBar::StatusBarMode::Small) {
            m_mouseInView = true;
            m_statusBar->updateWidthToContent();
        }
        break;
    }
    case QEvent::Leave: {
        if (object == this && m_view->selectedItems().isEmpty() && m_statusBar->isVisible() && m_statusBar->mode() == DolphinStatusBar::StatusBarMode::Small) {
            m_mouseInView = false;
        }
        break;
    }
    case QEvent::Resize: {
        if (object == this && m_statusBar->mode() == DolphinStatusBar::StatusBarMode::Small) {
            m_statusBar->updateWidthToContent();
        }
    }
    default:
        break;
    }
    return false;
}

QRect DolphinViewContainer::statusBarGeometry()
{
    // Add offset depending if horizontal scrollbar or filterbar is visible
    int filterBarHeightOffset = 0;
    int scrollbarHeightOffset = 0;
    const auto container = m_view->container();
    if (container) {
        if (container->horizontalScrollBar() && container->horizontalScrollBar()->isVisible()) {
            scrollbarHeightOffset = container->horizontalScrollBar()->height();
        }
    }
    if (m_filterBar->isVisible()) {
        filterBarHeightOffset = m_filterBar->height();
    }

    // Adjust to clipping
    int clipAdjustment = m_statusBar->clippingAmount() * 2;
    const int yPos = rect().bottom() - m_statusBar->minimumHeight() - scrollbarHeightOffset - filterBarHeightOffset + clipAdjustment;
    QRect statusBarRect = rect().adjusted(-clipAdjustment, yPos, 0, 0);
    return statusBarRect;
}

#include "moc_dolphinviewcontainer.cpp"
