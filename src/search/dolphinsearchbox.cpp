/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinsearchbox.h"
#include "global.h"

#include "dolphin_searchsettings.h"
#include "dolphinfacetswidget.h"
#include "dolphinplacesmodelsingleton.h"
#include "dolphinquery.h"

#include "config-dolphin.h"
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KSeparator>
#include <KService>
#if HAVE_BALOO
#include <Baloo/IndexerConfig>
#include <Baloo/Query>
#endif

#include <QButtonGroup>
#include <QDir>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QScrollArea>
#include <QShowEvent>
#include <QTimer>
#include <QToolButton>
#include <QUrlQuery>

DolphinSearchBox::DolphinSearchBox(QWidget *parent)
    : AnimatedHeightWidget(parent)
    , m_startedSearching(false)
    , m_active(true)
    , m_topLayout(nullptr)
    , m_searchInput(nullptr)
    , m_saveSearchAction(nullptr)
    , m_optionsScrollArea(nullptr)
    , m_fileNameButton(nullptr)
    , m_contentButton(nullptr)
    , m_separator(nullptr)
    , m_fromHereButton(nullptr)
    , m_everywhereButton(nullptr)
    , m_facetsWidget(nullptr)
    , m_searchPath()
    , m_startSearchTimer(nullptr)
    , m_initialized(false)
{
}

DolphinSearchBox::~DolphinSearchBox()
{
    saveSettings();
}

void DolphinSearchBox::setText(const QString &text)
{
    if (m_searchInput->text() != text) {
        m_searchInput->setText(text);
    }
}

QString DolphinSearchBox::text() const
{
    return m_searchInput->text();
}

void DolphinSearchBox::setSearchPath(const QUrl &url)
{
    if (url == m_searchPath) {
        return;
    }

    const QUrl cleanedUrl = url.adjusted(QUrl::RemoveUserInfo | QUrl::StripTrailingSlash);

    if (cleanedUrl.path() == QDir::homePath()) {
        m_fromHereButton->setChecked(false);
        m_everywhereButton->setChecked(true);
        if (!m_searchPath.isEmpty()) {
            return;
        }
    } else {
        m_everywhereButton->setChecked(false);
        m_fromHereButton->setChecked(true);
    }

    m_searchPath = url;

    QFontMetrics metrics(m_fromHereButton->font());
    const int maxWidth = metrics.height() * 8;

    QString location = cleanedUrl.fileName();
    if (location.isEmpty()) {
        location = cleanedUrl.toString(QUrl::PreferLocalFile);
    }
    const QString elidedLocation = metrics.elidedText(location, Qt::ElideMiddle, maxWidth);
    m_fromHereButton->setText(i18nc("action:button", "From Here (%1)", elidedLocation));
    m_fromHereButton->setToolTip(i18nc("action:button", "Limit search to '%1' and its subfolders", cleanedUrl.toString(QUrl::PreferLocalFile)));
}

QUrl DolphinSearchBox::searchPath() const
{
    return m_everywhereButton->isChecked() ? QUrl::fromLocalFile(QDir::homePath()) : m_searchPath;
}

QUrl DolphinSearchBox::urlForSearching() const
{
    QUrl url;

    if (isIndexingEnabled()) {
        url = balooUrlForSearching();
    } else {
        url.setScheme(QStringLiteral("filenamesearch"));

        QUrlQuery query;
        query.addQueryItem(QStringLiteral("search"), QUrl::toPercentEncoding(m_searchInput->text()));
        if (m_contentButton->isChecked()) {
            query.addQueryItem(QStringLiteral("checkContent"), QStringLiteral("yes"));
        }

        query.addQueryItem(QStringLiteral("url"), QUrl::toPercentEncoding(searchPath().url()));
        query.addQueryItem(QStringLiteral("title"), queryTitle(m_searchInput->text()));

        url.setQuery(query);
    }

    return url;
}

void DolphinSearchBox::fromSearchUrl(const QUrl &url)
{
    if (DolphinQuery::supportsScheme(url.scheme())) {
        const DolphinQuery query = DolphinQuery::fromSearchUrl(url);
        updateFromQuery(query);
    } else if (url.scheme() == QLatin1String("filenamesearch")) {
        const QUrlQuery query(url);
        setText(query.queryItemValue(QStringLiteral("search"), QUrl::FullyDecoded));
        if (m_searchPath.scheme() != url.scheme()) {
            m_searchPath = QUrl();
        }
        setSearchPath(QUrl::fromUserInput(query.queryItemValue(QStringLiteral("url"), QUrl::FullyDecoded), QString(), QUrl::AssumeLocalFile));
        m_contentButton->setChecked(query.queryItemValue(QStringLiteral("checkContent")) == QLatin1String("yes"));
    } else {
        setText(QString());
        m_searchPath = QUrl();
        setSearchPath(url);
    }

    updateFacetsVisible();
}

void DolphinSearchBox::selectAll()
{
    m_searchInput->selectAll();
}

void DolphinSearchBox::setActive(bool active)
{
    if (active != m_active) {
        m_active = active;

        if (active) {
            Q_EMIT activated();
        }
    }
}

bool DolphinSearchBox::isActive() const
{
    return m_active;
}

void DolphinSearchBox::setVisible(bool visible, Animated animated)
{
    if (visible) {
        init();
    }
    AnimatedHeightWidget::setVisible(visible, animated);
}

void DolphinSearchBox::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        m_searchInput->setFocus();
        m_startedSearching = false;
    }
}

void DolphinSearchBox::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    m_startedSearching = false;
    if (m_startSearchTimer) {
        m_startSearchTimer->stop();
    }
}

void DolphinSearchBox::keyReleaseEvent(QKeyEvent *event)
{
    QWidget::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Escape) {
        if (m_searchInput->text().isEmpty()) {
            emitCloseRequest();
        } else {
            m_searchInput->clear();
        }
    } else if (event->key() == Qt::Key_Down) {
        Q_EMIT focusViewRequest();
    }
}

bool DolphinSearchBox::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::FocusIn:
        // #379135: we get the FocusIn event when we close a tab but we don't want to emit
        // the activated() signal before the removeTab() call in DolphinTabWidget::closeTab() returns.
        // To avoid this issue, we delay the activation of the search box.
        // We also don't want to schedule the activation process if we are already active,
        // otherwise we can enter in a loop of FocusIn/FocusOut events with the searchbox of another tab.
        if (!isActive()) {
            QTimer::singleShot(0, this, [this] {
                setActive(true);
                setFocus();
            });
        }
        break;

    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

void DolphinSearchBox::emitSearchRequest()
{
    m_startSearchTimer->stop();
    m_startedSearching = true;
    m_saveSearchAction->setEnabled(true);
    Q_EMIT searchRequest();
}

void DolphinSearchBox::emitCloseRequest()
{
    m_startSearchTimer->stop();
    m_startedSearching = false;
    m_saveSearchAction->setEnabled(false);
    Q_EMIT closeRequest();
}

void DolphinSearchBox::slotConfigurationChanged()
{
    saveSettings();
    if (m_startedSearching) {
        emitSearchRequest();
    }
}

void DolphinSearchBox::slotSearchTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        // Restore URL when search box is cleared by closing and reopening the box.
        emitCloseRequest();
        Q_EMIT openRequest();
    } else {
        m_startSearchTimer->start();
    }
    Q_EMIT searchTextChanged(text);
}

void DolphinSearchBox::slotReturnPressed()
{
    if (m_searchInput->text().isEmpty()) {
        return;
    }

    emitSearchRequest();
    Q_EMIT focusViewRequest();
}

void DolphinSearchBox::slotFacetChanged()
{
    m_startedSearching = true;
    m_startSearchTimer->stop();
    Q_EMIT searchRequest();
}

void DolphinSearchBox::slotSearchSaved()
{
    const QUrl searchURL = urlForSearching();
    if (searchURL.isValid()) {
        const QString label = i18n("Search for %1 in %2", text(), searchPath().fileName());
        DolphinPlacesModelSingleton::instance().placesModel()->addPlace(label, searchURL, QStringLiteral("folder-saved-search-symbolic"));
    }
}

void DolphinSearchBox::initButton(QToolButton *button)
{
    button->installEventFilter(this);
    button->setAutoExclusive(true);
    button->setAutoRaise(true);
    button->setCheckable(true);
    connect(button, &QToolButton::clicked, this, &DolphinSearchBox::slotConfigurationChanged);
}

void DolphinSearchBox::loadSettings()
{
    if (SearchSettings::location() == QLatin1String("Everywhere")) {
        m_everywhereButton->setChecked(true);
    } else {
        m_fromHereButton->setChecked(true);
    }

    if (SearchSettings::what() == QLatin1String("Content")) {
        m_contentButton->setChecked(true);
    } else {
        m_fileNameButton->setChecked(true);
    }

    updateFacetsVisible();
}

void DolphinSearchBox::saveSettings()
{
    if (m_initialized) {
        SearchSettings::setLocation(m_fromHereButton->isChecked() ? QStringLiteral("FromHere") : QStringLiteral("Everywhere"));
        SearchSettings::setWhat(m_fileNameButton->isChecked() ? QStringLiteral("FileName") : QStringLiteral("Content"));
        SearchSettings::self()->save();
    }
}

void DolphinSearchBox::init()
{
    if (m_initialized) {
        return; // This object is already initialised.
    }

    QWidget *contentsContainer = prepareContentsContainer();

    // Create search box
    m_searchInput = new QLineEdit(contentsContainer);
    m_searchInput->setPlaceholderText(i18n("Search…"));
    m_searchInput->installEventFilter(this);
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    connect(m_searchInput, &QLineEdit::returnPressed, this, &DolphinSearchBox::slotReturnPressed);
    connect(m_searchInput, &QLineEdit::textChanged, this, &DolphinSearchBox::slotSearchTextChanged);
    setFocusProxy(m_searchInput);

    // Add "Save search" button inside search box
    m_saveSearchAction = new QAction(this);
    m_saveSearchAction->setIcon(QIcon::fromTheme(QStringLiteral("document-save-symbolic")));
    m_saveSearchAction->setText(i18nc("action:button", "Save this search to quickly access it again in the future"));
    m_saveSearchAction->setEnabled(false);
    m_searchInput->addAction(m_saveSearchAction, QLineEdit::TrailingPosition);
    connect(m_saveSearchAction, &QAction::triggered, this, &DolphinSearchBox::slotSearchSaved);

    // Create close button
    QToolButton *closeButton = new QToolButton(contentsContainer);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeButton->setToolTip(i18nc("@info:tooltip", "Quit searching"));
    connect(closeButton, &QToolButton::clicked, this, &DolphinSearchBox::emitCloseRequest);

    // Apply layout for the search input
    QHBoxLayout *searchInputLayout = new QHBoxLayout();
    searchInputLayout->setContentsMargins(0, 0, 0, 0);
    searchInputLayout->addWidget(m_searchInput);
    searchInputLayout->addWidget(closeButton);

    // Create "Filename" and "Content" button
    m_fileNameButton = new QToolButton(contentsContainer);
    m_fileNameButton->setText(i18nc("action:button", "Filename"));
    initButton(m_fileNameButton);

    m_contentButton = new QToolButton();
    m_contentButton->setText(i18nc("action:button", "Content"));
    initButton(m_contentButton);

    QButtonGroup *searchWhatGroup = new QButtonGroup(contentsContainer);
    searchWhatGroup->addButton(m_fileNameButton);
    searchWhatGroup->addButton(m_contentButton);

    m_separator = new KSeparator(Qt::Vertical, contentsContainer);

    // Create "From Here" and "Your files" buttons
    m_fromHereButton = new QToolButton(contentsContainer);
    m_fromHereButton->setText(i18nc("action:button", "From Here"));
    initButton(m_fromHereButton);

    m_everywhereButton = new QToolButton(contentsContainer);
    m_everywhereButton->setText(i18nc("action:button", "Your files"));
    m_everywhereButton->setToolTip(i18nc("action:button", "Search in your home directory"));
    m_everywhereButton->setIcon(QIcon::fromTheme(QStringLiteral("user-home")));
    m_everywhereButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    initButton(m_everywhereButton);

    QButtonGroup *searchLocationGroup = new QButtonGroup(contentsContainer);
    searchLocationGroup->addButton(m_fromHereButton);
    searchLocationGroup->addButton(m_everywhereButton);

    KService::Ptr kfind = KService::serviceByDesktopName(QStringLiteral("org.kde.kfind"));

    QToolButton *kfindToolsButton = nullptr;
    if (kfind) {
        kfindToolsButton = new QToolButton(contentsContainer);
        kfindToolsButton->setAutoRaise(true);
        kfindToolsButton->setPopupMode(QToolButton::InstantPopup);
        kfindToolsButton->setIcon(QIcon::fromTheme("arrow-down-double"));
        kfindToolsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        kfindToolsButton->setText(i18n("Open %1", kfind->name()));
        kfindToolsButton->setIcon(QIcon::fromTheme(kfind->icon()));

        connect(kfindToolsButton, &QToolButton::clicked, this, [this, kfind] {
            auto *job = new KIO::ApplicationLauncherJob(kfind);
            job->setUrls({m_searchPath});
            job->start();
        });
    }

    // Create "Facets" widget
    m_facetsWidget = new DolphinFacetsWidget(contentsContainer);
    m_facetsWidget->installEventFilter(this);
    m_facetsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_facetsWidget->layout()->setSpacing(Dolphin::LAYOUT_SPACING_SMALL);
    connect(m_facetsWidget, &DolphinFacetsWidget::facetChanged, this, &DolphinSearchBox::slotFacetChanged);

    // Put the options into a QScrollArea. This prevents increasing the view width
    // in case that not enough width for the options is available.
    QWidget *optionsContainer = new QWidget(contentsContainer);

    // Apply layout for the options
    QHBoxLayout *optionsLayout = new QHBoxLayout(optionsContainer);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(Dolphin::LAYOUT_SPACING_SMALL);
    optionsLayout->addWidget(m_fileNameButton);
    optionsLayout->addWidget(m_contentButton);
    optionsLayout->addWidget(m_separator);
    optionsLayout->addWidget(m_fromHereButton);
    optionsLayout->addWidget(m_everywhereButton);
    optionsLayout->addWidget(new KSeparator(Qt::Vertical, contentsContainer));
    if (kfindToolsButton) {
        optionsLayout->addWidget(kfindToolsButton);
    }
    optionsLayout->addStretch(1);

    m_optionsScrollArea = new QScrollArea(contentsContainer);
    m_optionsScrollArea->setFrameShape(QFrame::NoFrame);
    m_optionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_optionsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_optionsScrollArea->setMaximumHeight(optionsContainer->sizeHint().height());
    m_optionsScrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_optionsScrollArea->setWidget(optionsContainer);
    m_optionsScrollArea->setWidgetResizable(true);

    m_topLayout = new QVBoxLayout(contentsContainer);
    m_topLayout->setContentsMargins(0, Dolphin::LAYOUT_SPACING_SMALL, 0, 0);
    m_topLayout->setSpacing(Dolphin::LAYOUT_SPACING_SMALL);
    m_topLayout->addLayout(searchInputLayout);
    m_topLayout->addWidget(m_optionsScrollArea);
    m_topLayout->addWidget(m_facetsWidget);

    loadSettings();

    // The searching should be started automatically after the user did not change
    // the text for a while
    m_startSearchTimer = new QTimer(this);
    m_startSearchTimer->setSingleShot(true);
    m_startSearchTimer->setInterval(500);
    connect(m_startSearchTimer, &QTimer::timeout, this, &DolphinSearchBox::emitSearchRequest);

    m_initialized = true;
}

QString DolphinSearchBox::queryTitle(const QString &text) const
{
    return i18nc("@title UDS_DISPLAY_NAME for a KIO directory listing. %1 is the query the user entered.", "Query Results from '%1'", text);
}

QUrl DolphinSearchBox::balooUrlForSearching() const
{
#if HAVE_BALOO
    const QString text = m_searchInput->text();

    Baloo::Query query;
    query.addType(m_facetsWidget->facetType());

    QStringList queryStrings = m_facetsWidget->searchTerms();

    if (m_contentButton->isChecked()) {
        queryStrings << text;
    } else if (!text.isEmpty()) {
        queryStrings << QStringLiteral("filename:\"%1\"").arg(text);
    }

    if (m_fromHereButton->isChecked()) {
        query.setIncludeFolder(m_searchPath.toLocalFile());
    }

    query.setSearchString(queryStrings.join(QLatin1Char(' ')));

    return query.toSearchUrl(queryTitle(text));
#else
    return QUrl();
#endif
}

void DolphinSearchBox::updateFromQuery(const DolphinQuery &query)
{
    // Block all signals to avoid unnecessary "searchRequest" signals
    // while we adjust the search text and the facet widget.
    blockSignals(true);

    const QString customDir = query.includeFolder();
    if (!customDir.isEmpty()) {
        setSearchPath(QUrl::fromLocalFile(customDir));
    } else {
        setSearchPath(QUrl::fromLocalFile(QDir::homePath()));
    }

    setText(query.text());

    if (query.hasContentSearch()) {
        m_contentButton->setChecked(true);
    } else if (query.hasFileName()) {
        m_fileNameButton->setChecked(true);
    }

    m_facetsWidget->resetSearchTerms();
    m_facetsWidget->setFacetType(query.type());
    const QStringList searchTerms = query.searchTerms();
    for (const QString &searchTerm : searchTerms) {
        m_facetsWidget->setSearchTerm(searchTerm);
    }

    m_startSearchTimer->stop();
    blockSignals(false);
}

void DolphinSearchBox::updateFacetsVisible()
{
    const bool indexingEnabled = isIndexingEnabled();
    m_facetsWidget->setEnabled(indexingEnabled);
    m_facetsWidget->setVisible(indexingEnabled);

    // The m_facetsWidget might have changed visibility. We smoothly animate towards the updated height.
    if (isVisible() && isEnabled()) {
        setVisible(true, WithAnimation);
    }
}

bool DolphinSearchBox::isIndexingEnabled() const
{
#if HAVE_BALOO
    const Baloo::IndexerConfig searchInfo;
    return searchInfo.fileIndexingEnabled() && !searchPath().isEmpty() && searchInfo.shouldBeIndexed(searchPath().toLocalFile());
#else
    return false;
#endif
}

int DolphinSearchBox::preferredHeight() const
{
    return m_initialized ? m_topLayout->sizeHint().height() : 0;
}

#include "moc_dolphinsearchbox.cpp"
