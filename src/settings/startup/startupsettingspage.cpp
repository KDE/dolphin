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

#include <QCheckBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

StartupSettingsPage::StartupSettingsPage(const QUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_homeUrl(nullptr),
    m_splitView(nullptr),
    m_editableUrl(nullptr),
    m_showFullPath(nullptr),
    m_filterBar(nullptr),
    m_showFullPathInTitlebar(nullptr)
{
    QFormLayout* topLayout = new QFormLayout(this);


    // create 'Home URL' editor
    QHBoxLayout* homeUrlBoxLayout = new QHBoxLayout();
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

    QHBoxLayout* buttonBoxLayout = new QHBoxLayout();
    buttonBoxLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton* useCurrentButton = new QPushButton(i18nc("@action:button", "Use Current Location"));
    buttonBoxLayout->addWidget(useCurrentButton);
    connect(useCurrentButton, &QPushButton::clicked,
            this, &StartupSettingsPage::useCurrentLocation);
    QPushButton* useDefaultButton = new QPushButton(i18nc("@action:button", "Use Default Location"));
    buttonBoxLayout->addWidget(useDefaultButton);
    connect(useDefaultButton, &QPushButton::clicked,
            this, &StartupSettingsPage::useDefaultLocation);

    QVBoxLayout* homeBoxLayout = new QVBoxLayout();
    homeBoxLayout->setContentsMargins(0, 0, 0, 0);
    homeBoxLayout->addLayout(homeUrlBoxLayout);
    homeBoxLayout->addLayout(buttonBoxLayout);

    topLayout->addRow(i18nc("@label:textbox", "Start in:"), homeBoxLayout);


    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));


    // create 'Split view', 'Show full path', 'Editable location' and 'Filter bar' checkboxes
    m_splitView = new QCheckBox(i18nc("@option:check Startup Settings", "Split view mode"));
    topLayout->addRow(i18nc("@label:checkbox", "Window options:"), m_splitView);
    m_editableUrl = new QCheckBox(i18nc("@option:check Startup Settings", "Editable location bar"));
    topLayout->addRow(QString(), m_editableUrl);
    m_showFullPath = new QCheckBox(i18nc("@option:check Startup Settings", "Show full path inside location bar"));
    topLayout->addRow(QString(), m_showFullPath);
    m_filterBar = new QCheckBox(i18nc("@option:check Startup Settings", "Show filter bar"));
    topLayout->addRow(QString(), m_filterBar);
    m_showFullPathInTitlebar = new QCheckBox(i18nc("@option:check Startup Settings", "Show full path in title bar"));
    topLayout->addRow(QString(), m_showFullPathInTitlebar);


    loadSettings();

    connect(m_homeUrl, &QLineEdit::textChanged, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_splitView,    &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_editableUrl,  &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_showFullPath, &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_filterBar,    &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
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

    settings->setSplitView(m_splitView->isChecked());
    settings->setEditableUrl(m_editableUrl->isChecked());
    settings->setShowFullPath(m_showFullPath->isChecked());
    settings->setFilterBar(m_filterBar->isChecked());
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
    emit changed();
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
    m_splitView->setChecked(GeneralSettings::splitView());
    m_editableUrl->setChecked(GeneralSettings::editableUrl());
    m_showFullPath->setChecked(GeneralSettings::showFullPath());
    m_filterBar->setChecked(GeneralSettings::filterBar());
    m_showFullPathInTitlebar->setChecked(GeneralSettings::showFullPathInTitlebar());
}
