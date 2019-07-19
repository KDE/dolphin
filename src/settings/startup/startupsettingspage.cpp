/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "startupsettingspage.h"

#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"
#include "dolphinviewcontainer.h"
#include "global.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QButtonGroup>
#include <QCheckBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>

StartupSettingsPage::StartupSettingsPage(const QUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_homeUrl(nullptr),
    m_homeUrlBoxLayoutContainer(nullptr),
    m_buttonBoxLayoutContainer(nullptr),
    m_rememberOpenedTabsRadioButton(nullptr),
    m_homeUrlRadioButton(nullptr),
    m_splitView(nullptr),
    m_editableUrl(nullptr),
    m_showFullPath(nullptr),
    m_filterBar(nullptr),
    m_showFullPathInTitlebar(nullptr),
    m_openExternallyCalledFolderInNewTab(nullptr)
{
    QFormLayout* topLayout = new QFormLayout(this);

    m_rememberOpenedTabsRadioButton = new QRadioButton(i18nc("@option:radio Startup Settings", "Folders, tabs, and window state from last time"));
    m_homeUrlRadioButton = new QRadioButton();
    // HACK: otherwise the radio button has too much spacing in a grid layout
    m_homeUrlRadioButton->setMaximumWidth(24);

    QButtonGroup* initialViewGroup = new QButtonGroup(this);
    initialViewGroup->addButton(m_rememberOpenedTabsRadioButton);
    initialViewGroup->addButton(m_homeUrlRadioButton);


    // create 'Home URL' editor
    m_homeUrlBoxLayoutContainer = new QWidget(this);
    QHBoxLayout* homeUrlBoxLayout = new QHBoxLayout(m_homeUrlBoxLayoutContainer);
    homeUrlBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_homeUrl = new QLineEdit();
    m_homeUrl->setClearButtonEnabled(true);
    homeUrlBoxLayout->addWidget(m_homeUrl);

    QPushButton* selectHomeUrlButton = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), QString());
    homeUrlBoxLayout->addWidget(selectHomeUrlButton);

#ifndef QT_NO_ACCESSIBILITY
    selectHomeUrlButton->setAccessibleName(i18nc("@action:button", "Select Home Location"));
#endif

    connect(selectHomeUrlButton, &QPushButton::clicked,
            this, &StartupSettingsPage::selectHomeUrl);

    m_buttonBoxLayoutContainer = new QWidget(this);
    QHBoxLayout* buttonBoxLayout = new QHBoxLayout(m_buttonBoxLayoutContainer);
    buttonBoxLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton* useCurrentButton = new QPushButton(i18nc("@action:button", "Use Current Location"));
    buttonBoxLayout->addWidget(useCurrentButton);
    connect(useCurrentButton, &QPushButton::clicked,
            this, &StartupSettingsPage::useCurrentLocation);
    QPushButton* useDefaultButton = new QPushButton(i18nc("@action:button", "Use Default Location"));
    buttonBoxLayout->addWidget(useDefaultButton);
    connect(useDefaultButton, &QPushButton::clicked,
            this, &StartupSettingsPage::useDefaultLocation);

    QGridLayout* startInLocationLayout = new QGridLayout();
    startInLocationLayout->setHorizontalSpacing(0);
    startInLocationLayout->setContentsMargins(0, 0, 0, 0);
    startInLocationLayout->addWidget(m_homeUrlRadioButton, 0, 0);
    startInLocationLayout->addWidget(m_homeUrlBoxLayoutContainer, 0, 1);
    startInLocationLayout->addWidget(m_buttonBoxLayoutContainer, 1, 1);

    topLayout->addRow(i18nc("@label:textbox", "Show on startup:"), m_rememberOpenedTabsRadioButton);
    topLayout->addRow(QString(), startInLocationLayout);


    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_splitView = new QCheckBox(i18nc("@option:check Startup Settings", "Begin in split view mode"));
    topLayout->addRow(i18n("New windows:"), m_splitView);
    m_filterBar = new QCheckBox(i18nc("@option:check Startup Settings", "Show filter bar"));
    topLayout->addRow(QString(), m_filterBar);
    m_editableUrl = new QCheckBox(i18nc("@option:check Startup Settings", "Make location bar editable"));
    topLayout->addRow(QString(), m_editableUrl);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_openExternallyCalledFolderInNewTab = new QCheckBox(i18nc("@option:check Startup Settings", "Open new folders in tabs"));
    topLayout->addRow(i18nc("@label:checkbox", "General:"), m_openExternallyCalledFolderInNewTab);
    m_showFullPath = new QCheckBox(i18nc("@option:check Startup Settings", "Show full path inside location bar"));
    topLayout->addRow(QString(), m_showFullPath);
    m_showFullPathInTitlebar = new QCheckBox(i18nc("@option:check Startup Settings", "Show full path in title bar"));
    topLayout->addRow(QString(), m_showFullPathInTitlebar);

    loadSettings();

    updateInitialViewOptions();

    connect(m_homeUrl, &QLineEdit::textChanged, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_rememberOpenedTabsRadioButton, &QRadioButton::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_homeUrlRadioButton, &QRadioButton::toggled, this, &StartupSettingsPage::slotSettingsChanged);

    connect(m_splitView,    &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_editableUrl,  &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_filterBar,    &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);

    connect(m_openExternallyCalledFolderInNewTab, &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_showFullPath, &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_showFullPathInTitlebar, &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
}

StartupSettingsPage::~StartupSettingsPage()
{
}

void StartupSettingsPage::applySettings()
{
    GeneralSettings* settings = GeneralSettings::self();

    const QUrl url(QUrl::fromUserInput(m_homeUrl->text(), QString(), QUrl::AssumeLocalFile));
    KFileItem fileItem(url);
    if ((url.isValid() && fileItem.isDir()) || (url.scheme() == QLatin1String("timeline"))) {
        settings->setHomeUrl(url.toDisplayString(QUrl::PreferLocalFile));
    } else {
        KMessageBox::error(this, i18nc("@info", "The location for the home folder is invalid or does not exist, it will not be applied."));
    }

    // Remove saved state if "remember open tabs" has been turned off
    if (!m_rememberOpenedTabsRadioButton->isChecked()) {
        KConfigGroup windowState{KSharedConfig::openConfig(QStringLiteral("dolphinrc")), "WindowState"};
        if (windowState.exists()) {
            windowState.deleteGroup();
        }
    }

    settings->setRememberOpenedTabs(m_rememberOpenedTabsRadioButton->isChecked());
    settings->setSplitView(m_splitView->isChecked());
    settings->setEditableUrl(m_editableUrl->isChecked());
    settings->setFilterBar(m_filterBar->isChecked());
    settings->setOpenExternallyCalledFolderInNewTab(m_openExternallyCalledFolderInNewTab->isChecked());
    settings->setShowFullPath(m_showFullPath->isChecked());
    settings->setShowFullPathInTitlebar(m_showFullPathInTitlebar->isChecked());
    settings->save();
}

void StartupSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void StartupSettingsPage::slotSettingsChanged()
{
    // Provide a hint that the startup settings have been changed. This allows the views
    // to apply the startup settings only if they have been explicitly changed by the user
    // (see bug #254947).
    GeneralSettings::setModifiedStartupSettings(true);

    // Enable and disable home URL controls appropriately
    updateInitialViewOptions();
    emit changed();
}

void StartupSettingsPage::updateInitialViewOptions()
{
    m_homeUrlBoxLayoutContainer->setEnabled(m_homeUrlRadioButton->isChecked());
    m_buttonBoxLayoutContainer->setEnabled(m_homeUrlRadioButton->isChecked());
}

void StartupSettingsPage::selectHomeUrl()
{
    const QUrl homeUrl(QUrl::fromUserInput(m_homeUrl->text(), QString(), QUrl::AssumeLocalFile));
    QUrl url = QFileDialog::getExistingDirectoryUrl(this, QString(), homeUrl);
    if (!url.isEmpty()) {
        m_homeUrl->setText(url.toDisplayString(QUrl::PreferLocalFile));
        slotSettingsChanged();
    }
}

void StartupSettingsPage::useCurrentLocation()
{
    m_homeUrl->setText(m_url.toDisplayString(QUrl::PreferLocalFile));
}

void StartupSettingsPage::useDefaultLocation()
{
    m_homeUrl->setText(QDir::homePath());
}

void StartupSettingsPage::loadSettings()
{
    const QUrl url(Dolphin::homeUrl());
    m_homeUrl->setText(url.toDisplayString(QUrl::PreferLocalFile));
    m_rememberOpenedTabsRadioButton->setChecked(GeneralSettings::rememberOpenedTabs());
    m_homeUrlRadioButton->setChecked(!GeneralSettings::rememberOpenedTabs());
    m_splitView->setChecked(GeneralSettings::splitView());
    m_editableUrl->setChecked(GeneralSettings::editableUrl());
    m_showFullPath->setChecked(GeneralSettings::showFullPath());
    m_filterBar->setChecked(GeneralSettings::filterBar());
    m_showFullPathInTitlebar->setChecked(GeneralSettings::showFullPathInTitlebar());
    m_openExternallyCalledFolderInNewTab->setChecked(GeneralSettings::openExternallyCalledFolderInNewTab());
}
