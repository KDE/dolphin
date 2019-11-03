/***************************************************************************
*    Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>            *
*                                                                         *
*    This program is free software; you can redistribute it and/or modify *
*    it under the terms of the GNU General Public License as published by *
*    the Free Software Foundation; either version 2 of the License, or    *
*    (at your option) any later version.                                  *
*                                                                         *
*    This program is distributed in the hope that it will be useful,      *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*    GNU General Public License for more details.                         *
*                                                                         *
*    You should have received a copy of the GNU General Public License    *
*    along with this program; if not, write to the                        *
*    Free Software Foundation, Inc.,                                      *
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA           *
* **************************************************************************/

#include "dolphinsearchbox.h"

#include "dolphin_searchsettings.h"
#include "dolphinfacetswidget.h"
#include "panels/places/placesitemmodel.h"

#include <KLocalizedString>
#include <KNS3/KMoreToolsMenuFactory>
#include <KSeparator>
#include <config-baloo.h>
#ifdef HAVE_BALOO
#include <Baloo/Query>
#include <Baloo/IndexerConfig>
#endif

#include <QButtonGroup>
#include <QDir>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QUrlQuery>

DolphinSearchBox::DolphinSearchBox(QWidget* parent) :
    QWidget(parent),
    m_startedSearching(false),
    m_active(true),
    m_topLayout(nullptr),
    m_searchInput(nullptr),
    m_saveSearchAction(nullptr),
    m_optionsScrollArea(nullptr),
    m_fileNameButton(nullptr),
    m_contentButton(nullptr),
    m_separator(nullptr),
    m_fromHereButton(nullptr),
    m_everywhereButton(nullptr),
    m_facetsWidget(nullptr),
    m_searchPath(),
    m_startSearchTimer(nullptr)
{
}

DolphinSearchBox::~DolphinSearchBox()
{
    saveSettings();
}

void DolphinSearchBox::setText(const QString& text)
{
    m_searchInput->setText(text);
}

QString DolphinSearchBox::text() const
{
    return m_searchInput->text();
}

void DolphinSearchBox::setSearchPath(const QUrl& url)
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
        query.addQueryItem(QStringLiteral("search"), m_searchInput->text());
        if (m_contentButton->isChecked()) {
            query.addQueryItem(QStringLiteral("checkContent"), QStringLiteral("yes"));
        }

        query.addQueryItem(QStringLiteral("url"), searchPath().url());

        url.setQuery(query);
    }

    return url;
}

void DolphinSearchBox::fromSearchUrl(const QUrl& url)
{
    if (url.scheme() == QLatin1String("baloosearch")) {
        fromBalooSearchUrl(url);
    } else if (url.scheme() == QLatin1String("filenamesearch")) {
        const QUrlQuery query(url);
        setText(query.queryItemValue(QStringLiteral("search")));
        if (m_searchPath.scheme() != url.scheme()) {
            m_searchPath = QUrl();
        }
        setSearchPath(QUrl::fromUserInput(query.queryItemValue(QStringLiteral("url")), QString(), QUrl::AssumeLocalFile));
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
            emit activated();
        }
    }
}

bool DolphinSearchBox::isActive() const
{
    return m_active;
}

bool DolphinSearchBox::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        init();
    }
    return QWidget::event(event);
}

void DolphinSearchBox::showEvent(QShowEvent* event)
{
    if (!event->spontaneous()) {
        m_searchInput->setFocus();
        m_startedSearching = false;
    }
}

void DolphinSearchBox::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    m_startedSearching = false;
    m_startSearchTimer->stop();
}

void DolphinSearchBox::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Escape) {
        if (m_searchInput->text().isEmpty()) {
            emit closeRequest();
        } else {
            m_searchInput->clear();
        }
    }
}

bool DolphinSearchBox::eventFilter(QObject* obj, QEvent* event)
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
    emit searchRequest();
}

void DolphinSearchBox::emitCloseRequest()
{
    m_startSearchTimer->stop();
    m_startedSearching = false;
    m_saveSearchAction->setEnabled(false);
    emit closeRequest();
}

void DolphinSearchBox::slotConfigurationChanged()
{
    saveSettings();
    if (m_startedSearching) {
        emitSearchRequest();
    }
}

void DolphinSearchBox::slotSearchTextChanged(const QString& text)
{

    if (text.isEmpty()) {
        m_startSearchTimer->stop();
    } else {
        m_startSearchTimer->start();
    }
    emit searchTextChanged(text);
}

void DolphinSearchBox::slotReturnPressed()
{
    emitSearchRequest();
    emit returnPressed();
}

void DolphinSearchBox::slotFacetChanged()
{
    m_startedSearching = true;
    m_startSearchTimer->stop();
    emit searchRequest();
}

void DolphinSearchBox::slotSearchSaved()
{
    const QUrl searchURL = urlForSearching();
    if (searchURL.isValid()) {
        PlacesItemModel model;
        const QString label = i18n("Search for %1 in %2", text(), searchPath().fileName());
        model.createPlacesItem(label,
                               searchURL,
                               QStringLiteral("folder-saved-search-symbolic"));
    }
}

void DolphinSearchBox::initButton(QToolButton* button)
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
    SearchSettings::setLocation(m_fromHereButton->isChecked() ? QStringLiteral("FromHere") : QStringLiteral("Everywhere"));
    SearchSettings::setWhat(m_fileNameButton->isChecked() ? QStringLiteral("FileName") : QStringLiteral("Content"));
    SearchSettings::self()->save();
}

void DolphinSearchBox::init()
{
    // Create close button
    QToolButton* closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeButton->setToolTip(i18nc("@info:tooltip", "Quit searching"));
    connect(closeButton, &QToolButton::clicked, this, &DolphinSearchBox::emitCloseRequest);

    // Create search box
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(i18n("Search..."));
    m_searchInput->installEventFilter(this);
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    connect(m_searchInput, &QLineEdit::returnPressed,
            this, &DolphinSearchBox::slotReturnPressed);
    connect(m_searchInput, &QLineEdit::textChanged,
            this, &DolphinSearchBox::slotSearchTextChanged);
    setFocusProxy(m_searchInput);

    // Add "Save search" button inside search box
    m_saveSearchAction = new QAction(this);
    m_saveSearchAction->setIcon (QIcon::fromTheme(QStringLiteral("document-save-symbolic")));
    m_saveSearchAction->setText(i18nc("action:button", "Save this search to quickly access it again in the future"));
    m_saveSearchAction->setEnabled(false);
    m_searchInput->addAction(m_saveSearchAction, QLineEdit::TrailingPosition);
    connect(m_saveSearchAction, &QAction::triggered, this, &DolphinSearchBox::slotSearchSaved);

    // Apply layout for the search input
    QHBoxLayout* searchInputLayout = new QHBoxLayout();
    searchInputLayout->setContentsMargins(0, 0, 0, 0);
    searchInputLayout->addWidget(closeButton);
    searchInputLayout->addWidget(m_searchInput);

    // Create "Filename" and "Content" button
    m_fileNameButton = new QToolButton(this);
    m_fileNameButton->setText(i18nc("action:button", "Filename"));
    initButton(m_fileNameButton);

    m_contentButton = new QToolButton();
    m_contentButton->setText(i18nc("action:button", "Content"));
    initButton(m_contentButton);

    QButtonGroup* searchWhatGroup = new QButtonGroup(this);
    searchWhatGroup->addButton(m_fileNameButton);
    searchWhatGroup->addButton(m_contentButton);

    m_separator = new KSeparator(Qt::Vertical, this);

    // Create "From Here" and "Your files" buttons
    m_fromHereButton = new QToolButton(this);
    m_fromHereButton->setText(i18nc("action:button", "From Here"));
    initButton(m_fromHereButton);

    m_everywhereButton = new QToolButton(this);
    m_everywhereButton->setText(i18nc("action:button", "Your files"));
    m_everywhereButton->setToolTip(i18nc("action:button", "Search in your home directory"));
    m_everywhereButton->setIcon(QIcon::fromTheme(QStringLiteral("user-home")));
    m_everywhereButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    initButton(m_everywhereButton);

    QButtonGroup* searchLocationGroup = new QButtonGroup(this);
    searchLocationGroup->addButton(m_fromHereButton);
    searchLocationGroup->addButton(m_everywhereButton);

    auto moreSearchToolsButton = new QToolButton(this);
    moreSearchToolsButton->setAutoRaise(true);
    moreSearchToolsButton->setPopupMode(QToolButton::InstantPopup);
    moreSearchToolsButton->setIcon(QIcon::fromTheme("arrow-down-double"));
    moreSearchToolsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    moreSearchToolsButton->setText(i18n("More Search Tools"));
    moreSearchToolsButton->setMenu(new QMenu(this));
    connect(moreSearchToolsButton->menu(), &QMenu::aboutToShow, moreSearchToolsButton->menu(), [this, moreSearchToolsButton]()
    {
        m_menuFactory.reset(new KMoreToolsMenuFactory("dolphin/search-tools"));
        moreSearchToolsButton->menu()->clear();
        m_menuFactory->fillMenuFromGroupingNames(moreSearchToolsButton->menu(), { "files-find" }, this->m_searchPath);
    } );

    // Create "Facets" widget
    m_facetsWidget = new DolphinFacetsWidget(this);
    m_facetsWidget->installEventFilter(this);
    m_facetsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    connect(m_facetsWidget, &DolphinFacetsWidget::facetChanged, this, &DolphinSearchBox::slotFacetChanged);

    // Apply layout for the options
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->addWidget(m_fileNameButton);
    optionsLayout->addWidget(m_contentButton);
    optionsLayout->addWidget(m_separator);
    optionsLayout->addWidget(m_fromHereButton);
    optionsLayout->addWidget(m_everywhereButton);
    optionsLayout->addWidget(new KSeparator(Qt::Vertical, this));
    optionsLayout->addWidget(moreSearchToolsButton);
    optionsLayout->addStretch(1);

    // Put the options into a QScrollArea. This prevents increasing the view width
    // in case that not enough width for the options is available.
    QWidget* optionsContainer = new QWidget(this);
    optionsContainer->setLayout(optionsLayout);

    m_optionsScrollArea = new QScrollArea(this);
    m_optionsScrollArea->setFrameShape(QFrame::NoFrame);
    m_optionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_optionsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_optionsScrollArea->setMaximumHeight(optionsContainer->sizeHint().height());
    m_optionsScrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_optionsScrollArea->setWidget(optionsContainer);
    m_optionsScrollArea->setWidgetResizable(true);

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setContentsMargins(0, 0, 0, 0);
    m_topLayout->addLayout(searchInputLayout);
    m_topLayout->addWidget(m_optionsScrollArea);
    m_topLayout->addWidget(m_facetsWidget);

    loadSettings();

    // The searching should be started automatically after the user did not change
    // the text within one second
    m_startSearchTimer = new QTimer(this);
    m_startSearchTimer->setSingleShot(true);
    m_startSearchTimer->setInterval(1000);
    connect(m_startSearchTimer, &QTimer::timeout, this, &DolphinSearchBox::emitSearchRequest);
}

QUrl DolphinSearchBox::balooUrlForSearching() const
{
#ifdef HAVE_BALOO
    const QString text = m_searchInput->text();

    Baloo::Query query;
    query.addType(m_facetsWidget->facetType());

    QStringList queryStrings;
    QString ratingQuery = m_facetsWidget->ratingTerm();
    if (!ratingQuery.isEmpty()) {
        queryStrings << ratingQuery;
    }

    if (m_contentButton->isChecked()) {
        queryStrings << text;
    } else if (!text.isEmpty()) {
        queryStrings << QStringLiteral("filename:\"%1\"").arg(text);
    }

    if (m_fromHereButton->isChecked()) {
        query.setIncludeFolder(m_searchPath.toLocalFile());
    }

    query.setSearchString(queryStrings.join(QLatin1Char(' ')));

    return query.toSearchUrl(i18nc("@title UDS_DISPLAY_NAME for a KIO directory listing. %1 is the query the user entered.",
                                   "Query Results from '%1'", text));
#else
    return QUrl();
#endif
}

void DolphinSearchBox::fromBalooSearchUrl(const QUrl& url)
{
#ifdef HAVE_BALOO
    const Baloo::Query query = Baloo::Query::fromSearchUrl(url);

    // Block all signals to avoid unnecessary "searchRequest" signals
    // while we adjust the search text and the facet widget.
    blockSignals(true);

    const QString customDir = query.includeFolder();
    if (!customDir.isEmpty()) {
        setSearchPath(QUrl::fromLocalFile(customDir));
    } else {
        setSearchPath(QUrl::fromLocalFile(QDir::homePath()));
    }

    m_facetsWidget->resetOptions();

    setText(query.searchString());

    QStringList types = query.types();
    if (!types.isEmpty()) {
        m_facetsWidget->setFacetType(types.first());
    }

    const QStringList subTerms = query.searchString().split(' ', QString::SkipEmptyParts);
    foreach (const QString& subTerm, subTerms) {
        if (subTerm.startsWith(QLatin1String("filename:"))) {
            const QString value = subTerm.mid(9);
            setText(value);
        } else if (m_facetsWidget->isRatingTerm(subTerm)) {
            m_facetsWidget->setRatingTerm(subTerm);
        }
    }

    m_startSearchTimer->stop();
    blockSignals(false);
#else
    Q_UNUSED(url);
#endif
}

void DolphinSearchBox::updateFacetsVisible()
{
    const bool indexingEnabled = isIndexingEnabled();
    m_facetsWidget->setEnabled(indexingEnabled);
    m_facetsWidget->setVisible(indexingEnabled);
}

bool DolphinSearchBox::isIndexingEnabled() const
{
#ifdef HAVE_BALOO
    const Baloo::IndexerConfig searchInfo;
    return searchInfo.fileIndexingEnabled() && searchInfo.shouldBeIndexed(searchPath().toLocalFile());
#else
    return false;
#endif
}
