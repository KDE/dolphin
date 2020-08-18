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

#include "dolphin_generalsettings.h"
#include "dolphinplacesmodelsingleton.h"
#include "dolphindebug.h"
#include "filterbar/filterbar.h"
#include "global.h"
#include "search/dolphinsearchbox.h"
#include "statusbar/dolphinstatusbar.h"
#include "trash/dolphintrash.h"
#include "views/viewmodecontroller.h"
#include "views/viewproperties.h"
#include "dolphin_detailsmodesettings.h"
#include "views/dolphinview.h"

#ifdef HAVE_KACTIVITIES
#include <KActivities/ResourceInstance>
#endif
#include <KFileItemActions>
#include <KFilePlacesModel>
#include <KIO/PreviewJob>
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KLocalizedString>
#include <KMessageWidget>
#include <KProtocolManager>
#include <KShell>
#include <KUrlComboBox>
#include <KUrlNavigator>

#include <QDropEvent>
#include <QLoggingCategory>
#include <QMimeData>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QDesktopServices>

DolphinViewContainer::DolphinViewContainer(const QUrl& url, QWidget* parent) :
    QWidget(parent),
    m_topLayout(nullptr),
    m_navigatorWidget(nullptr),
    m_urlNavigator(nullptr),
    m_emptyTrashButton(nullptr),
    m_searchBox(nullptr),
    m_searchModeEnabled(false),
    m_messageWidget(nullptr),
    m_view(nullptr),
    m_filterBar(nullptr),
    m_statusBar(nullptr),
    m_statusBarTimer(nullptr),
    m_statusBarTimestamp(),
    m_autoGrabFocus(true)
#ifdef HAVE_KACTIVITIES
    , m_activityResourceInstance(nullptr)
#endif
{
    hide();

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setSpacing(0);
    m_topLayout->setContentsMargins(0, 0, 0, 0);

    m_navigatorWidget = new QWidget(this);
    QHBoxLayout* navigatorLayout = new QHBoxLayout(m_navigatorWidget);
    navigatorLayout->setSpacing(0);
    navigatorLayout->setContentsMargins(0, 0, 0, 0);
    m_navigatorWidget->setWhatsThis(xi18nc("@info:whatsthis location bar",
        "<para>This line describes the location of the files and folders "
        "displayed below.</para><para>The name of the currently viewed "
        "folder can be read at the very right. To the left of it is the "
        "name of the folder that contains it. The whole line is called "
        "the <emphasis>path</emphasis> to the current location because "
        "following these folders from left to right leads here.</para>"
        "<para>The path is displayed on the <emphasis>location bar</emphasis> "
        "which is more powerful than one would expect. To learn more "
        "about the basic and advanced features of the location bar "
        "<link url='help:/dolphin/location-bar.html'>click here</link>. "
        "This will open the dedicated page in the Handbook.</para>"));

    m_urlNavigator = new KUrlNavigator(DolphinPlacesModelSingleton::instance().placesModel(), url, this);
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

    m_emptyTrashButton = new QPushButton(QIcon::fromTheme(QStringLiteral("user-trash")), i18nc("@action:button", "Empty Trash"), this);
    m_emptyTrashButton->setFlat(true);
    connect(m_emptyTrashButton, &QPushButton::clicked, this, [this]() { Trash::empty(this); });
    connect(&Trash::instance(), &Trash::emptinessChanged, m_emptyTrashButton, &QPushButton::setDisabled);
    m_emptyTrashButton->setDisabled(Trash::isEmpty());
    m_emptyTrashButton->hide();

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
    m_filterBar->setVisible(settings->filterBar());

    connect(m_filterBar, &FilterBar::filterChanged,
            this, &DolphinViewContainer::setNameFilter);
    connect(m_filterBar, &FilterBar::closeRequest,
            this, &DolphinViewContainer::closeFilterBar);
    connect(m_filterBar, &FilterBar::focusViewRequest,
            this, &DolphinViewContainer::requestFocus);

    // Initialize the main view
    m_view = new DolphinView(url, this);
    connect(m_view, &DolphinView::urlChanged,
            m_filterBar, &FilterBar::slotUrlChanged);
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
        m_view->dropUrls(destination, event, m_urlNavigator->dropWidget());
    });

    connect(m_view, &DolphinView::directoryLoadingCompleted, this, [this]() {
        m_emptyTrashButton->setVisible(m_view->url().scheme() == QLatin1String("trash"));
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

    navigatorLayout->addWidget(m_urlNavigator);
    navigatorLayout->addWidget(m_emptyTrashButton);

    m_topLayout->addWidget(m_navigatorWidget);
    m_topLayout->addWidget(m_searchBox);
    m_topLayout->addWidget(m_messageWidget);
    m_topLayout->addWidget(m_view);
    m_topLayout->addWidget(m_filterBar);
    m_topLayout->addWidget(m_statusBar);

    setSearchModeEnabled(isSearchUrl(url));

    connect(DetailsModeSettings::self(), &KCoreConfigSkeleton::configChanged, this, [=]() {
        if (view()->mode() == DolphinView::Mode::DetailsView) {
            view()->reload();
        }
    });

    // Initialize kactivities resource instance

#ifdef HAVE_KACTIVITIES
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

void DolphinViewContainer::setActive(bool active)
{
    m_searchBox->setActive(active);
    m_urlNavigator->setActive(active);
    m_view->setActive(active);

#ifdef HAVE_KACTIVITIES
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

QString DolphinViewContainer::currentSearchText() const
{
     return m_searchBox->text();
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

    // TODO: wrap at arbitrary character positions once QLabel can do this
    // https://bugreports.qt.io/browse/QTBUG-1276
    m_messageWidget->setWordWrap(true);

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
    m_searchBox->setVisible(enabled);
    m_navigatorWidget->setVisible(!enabled);

    if (enabled) {
        const QUrl& locationUrl = m_urlNavigator->locationUrl();
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
        m_urlNavigator->setLocationUrl(url);
    }

    m_searchModeEnabled = enabled;

    emit searchModeEnabledChanged(enabled);
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
        if (currentSearchText().isEmpty()){
            return i18n("Search");
        } else {
            return i18n("Search for %1", currentSearchText());
        }
    }

    KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();
    const auto& matchedPlaces = placesModel->match(placesModel->index(0,0), KFilePlacesModel::UrlRole, QUrl(url().adjusted(QUrl::StripTrailingSlash).toString(QUrl::FullyEncoded).append("/?")), 1, Qt::MatchRegExp);

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

void DolphinViewContainer::setUrl(const QUrl& newUrl)
{
    if (newUrl != m_urlNavigator->locationUrl()) {
        m_urlNavigator->setLocationUrl(newUrl);
    }

#ifdef HAVE_KACTIVITIES
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

    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(item.targetUrl());
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    job->setShowOpenOrExecuteDialog(true);
    job->start();
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
    m_view->hideToolTip(ToolTipManager::HideBehavior::Instantly);
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
        if (url.scheme().startsWith(QLatin1String("http"))) {
            showMessage(i18nc("@info:status", // krazy:exclude=qmethods
                              "Dolphin does not support web pages, the web browser has been launched"),
                        Information);
        } else {
            showMessage(i18nc("@info:status",
                              "Protocol not supported by Dolphin, default application has been launched"),
                        Information);
        }

        QDesktopServices::openUrl(url);
        redirect(QUrl(), m_urlNavigator->locationUrl(1));
    } else {
        showMessage(i18nc("@info:status", "Invalid protocol"), Error);
    }
}

void DolphinViewContainer::slotUrlSelectionRequested(const QUrl& url)
{
    m_view->markUrlsAsSelected({url});
    m_view->markUrlAsCurrent(url); // makes the item scroll into view
}

void DolphinViewContainer::redirect(const QUrl& oldUrl, const QUrl& newUrl)
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
