/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "folderstabssettingspage.h"
#include "dolphinmainwindow.h"
#include "dolphinviewcontainer.h"
#include "global.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KProtocolManager>

#include <QButtonGroup>
#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>

FoldersTabsSettingsPage::FoldersTabsSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_homeUrlBoxLayoutContainer(nullptr)
    , m_buttonBoxLayoutContainer(nullptr)
    , m_homeUrlRadioButton(nullptr)
    , m_url(url)
    , m_homeUrl(nullptr)
    , m_rememberOpenedTabsRadioButton(nullptr)
    , m_openNewTabAfterLastTab(nullptr)
    , m_openNewTabAfterCurrentTab(nullptr)
    , m_splitView(nullptr)
    , m_filterBar(nullptr)
    , m_showFullPathInTitlebar(nullptr)
    , m_openExternallyCalledFolderInNewTab(nullptr)
    , m_useTabForSplitViewSwitch(nullptr)
{
    QFormLayout *topLayout = new QFormLayout(this);

    // Show on startup
    m_rememberOpenedTabsRadioButton = new QRadioButton(i18nc("@option:radio Show on startup", "Folders, tabs, and window state from last time"));
    m_homeUrlRadioButton = new QRadioButton();
    // HACK: otherwise the radio button has too much spacing in a grid layout
    m_homeUrlRadioButton->setMaximumWidth(24);

    QButtonGroup *initialViewGroup = new QButtonGroup(this);
    initialViewGroup->addButton(m_rememberOpenedTabsRadioButton);
    initialViewGroup->addButton(m_homeUrlRadioButton);

    // create 'Home URL' editor
    m_homeUrlBoxLayoutContainer = new QWidget(this);
    QHBoxLayout *homeUrlBoxLayout = new QHBoxLayout(m_homeUrlBoxLayoutContainer);
    homeUrlBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_homeUrl = new QLineEdit();
    m_homeUrl->setClearButtonEnabled(true);
    homeUrlBoxLayout->addWidget(m_homeUrl);

    QPushButton *selectHomeUrlButton = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), QString());
    homeUrlBoxLayout->addWidget(selectHomeUrlButton);

#ifndef QT_NO_ACCESSIBILITY
    selectHomeUrlButton->setAccessibleName(i18nc("@action:button", "Select Home Location"));
#endif

    connect(selectHomeUrlButton, &QPushButton::clicked, this, &FoldersTabsSettingsPage::selectHomeUrl);

    m_buttonBoxLayoutContainer = new QWidget(this);
    QHBoxLayout *buttonBoxLayout = new QHBoxLayout(m_buttonBoxLayoutContainer);
    buttonBoxLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *useCurrentButton = new QPushButton(i18nc("@action:button", "Use Current Location"));
    buttonBoxLayout->addWidget(useCurrentButton);
    connect(useCurrentButton, &QPushButton::clicked, this, &FoldersTabsSettingsPage::useCurrentLocation);
    QPushButton *useDefaultButton = new QPushButton(i18nc("@action:button", "Use Default Location"));
    buttonBoxLayout->addWidget(useDefaultButton);
    connect(useDefaultButton, &QPushButton::clicked, this, &FoldersTabsSettingsPage::useDefaultLocation);

    QGridLayout *startInLocationLayout = new QGridLayout();
    startInLocationLayout->setHorizontalSpacing(0);
    startInLocationLayout->setContentsMargins(0, 0, 0, 0);
    startInLocationLayout->addWidget(m_homeUrlRadioButton, 0, 0);
    startInLocationLayout->addWidget(m_homeUrlBoxLayoutContainer, 0, 1);
    startInLocationLayout->addWidget(m_buttonBoxLayoutContainer, 1, 1);

    topLayout->addRow(i18nc("@label:textbox", "Show on startup:"), m_rememberOpenedTabsRadioButton);
    topLayout->addRow(QString(), startInLocationLayout);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));
    // Opening Folders
    m_openExternallyCalledFolderInNewTab = new QCheckBox(i18nc("@option:check Opening Folders", "Keep a single Dolphin window, opening new folders in tabs"));
    topLayout->addRow(i18nc("@label:checkbox", "Opening Folders:"), m_openExternallyCalledFolderInNewTab);
    // Window
    m_showFullPathInTitlebar = new QCheckBox(i18nc("@option:check Startup Settings", "Show full path in title bar"));
    topLayout->addRow(i18nc("@label:checkbox", "Window:"), m_showFullPathInTitlebar);
    m_filterBar = new QCheckBox(i18nc("@option:check Window Startup Settings", "Show filter bar"));
    topLayout->addRow(QString(), m_filterBar);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Tabs properties
    m_openNewTabAfterCurrentTab = new QRadioButton(i18nc("option:radio", "After current tab"));
    m_openNewTabAfterLastTab = new QRadioButton(i18nc("option:radio", "At end of tab bar"));
    QButtonGroup *tabsBehaviorGroup = new QButtonGroup(this);
    tabsBehaviorGroup->addButton(m_openNewTabAfterCurrentTab);
    tabsBehaviorGroup->addButton(m_openNewTabAfterLastTab);
    topLayout->addRow(i18nc("@title:group", "Open new tabs: "), m_openNewTabAfterCurrentTab);
    topLayout->addRow(QString(), m_openNewTabAfterLastTab);

    // Split Views
    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // 'Switch between panes of split views with tab key'
    m_useTabForSplitViewSwitch = new QCheckBox(i18nc("option:check split view panes", "Switch between views with Tab key"));
    topLayout->addRow(i18nc("@title:group", "Split view: "), m_useTabForSplitViewSwitch);

    // 'Close active pane when turning off split view'
    m_closeActiveSplitView = new QCheckBox(i18nc("option:check", "Turning off split view closes the view in focus"));
    topLayout->addRow(QString(), m_closeActiveSplitView);
    m_closeActiveSplitView->setToolTip(
        i18n("When unchecked, the opposite view will be closed. The Close icon always illustrates which view (left or right) will be closed."));

    // 'Begin in split view mode'
    m_splitView = new QCheckBox(i18nc("@option:check Startup Settings", "Begin in split view mode"));
    topLayout->addRow(i18n("New windows:"), m_splitView);

    loadSettings();

    updateInitialViewOptions();

    connect(m_homeUrl, &QLineEdit::textChanged, this, &FoldersTabsSettingsPage::slotSettingsChanged);
    connect(m_rememberOpenedTabsRadioButton, &QRadioButton::toggled, this, &FoldersTabsSettingsPage::slotSettingsChanged);
    connect(m_homeUrlRadioButton, &QRadioButton::toggled, this, &FoldersTabsSettingsPage::slotSettingsChanged);

    connect(m_splitView, &QCheckBox::toggled, this, &FoldersTabsSettingsPage::slotSettingsChanged);
    connect(m_filterBar, &QCheckBox::toggled, this, &FoldersTabsSettingsPage::slotSettingsChanged);

    connect(m_openExternallyCalledFolderInNewTab, &QCheckBox::toggled, this, &FoldersTabsSettingsPage::slotSettingsChanged);
    connect(m_showFullPathInTitlebar, &QCheckBox::toggled, this, &FoldersTabsSettingsPage::slotSettingsChanged);

    connect(m_useTabForSplitViewSwitch, &QCheckBox::toggled, this, &FoldersTabsSettingsPage::changed);
    connect(m_closeActiveSplitView, &QCheckBox::toggled, this, &FoldersTabsSettingsPage::changed);

    connect(m_openNewTabAfterCurrentTab, &QRadioButton::toggled, this, &FoldersTabsSettingsPage::changed);
    connect(m_openNewTabAfterLastTab, &QRadioButton::toggled, this, &FoldersTabsSettingsPage::changed);
}

FoldersTabsSettingsPage::~FoldersTabsSettingsPage()
{
}

void FoldersTabsSettingsPage::applySettings()
{
    GeneralSettings *settings = GeneralSettings::self();

    settings->setUseTabForSwitchingSplitView(m_useTabForSplitViewSwitch->isChecked());
    settings->setCloseActiveSplitView(m_closeActiveSplitView->isChecked());
    const QUrl url(QUrl::fromUserInput(m_homeUrl->text(), QString(), QUrl::AssumeLocalFile));
    if (url.isValid() && KProtocolManager::supportsListing(url)) {
        KIO::StatJob *job = KIO::stat(url, KIO::StatJob::SourceSide, KIO::StatDetail::StatBasic, KIO::JobFlag::HideProgressInfo);
        connect(job, &KJob::result, this, [this, settings, url](KJob *job) {
            if (job->error() == 0 && qobject_cast<KIO::StatJob *>(job)->statResult().isDir()) {
                settings->setHomeUrl(url.toDisplayString(QUrl::PreferLocalFile));
            } else {
                showSetDefaultDirectoryError();
            }
        });
    } else {
        showSetDefaultDirectoryError();
    }

    // Remove saved state if "remember open tabs" has been turned off
    if (!m_rememberOpenedTabsRadioButton->isChecked()) {
        KConfigGroup windowState{KSharedConfig::openConfig(QStringLiteral("dolphinrc")), QStringLiteral("WindowState")};
        if (windowState.exists()) {
            windowState.deleteGroup();
        }
    }

    settings->setRememberOpenedTabs(m_rememberOpenedTabsRadioButton->isChecked());
    settings->setSplitView(m_splitView->isChecked());
    settings->setFilterBar(m_filterBar->isChecked());
    settings->setOpenExternallyCalledFolderInNewTab(m_openExternallyCalledFolderInNewTab->isChecked());
    settings->setShowFullPathInTitlebar(m_showFullPathInTitlebar->isChecked());

    settings->setOpenNewTabAfterLastTab(m_openNewTabAfterLastTab->isChecked());

    settings->save();
}

void FoldersTabsSettingsPage::restoreDefaults()
{
    GeneralSettings *settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void FoldersTabsSettingsPage::slotSettingsChanged()
{
    // Provide a hint that the startup settings have been changed. This allows the views
    // to apply the startup settings only if they have been explicitly changed by the user
    // (see bug #254947).
    GeneralSettings::setModifiedStartupSettings(true);

    // Enable and disable home URL controls appropriately
    updateInitialViewOptions();
    Q_EMIT changed();
}

void FoldersTabsSettingsPage::updateInitialViewOptions()
{
    m_homeUrlBoxLayoutContainer->setEnabled(m_homeUrlRadioButton->isChecked());
    m_buttonBoxLayoutContainer->setEnabled(m_homeUrlRadioButton->isChecked());
}

void FoldersTabsSettingsPage::selectHomeUrl()
{
    const QUrl homeUrl(QUrl::fromUserInput(m_homeUrl->text(), QString(), QUrl::AssumeLocalFile));
    QUrl url = QFileDialog::getExistingDirectoryUrl(this, QString(), homeUrl);
    if (!url.isEmpty()) {
        m_homeUrl->setText(url.toDisplayString(QUrl::PreferLocalFile));
        slotSettingsChanged();
    }
}

void FoldersTabsSettingsPage::useCurrentLocation()
{
    m_homeUrl->setText(m_url.toDisplayString(QUrl::PreferLocalFile));
}

void FoldersTabsSettingsPage::useDefaultLocation()
{
    m_homeUrl->setText(QDir::homePath());
}

void FoldersTabsSettingsPage::loadSettings()
{
    const QUrl url(Dolphin::homeUrl());
    m_homeUrl->setText(url.toDisplayString(QUrl::PreferLocalFile));
    m_rememberOpenedTabsRadioButton->setChecked(GeneralSettings::rememberOpenedTabs());
    m_homeUrlRadioButton->setChecked(!GeneralSettings::rememberOpenedTabs());
    m_splitView->setChecked(GeneralSettings::splitView());
    m_filterBar->setChecked(GeneralSettings::filterBar());
    m_showFullPathInTitlebar->setChecked(GeneralSettings::showFullPathInTitlebar());
    m_openExternallyCalledFolderInNewTab->setChecked(GeneralSettings::openExternallyCalledFolderInNewTab());

    m_useTabForSplitViewSwitch->setChecked(GeneralSettings::useTabForSwitchingSplitView());
    m_closeActiveSplitView->setChecked(GeneralSettings::closeActiveSplitView());

    m_openNewTabAfterLastTab->setChecked(GeneralSettings::openNewTabAfterLastTab());
    m_openNewTabAfterCurrentTab->setChecked(!m_openNewTabAfterLastTab->isChecked());
}

void FoldersTabsSettingsPage::showSetDefaultDirectoryError()
{
    KMessageBox::error(this, i18nc("@info", "The location for the home folder is invalid or does not exist, it will not be applied."));
}

#include "moc_folderstabssettingspage.cpp"
