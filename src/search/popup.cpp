/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "popup.h"

#include "config-dolphin.h"
#include "dolphinpackageinstaller.h"
#include "dolphinquery.h"
#include "global.h"
#include "selectors/dateselector.h"
#include "selectors/filetypeselector.h"
#include "selectors/minimumratingselector.h"
#include "selectors/tagsselector.h"

#include <KContextualHelpButton>
#include <KDialogJobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KService>

#include <QButtonGroup>
#ifdef Q_OS_WIN
#include <QDesktopServices>
#endif
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QRadioButton>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
constexpr auto kFindDesktopName = "org.kde.kfind";
}

using namespace Search;

QString Search::filenamesearchUiName()
{
    // i18n: Localized name for the Filenamesearch search tool for use in user interfaces.
    return i18n("Simple search");
};

QString Search::balooUiName()
{
    // i18n: Localized name for the Baloo search tool for use in user interfaces.
    return i18n("File Indexing");
};

Popup::Popup(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent)
    : WidgetMenu{parent}
    , UpdatableStateInterface{dolphinQuery}
{
}

QWidget *Popup::init()
{
    auto containerWidget = new QWidget{this};
    containerWidget->setContentsMargins(Dolphin::VERTICAL_SPACER_HEIGHT,
                                        Dolphin::VERTICAL_SPACER_HEIGHT,
                                        Dolphin::VERTICAL_SPACER_HEIGHT, // Using the same value for every spacing in this containerWidget looks nice.
                                        Dolphin::VERTICAL_SPACER_HEIGHT);
    auto verticalMainLayout = new QVBoxLayout{containerWidget};
    verticalMainLayout->setSpacing((2 * Dolphin::VERTICAL_SPACER_HEIGHT) / 3); // A bit less spacing between rows than when adding an explicit spacer.

    /// Add UI to switch between only searching in file names or also in contents.
    auto searchInLabel = new QLabel{i18nc("@title:group", "Search in:"), containerWidget};
    searchInLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    verticalMainLayout->addWidget(searchInLabel);

    m_searchInFileNamesRadioButton = new QRadioButton{i18nc("@option:radio Search in:", "File names"), containerWidget};
    connect(m_searchInFileNamesRadioButton, &QAbstractButton::clicked, this, [this]() {
        if (m_searchConfiguration->searchThrough() == SearchThrough::FileNames) {
            return; // Already selected.
        }
        SearchSettings::setWhat(QStringLiteral("FileNames"));
        SearchSettings::self()->save();
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setSearchThrough(SearchThrough::FileNames);
        Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
    });
    verticalMainLayout->addWidget(m_searchInFileNamesRadioButton);

    m_searchInFileContentsRadioButton = new QRadioButton{containerWidget};
    connect(m_searchInFileContentsRadioButton, &QAbstractButton::clicked, this, [this]() {
        if (m_searchConfiguration->searchThrough() == SearchThrough::FileContents) {
            return; // Already selected.
        }
        SearchSettings::setWhat(QStringLiteral("FileContents"));
        SearchSettings::self()->save();
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setSearchThrough(SearchThrough::FileContents);
        Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
    });
    verticalMainLayout->addWidget(m_searchInFileContentsRadioButton);

    auto searchInButtonGroup = new QButtonGroup{this};
    searchInButtonGroup->addButton(m_searchInFileNamesRadioButton);
    searchInButtonGroup->addButton(m_searchInFileContentsRadioButton);

    /// Add UI to switch between search tools.
    // When we build without Baloo, there is only one search tool available, so we skip adding the UI to switch.
#if HAVE_BALOO
    verticalMainLayout->addSpacing(Dolphin::VERTICAL_SPACER_HEIGHT);

    auto searchUsingLabel = new QLabel{i18nc("@title:group", "Search using:"), containerWidget};
    searchUsingLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    verticalMainLayout->addWidget(searchUsingLabel);

    /// Initialize the Filenamesearch row.
    m_filenamesearchRadioButton = new QRadioButton{filenamesearchUiName(), containerWidget};
    connect(m_filenamesearchRadioButton, &QAbstractButton::clicked, this, [this]() {
        if (m_searchConfiguration->searchTool() == SearchTool::Filenamesearch) {
            return; // Already selected.
        }
        SearchSettings::setSearchTool(QStringLiteral("Filenamesearch"));
        SearchSettings::self()->save();
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setSearchTool(SearchTool::Filenamesearch);
        Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
    });

    m_filenamesearchContextualHelpButton = new KContextualHelpButton(
        xi18nc("@info about a search tool",
               "<para>For searching in file contents <application>%1</application> attempts to use third-party search tools if they are available on this "
               "system and are expected to lead to better or faster results. <application>ripgrep</application> and <application>ripgrep-all</application> "
               "might improve your search experience if they are installed. <application>ripgrep-all</application> in particular enables searches in more "
               "file types (e.g. pdf, docx, sqlite, jpg, movie subtitles (mkv, mp4)).</para><para>The manner in which these search tools are invoked can be "
               "configured by editing a script file. Copy it from <filename>%2</filename> to <filename>%3</filename> before modifying your copy. If any "
               "issues arise, delete your copy <filename>%3</filename> to revert your changes.</para>",
               filenamesearchUiName(),
               QStringLiteral("%1/kio_filenamesearch/kio-filenamesearch-grep").arg(KDE_INSTALL_FULL_DATADIR),
               QStringLiteral("%1/kio_filenamesearch/kio-filenamesearch-grep").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))),
        m_filenamesearchRadioButton,
        containerWidget);

    auto filenamesearchRowLayout = new QHBoxLayout;
    filenamesearchRowLayout->addWidget(m_filenamesearchRadioButton);
    filenamesearchRowLayout->addWidget(m_filenamesearchContextualHelpButton);
    filenamesearchRowLayout->addStretch(); // for left-alignment
    verticalMainLayout->addLayout(filenamesearchRowLayout);

    /// Initialize the Baloo row.
    m_balooRadioButton = new QRadioButton{balooUiName(), containerWidget};
    connect(m_balooRadioButton, &QAbstractButton::clicked, this, [this]() {
        if (m_searchConfiguration->searchTool() == SearchTool::Baloo) {
            return; // Already selected.
        }
        SearchSettings::setSearchTool(QStringLiteral("Baloo"));
        SearchSettings::self()->save();
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setSearchTool(SearchTool::Baloo);
        Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
    });

    m_balooContextualHelpButton = new KContextualHelpButton(QString(), m_balooRadioButton, containerWidget);

    auto balooSettingsButton = new QToolButton{containerWidget};
    balooSettingsButton->setText(i18nc("@action:button %1 is software name", "Configure %1…", balooUiName()));
    balooSettingsButton->setIcon(QIcon::fromTheme("configure"));
    balooSettingsButton->setToolTip(balooSettingsButton->text());
    balooSettingsButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    balooSettingsButton->setAutoRaise(true);
    balooSettingsButton->setFixedHeight(m_balooRadioButton->sizeHint().height());
    connect(balooSettingsButton, &QToolButton::clicked, this, [containerWidget] {
        // Code taken from KCMLauncher::openSystemSettings() in the KCMUtil KDE framework.
        constexpr auto systemSettings = "systemsettings";
        KIO::CommandLauncherJob *openBalooSettingsJob;
        // Open in System Settings if it's available
        if (KService::serviceByDesktopName(systemSettings)) {
            openBalooSettingsJob = new KIO::CommandLauncherJob(systemSettings, {"kcm_baloofile"}, containerWidget);
            openBalooSettingsJob->setDesktopName(systemSettings);
        } else {
            openBalooSettingsJob = new KIO::CommandLauncherJob(QStringLiteral("kcmshell6"), {"kcm_baloofile"}, containerWidget);
        }
        openBalooSettingsJob->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, containerWidget));
        openBalooSettingsJob->start();
    });

    auto balooRowLayout = new QHBoxLayout;
    balooRowLayout->addWidget(m_balooRadioButton);
    balooRowLayout->addWidget(m_balooContextualHelpButton);
    balooRowLayout->addWidget(balooSettingsButton);
    balooRowLayout->addStretch(); // for left-alignment
    verticalMainLayout->addLayout(balooRowLayout);

    auto searchUsingButtonGroup = new QButtonGroup{this};
    searchUsingButtonGroup->addButton(m_filenamesearchRadioButton);
    searchUsingButtonGroup->addButton(m_balooRadioButton);

    verticalMainLayout->addSpacing(Dolphin::VERTICAL_SPACER_HEIGHT);

    /// Add extra search filters like date, tags, rating, etc.
    m_selectorsLayoutWidget = new QWidget{containerWidget};
    if (m_searchConfiguration->searchTool() == SearchTool::Filenamesearch) {
        m_selectorsLayoutWidget->hide();
    }
    auto selectorsLayout = new QGridLayout{m_selectorsLayoutWidget};
    selectorsLayout->setContentsMargins(0, 0, 0, 0);
    selectorsLayout->setSpacing(verticalMainLayout->spacing());

    auto typeSelectorTitle = new QLabel{i18nc("@title:group for filtering files based on their type", "File Type:"), containerWidget};
    typeSelectorTitle->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    selectorsLayout->addWidget(typeSelectorTitle, 1, 0);

    m_typeSelector = new FileTypeSelector{m_searchConfiguration, containerWidget};
    connect(m_typeSelector, &FileTypeSelector::configurationChanged, this, &Popup::configurationChanged);
    selectorsLayout->addWidget(m_typeSelector, 2, 0);

    auto dateSelectorTitle = new QLabel{i18nc("@title:group for filtering files by modified date", "Modified since:"), containerWidget};
    dateSelectorTitle->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    selectorsLayout->addWidget(dateSelectorTitle, 1, 1);

    m_dateSelector = new DateSelector{m_searchConfiguration, containerWidget};
    m_dateSelector->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed); // Make sure this button is as wide as the other button in this column.
    connect(m_dateSelector, &DateSelector::configurationChanged, this, &Popup::configurationChanged);
    selectorsLayout->addWidget(m_dateSelector, 2, 1);

    auto ratingSelectorTitle = new QLabel{i18nc("@title:group for selecting a minimum rating of search results", "Rating:"), containerWidget};
    ratingSelectorTitle->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    selectorsLayout->addWidget(ratingSelectorTitle, 3, 0);

    m_ratingSelector = new MinimumRatingSelector{m_searchConfiguration, containerWidget};
    connect(m_ratingSelector, &MinimumRatingSelector::configurationChanged, this, &Popup::configurationChanged);
    selectorsLayout->addWidget(m_ratingSelector, 4, 0);

    auto tagsSelectorTitle = new QLabel{i18nc("@title:group for selecting required tags for search results", "Tags:"), containerWidget};
    tagsSelectorTitle->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    selectorsLayout->addWidget(tagsSelectorTitle, 3, 1);

    m_tagsSelector = new TagsSelector{m_searchConfiguration, containerWidget};
    m_tagsSelector->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed); // Make sure this button is as wide as the other button in this column.
    connect(m_tagsSelector, &TagsSelector::configurationChanged, this, &Popup::configurationChanged);
    selectorsLayout->addWidget(m_tagsSelector, 4, 1);

    verticalMainLayout->addWidget(m_selectorsLayoutWidget);
#endif // HAVE_BALOO

    /**
     * Dolphin cannot provide every advanced search workflow, so here at the end we need to push users to more dedicated search tools if what Dolphin provides
     * turns out to be insufficient.
     */
    verticalMainLayout->addSpacing(Dolphin::VERTICAL_SPACER_HEIGHT);

    auto kfindLabel = new QLabel{i18nc("@label above 'Install KFind'/'Open KFind' button", "For more advanced searches:"), containerWidget};
    kfindLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
    verticalMainLayout->addWidget(kfindLabel);

    m_kFindButton = new QToolButton{containerWidget};
    m_kFindButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_kFindButton, &QToolButton::clicked, this, &Popup::slotKFindButtonClicked);
    verticalMainLayout->addWidget(m_kFindButton);

    return containerWidget;
}

void Popup::updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery)
{
    m_searchInFileNamesRadioButton->setChecked(dolphinQuery->searchThrough() == SearchThrough::FileNames);
    m_searchInFileContentsRadioButton->setChecked(dolphinQuery->searchThrough() == SearchThrough::FileContents);

    // When we build without Baloo, there is only one search tool available and no UI to switch.
#if HAVE_BALOO
    m_filenamesearchRadioButton->setChecked(dolphinQuery->searchTool() == SearchTool::Filenamesearch);
    m_filenamesearchContextualHelpButton->setVisible(dolphinQuery->searchThrough() == SearchThrough::FileContents);

    if (dolphinQuery->searchLocations() != SearchLocations::Everywhere && !isIndexingEnabledIn(dolphinQuery->searchPath())) {
        m_balooRadioButton->setToolTip(
            xi18nc("@info:tooltip",
                   "<para>Searching in <filename>%1</filename> using <application>%2</application> is currently not possible because "
                   "<application>%2</application> is configured to never create a search index of that location.</para>",
                   dolphinQuery->searchPath().adjusted(QUrl::RemoveUserInfo | QUrl::StripTrailingSlash).toString(QUrl::PreferLocalFile),
                   balooUiName()));
        m_balooRadioButton->setDisabled(true);
    } else if (dolphinQuery->searchThrough() == SearchThrough::FileContents && !isContentIndexingEnabled()) {
        m_balooRadioButton->setToolTip(xi18nc("@info:tooltip",
                                              "<para>Searching through file contents using <application>%1</application> is currently not possible because "
                                              "<application>%1</application> is configured to never create a search index for file contents.</para>",
                                              balooUiName()));
        m_balooRadioButton->setDisabled(true);
    } else {
        m_balooRadioButton->setToolTip(QString());
        m_balooRadioButton->setEnabled(true);
    }
    m_balooContextualHelpButton->setContextualHelpText(
        i18nc("@info make a warning paragraph bold before other paragraphs", "<b>%1</b>", m_balooRadioButton->toolTip())
        + xi18nc(
            "@info about a search tool",
            "<para><application>%1</application> uses a database for searching. The database is created by indexing your files in the background based on "
            "how <application>%1</application> is configured.<list><item><application>%1</application> provides results extremely "
            "quickly.</item><item>Allows searching for file types, dates, tags, etc.</item><item>Only searches in indexed folders. Configure which folders "
            "should be indexed in <application>System Settings</application>.</item><item>When the searched locations contain links to other files or "
            "folders, those will not be searched or show up in search results.</item><item>Hidden files and folders and their contents might also not be "
            "searched depending on how <application>%1</application> is configured.</item></list></para>",
            balooUiName()));

    m_balooRadioButton->setChecked(dolphinQuery->searchTool() == SearchTool::Baloo);
    m_balooRadioButton->setChecked(false);

    if (m_balooRadioButton->isChecked()) {
        m_searchInFileContentsRadioButton->setText(i18nc("@option:radio Search in:", "File names and contents"));
        m_typeSelector->updateStateToMatch(dolphinQuery);
        m_dateSelector->updateStateToMatch(dolphinQuery);
        m_ratingSelector->updateStateToMatch(dolphinQuery);
        m_tagsSelector->updateStateToMatch(dolphinQuery);
    } else {
#endif // HAVE_BALOO
        m_searchInFileContentsRadioButton->setText(i18nc("@option:radio Search in:", "File contents"));
#if HAVE_BALOO
    }

    /// Show/Hide Baloo-specific selectors.
    m_selectorsLayoutWidget->setVisible(m_balooRadioButton->isChecked());
    const int columnWidth = std::max(
        {m_typeSelector->sizeHint().width(), m_dateSelector->sizeHint().width(), m_ratingSelector->sizeHint().width(), m_tagsSelector->sizeHint().width()});
    static_cast<QGridLayout *>(m_selectorsLayoutWidget->layout())->setColumnMinimumWidth(0, columnWidth);
    static_cast<QGridLayout *>(m_selectorsLayoutWidget->layout())->setColumnMinimumWidth(1, columnWidth);
    resizeToFitContents();
#endif // HAVE_BALOO

    KService::Ptr kFind = KService::serviceByDesktopName(kFindDesktopName);
    if (kFind) {
        m_kFindButton->setText(i18nc("@action:button 1 is KFind app name", "Open %1", kFind->name()));
        m_kFindButton->setIcon(QIcon::fromTheme(kFind->icon()));
    } else {
        m_kFindButton->setText(i18nc("@action:button", "Install KFind…"));
        m_kFindButton->setIcon(QIcon::fromTheme(QStringLiteral("kfind"), QIcon::fromTheme(QStringLiteral("install"))));
    }
}

void Popup::slotKFindButtonClicked()
{
    /// Open KFind if it is installed.
    KService::Ptr kFind = KService::serviceByDesktopName(kFindDesktopName);
    if (kFind) {
        auto *job = new KIO::ApplicationLauncherJob(kFind);
        job->setUrls({m_searchConfiguration->searchPath()});
        job->start();
        return;
    }

    /// Otherwise, install KFind.
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl("https://apps.kde.org/kfind"));
#else
    auto packageInstaller = new DolphinPackageInstaller(
        KFIND_PACKAGE_NAME,
        QUrl("appstream://org.kde.kfind.desktop"),
        []() {
            return KService::serviceByDesktopName(kFindDesktopName);
        },
        this);
    connect(packageInstaller, &KJob::result, this, [this](KJob *job) {
        Q_EMIT showInstallationProgress(QString(), 100); // Hides the progress information in the status bar.
        if (job->error()) {
            Q_EMIT showMessage(job->errorString(), KMessageWidget::Error);
        } else {
            Q_EMIT showMessage(xi18nc("@info", "<application>KFind</application> installed successfully."), KMessageWidget::Positive);
            updateStateToMatch(m_searchConfiguration); // Updates m_kfindButton from an "Install KFind…" to an "Open KFind" button.
        }
    });
    const auto installationTaskText{i18nc("@info:status", "Installing KFind")};
    Q_EMIT showInstallationProgress(installationTaskText, -1);
    connect(packageInstaller, &KJob::percentChanged, this, [this, installationTaskText](KJob * /* job */, long unsigned int percent) {
        if (percent < 100) { // Ignore some weird reported values.
            Q_EMIT showInstallationProgress(installationTaskText, percent);
        }
    });
    packageInstaller->start();
#endif
}
