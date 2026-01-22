/*
    SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "bar.h"
#include "global.h"

#include "barsecondrowflowlayout.h"
#include "chip.h"
#include "dolphin_searchsettings.h"
#include "dolphinplacesmodelsingleton.h"
#include "popup.h"
#include "widgetmenu.h"

#include "config-dolphin.h"
#include <KLocalizedString>

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>

using namespace Search;

namespace
{
/**
 * @see Bar::IsSearchConfigured().
 */
bool isSearchConfigured(const std::shared_ptr<const DolphinQuery> &searchConfiguration)
{
    return !searchConfiguration->searchTerm().isEmpty()
        || (searchConfiguration->searchTool() != SearchTool::Filenamesearch
            && (searchConfiguration->fileType() != KFileMetaData::Type::Empty || searchConfiguration->modifiedSinceDate().isValid()
                || searchConfiguration->minimumRating() > 0 || !searchConfiguration->requiredTags().isEmpty()));
};
}

Bar::Bar(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent)
    : AnimatedHeightWidget(parent)
    , UpdatableStateInterface{dolphinQuery}
{
    QWidget *contentsContainer = prepareContentsContainer();

    // Create search box
    m_searchTermEditor = new QLineEdit(contentsContainer);
    m_searchTermEditor->setClearButtonEnabled(true);
    connect(m_searchTermEditor, &QLineEdit::returnPressed, this, &Bar::slotReturnPressed);
    connect(m_searchTermEditor, &QLineEdit::textEdited, this, &Bar::slotSearchTermEdited);
    setFocusProxy(m_searchTermEditor);

    // Add "Save search" button inside search box
    m_saveSearchAction = new QAction(this);
    m_saveSearchAction->setIcon(QIcon::fromTheme(QStringLiteral("document-save-symbolic")));
    m_saveSearchAction->setText(i18nc("action:button", "Save this search to quickly access it again in the future"));
    m_searchTermEditor->addAction(m_saveSearchAction, QLineEdit::TrailingPosition);
    connect(m_saveSearchAction, &QAction::triggered, this, &Bar::slotSaveSearch);

    // Filter button
    auto filterButton = new QToolButton(contentsContainer);
    filterButton->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    filterButton->setText(i18nc("@action:button for changing search options", "Filter"));
    filterButton->setAutoRaise(true);
    filterButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    filterButton->setPopupMode(QToolButton::InstantPopup);
    filterButton->setAttribute(Qt::WA_CustomWhatsThis);
    m_popup = new Popup{m_searchConfiguration, this};
    connect(m_popup, &QMenu::aboutToShow, this, [this]() {
        m_popup->updateStateToMatch(m_searchConfiguration);
    });
    connect(m_popup, &Popup::configurationChanged, this, &Bar::slotConfigurationChanged);
    connect(m_popup, &Popup::showMessage, this, &Bar::showMessage);
    connect(m_popup, &Popup::showInstallationProgress, this, &Bar::showInstallationProgress);
    filterButton->setMenu(m_popup);

    // Create close button
    QToolButton *closeButton = new QToolButton(contentsContainer);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeButton->setToolTip(i18nc("@info:tooltip", "Quit searching"));
    connect(closeButton, &QToolButton::clicked, this, [this]() {
        setVisible(false, WithAnimation);
    });

    // Apply layout for the search input row
    QHBoxLayout *firstRowLayout = new QHBoxLayout{};
    firstRowLayout->setContentsMargins(0, 0, 0, 0);
    firstRowLayout->addWidget(m_searchTermEditor);
    firstRowLayout->addWidget(filterButton);
    firstRowLayout->addWidget(closeButton);

    // Create "From Here" and "Your files" buttons
    m_fromHereButton = new QToolButton(contentsContainer);
    m_fromHereButton->setText(i18nc("action:button search from here", "Here"));
    m_fromHereButton->setAutoRaise(true);
    m_fromHereButton->setCheckable(true);
    connect(m_fromHereButton, &QToolButton::clicked, this, [this]() {
        if (m_searchConfiguration->searchLocations() == SearchLocations::FromHere) {
            return; // Already selected.
        }
        SearchSettings::setLocation(QStringLiteral("FromHere"));
        SearchSettings::self()->save();
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setSearchLocations(SearchLocations::FromHere);
        slotConfigurationChanged(searchConfigurationCopy);
    });

    m_everywhereButton = new QToolButton(contentsContainer);
    m_everywhereButton->setText(i18nc("action:button search everywhere", "Everywhere"));
    m_everywhereButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_everywhereButton->setAutoRaise(true);
    m_everywhereButton->setCheckable(true);
    connect(m_everywhereButton, &QToolButton::clicked, this, [this]() {
        if (m_searchConfiguration->searchLocations() == SearchLocations::Everywhere) {
            return; // Already selected.
        }
        SearchSettings::setLocation(QStringLiteral("Everywhere"));
        SearchSettings::self()->save();
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setSearchLocations(SearchLocations::Everywhere);
        slotConfigurationChanged(searchConfigurationCopy);
    });

    // Apply layout for the location buttons and chips row
    m_secondRowLayout = new BarSecondRowFlowLayout{nullptr};
    m_secondRowLayout->setSpacing(Dolphin::LAYOUT_SPACING_SMALL);
    connect(m_secondRowLayout, &BarSecondRowFlowLayout::heightHintChanged, this, [this]() {
        if (isEnabled()) {
            AnimatedHeightWidget::setVisible(true, WithAnimation);
        }
        // If this Search::Bar is not enabled we can safely assume that this widget is currently in an animation to hide itself and we do nothing.
    });
    m_secondRowLayout->addWidget(m_fromHereButton);
    m_secondRowLayout->addWidget(m_everywhereButton);

    m_topLayout = new QVBoxLayout(contentsContainer);
    m_topLayout->setContentsMargins(Dolphin::LAYOUT_SPACING_SMALL, Dolphin::LAYOUT_SPACING_SMALL, Dolphin::LAYOUT_SPACING_SMALL, Dolphin::LAYOUT_SPACING_SMALL);
    m_topLayout->setSpacing(Dolphin::LAYOUT_SPACING_SMALL);
    m_topLayout->addLayout(firstRowLayout);
    m_topLayout->addLayout(m_secondRowLayout);

    setWhatsThis(xi18nc(
        "@info:whatsthis search bar",
        "<para>This helps you find files and folders.<list><item>Enter a <emphasis>search term</emphasis> in the input field.</item><item>Decide where to "
        "search by pressing the location buttons below the search field. “Here” refers to the location that was open prior to starting a search, so navigating "
        "to a different location first can narrow down the search.</item><item>Press the “%1” button to further refine the manner of searching or the "
        "results.</item><item>Press the “Save” icon to add the current search configuration to the <emphasis>Places panel</emphasis>.</item></list></para>",
        filterButton->text()));

    // The searching should be started automatically after the user did not change
    // the text for a while
    m_startSearchTimer = new QTimer(this);
    m_startSearchTimer->setSingleShot(true);
    m_startSearchTimer->setInterval(500);
    connect(m_startSearchTimer, &QTimer::timeout, this, &Bar::commitCurrentConfiguration);

    updateStateToMatch(dolphinQuery);
}

QString Bar::text() const
{
    return m_searchTermEditor->text();
}

void Bar::setSearchPath(const QUrl &url)
{
    if (url == m_searchConfiguration->searchPath()) {
        return;
    }

    DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
    searchConfigurationCopy.setSearchPath(url);
    updateStateToMatch(std::make_shared<const DolphinQuery>(std::move(searchConfigurationCopy)));
}

void Bar::selectAll()
{
    m_searchTermEditor->setFocus();
    m_searchTermEditor->selectAll();
}

void Bar::setVisible(bool visible, Animated animated)
{
    if (!visible) {
        m_startSearchTimer->stop();
        Q_EMIT urlChangeRequested(m_searchConfiguration->searchPath());
        if (isAncestorOf(QApplication::focusWidget())) {
            Q_EMIT focusViewRequest();
        }
    }
    AnimatedHeightWidget::setVisible(visible, animated);
    Q_EMIT visibilityChanged(visible);
}

void Bar::updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery)
{
    const int cursorPosition = m_searchTermEditor->cursorPosition();
    m_searchTermEditor->setText(dolphinQuery->searchTerm());
    m_searchTermEditor->setCursorPosition(qMin(cursorPosition, dolphinQuery->searchTerm().length()));
    // When the Popup is closed users might not know whether they are searching in file names or contents. This can be problematic when users do not find a
    // file and then assume it doesn't exist. We consider searching for names matching the search term the default and only show a generic "Search…" text as
    // the placeholder then. But when names are not searched we change the placeholder message to make this clear.
    m_searchTermEditor->setPlaceholderText(dolphinQuery->searchTool() == SearchTool::Filenamesearch
                                                   && dolphinQuery->searchThrough() == SearchThrough::FileContents
                                               ? i18nc("@info:placeholder", "Search in file contents…")
                                               : i18n("Search…"));
    m_saveSearchAction->setEnabled(::isSearchConfigured(dolphinQuery));
    m_fromHereButton->setChecked(dolphinQuery->searchLocations() == SearchLocations::FromHere);
    m_everywhereButton->setChecked(dolphinQuery->searchLocations() == SearchLocations::Everywhere);

    if (m_popup && m_popup->isVisible()) {
        // The user actually sees the popup, so update it now! Normally the popup is only updated when Popup::aboutToShow() is emitted.
        m_popup->updateStateToMatch(dolphinQuery);
    }

    /// Update tooltip
    const QUrl cleanedUrl = dolphinQuery->searchPath().adjusted(QUrl::RemoveUserInfo | QUrl::StripTrailingSlash);
    m_fromHereButton->setToolTip(
        xi18nc("@info:tooltip", "Limit the search to <filename>%1</filename> and its subfolders.", cleanedUrl.toString(QUrl::PreferLocalFile)));
    m_everywhereButton->setToolTip(
        dolphinQuery->searchTool() == SearchTool::Filenamesearch
            // clang-format off
            // clang-format is turned off because we need to make sure the i18n call is in a single row or the i18n comment above will not be extracted.
            // See https://commits.kde.org/kxmlgui/a31135046e1b3335b5d7bbbe6aa9a883ce3284c1
            // i18n: The "Everywhere" button makes Dolphin search all files in "/" recursively. "From the root up" is meant to
            // communicate this colloquially while containing the technical term "root". It is fine to drop the technicalities here
            // and only to communicate that everything in the file system is supposed to be searched here.
            ? i18nc("@info:tooltip", "Search all directories from the root up.")
            // i18n: Tooltip for "Everywhere" button as opposed to searching for files in specific folders. The search tool uses
            // file indexing and will therefore only be able to search through directories which have been put into a data base.
            // Please make sure your translation of the path to the Search settings page is identical to translation there.
            : xi18nc("@info:tooltip", "Search all indexed locations.<nl/><nl/>Configure which locations are indexed in <interface>System Settings|Workspace|Search</interface>."));
    // clang-format on

    auto updateChip = [this, &dolphinQuery]<typename Selector>(bool shouldExist, Chip<Selector> *chip) -> Chip<Selector> * {
        if (shouldExist) {
            if (!chip) {
                chip = new Chip<Selector>{dolphinQuery, nullptr};
                chip->hide();
                chip->setMaximumHeight(m_fromHereButton->height());
                connect(chip, &ChipBase::configurationChanged, this, &Bar::slotConfigurationChanged);
                m_secondRowLayout->addWidget(chip); // Transfers ownership
                chip->show(); // Only showing the chip after it was added to the correct layout avoids a bug which shows the chip at the top of the bar.
            } else {
                chip->updateStateToMatch(dolphinQuery);
            }
            return chip;
        }
        if (chip) {
            chip->deleteLater();
        }
        return nullptr;
    };

    m_fileTypeSelectorChip = updateChip.template operator()<FileTypeSelector>(dolphinQuery->searchTool() != SearchTool::Filenamesearch
                                                                                  && dolphinQuery->fileType() != KFileMetaData::Type::Empty,
                                                                              m_fileTypeSelectorChip);
    m_modifiedSinceDateSelectorChip =
        updateChip.template operator()<DateSelector>(dolphinQuery->searchTool() != SearchTool::Filenamesearch && dolphinQuery->modifiedSinceDate().isValid(),
                                                     m_modifiedSinceDateSelectorChip);
    m_minimumRatingSelectorChip =
        updateChip.template operator()<MinimumRatingSelector>(dolphinQuery->searchTool() != SearchTool::Filenamesearch && dolphinQuery->minimumRating() > 0,
                                                              m_minimumRatingSelectorChip);
    m_requiredTagsSelectorChip =
        updateChip.template operator()<TagsSelector>(dolphinQuery->searchTool() != SearchTool::Filenamesearch && dolphinQuery->requiredTags().count(),
                                                     m_requiredTagsSelectorChip);
}

void Bar::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        if (m_searchTermEditor->text().isEmpty()) {
            setVisible(false, WithAnimation);
        } else {
            // Clear the text input
            slotSearchTermEdited(QString());
        }
    }
}

void Bar::keyReleaseEvent(QKeyEvent *event)
{
    QWidget::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Down) {
        Q_EMIT focusViewRequest();
    }
}

void Bar::slotConfigurationChanged(const DolphinQuery &searchConfiguration)
{
    Q_ASSERT_X(*m_searchConfiguration != searchConfiguration, "Bar::updateState()", "Redundantly updating to a state that is identical to the previous state.");
    updateStateToMatch(std::make_shared<const DolphinQuery>(searchConfiguration));

    commitCurrentConfiguration();
}

void Bar::slotSearchTermEdited(const QString &text)
{
    DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
    searchConfigurationCopy.setSearchTerm(text);
    updateStateToMatch(std::make_shared<const DolphinQuery>(searchConfigurationCopy));

    m_startSearchTimer->start();
}

void Bar::slotReturnPressed()
{
    commitCurrentConfiguration();
    Q_EMIT focusViewRequest();
}

void Bar::commitCurrentConfiguration()
{
    m_startSearchTimer->stop();
    // We return early and avoid searching when the user has not given any information we can search for. They might for example have deleted the search term.
    // In that case we want to show the files of the normal location again.
    if (!isSearchConfigured()) {
        Q_EMIT urlChangeRequested(m_searchConfiguration->searchPath());
        return;
    }
    Q_EMIT urlChangeRequested(m_searchConfiguration->toUrl());
}

void Bar::slotSaveSearch()
{
    Q_ASSERT_X(isSearchConfigured(),
               "Search::Bar::slotSaveSearch()",
               "Search::Bar::isSearchConfigured() considers this search invalid, so the user should not be able to save this search. The button to save should "
               "be disabled.");
    const QUrl searchUrl = m_searchConfiguration->toUrl();
    Q_ASSERT(searchUrl.isValid() && isSupportedSearchScheme(searchUrl.scheme()));
    DolphinPlacesModelSingleton::instance().placesModel()->addPlace(m_searchConfiguration->title(), searchUrl, QStringLiteral("folder-saved-search-symbolic"));
}

bool Bar::isSearchConfigured() const
{
    return ::isSearchConfigured(m_searchConfiguration);
}

QString Bar::queryTitle() const
{
    return m_searchConfiguration->title();
}

int Bar::preferredHeight() const
{
    return m_secondRowLayout->geometry().y() + m_secondRowLayout->sizeHint().height() + Dolphin::LAYOUT_SPACING_SMALL;
}
