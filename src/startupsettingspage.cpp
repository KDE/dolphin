/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "dolphinsettings.h"
#include "dolphinmainwindow.h"
#include "dolphinview.h"
#include "dolphinviewcontainer.h"

#include "dolphin_generalsettings.h"

#include <kdialog.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kvbox.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

StartupSettingsPage::StartupSettingsPage(DolphinMainWindow* mainWin, QWidget* parent) :
    SettingsPageBase(parent),
    m_mainWindow(mainWin),
    m_homeUrl(0),
    m_splitView(0),
    m_editableUrl(0),
    m_filterBar(0)
{
    const int spacing = KDialog::spacingHint();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(spacing);

    // create 'Home URL' editor
    QGroupBox* homeBox = new QGroupBox(i18nc("@title:group", "Home Folder"), vBox);

    KHBox* homeUrlBox = new KHBox(homeBox);
    homeUrlBox->setSpacing(spacing);

    new QLabel(i18nc("@label:textbox", "Location:"), homeUrlBox);
    m_homeUrl = new QLineEdit(homeUrlBox);

    QPushButton* selectHomeUrlButton = new QPushButton(KIcon("folder-open"), QString(), homeUrlBox);
    connect(selectHomeUrlButton, SIGNAL(clicked()),
            this, SLOT(selectHomeUrl()));

    KHBox* buttonBox = new KHBox(homeBox);
    buttonBox->setSpacing(spacing);

    QPushButton* useCurrentButton = new QPushButton(i18nc("@action:button", "Use Current Location"), buttonBox);
    connect(useCurrentButton, SIGNAL(clicked()),
            this, SLOT(useCurrentLocation()));
    QPushButton* useDefaultButton = new QPushButton(i18nc("@action:button", "Use Default Location"), buttonBox);
    connect(useDefaultButton, SIGNAL(clicked()),
            this, SLOT(useDefaultLocation()));

    QVBoxLayout* homeBoxLayout = new QVBoxLayout(homeBox);
    homeBoxLayout->addWidget(homeUrlBox);
    homeBoxLayout->addWidget(buttonBox);

    // create 'Split view', 'Editable location' and 'Filter bar' checkboxes
    m_splitView = new QCheckBox(i18nc("@option:check Startup Settings", "Split view mode"), vBox);
    m_editableUrl = new QCheckBox(i18nc("@option:check Startup Settings", "Editable location bar"), vBox);
    m_filterBar = new QCheckBox(i18nc("@option:check Startup Settings", "Show filter bar"), vBox);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();
}

StartupSettingsPage::~StartupSettingsPage()
{
}

void StartupSettingsPage::applySettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    const KUrl url(m_homeUrl->text());
    KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, url);
    if (url.isValid() && fileItem.isDir()) {
        settings->setHomeUrl(url.prettyUrl());
    } else {
        KMessageBox::error(this, i18n("The location for the home folder is invalid and will not get applied."));
    }

    settings->setSplitView(m_splitView->isChecked());
    settings->setEditableUrl(m_editableUrl->isChecked());
    settings->setFilterBar(m_filterBar->isChecked());
}

void StartupSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setDefaults();
    loadSettings();
}

void StartupSettingsPage::selectHomeUrl()
{
    const QString homeUrl = m_homeUrl->text();
    KUrl url = KFileDialog::getExistingDirectoryUrl(homeUrl);
    if (!url.isEmpty()) {
        m_homeUrl->setText(url.prettyUrl());
    }
}

void StartupSettingsPage::useCurrentLocation()
{
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_homeUrl->setText(view->url().prettyUrl());
}

void StartupSettingsPage::useDefaultLocation()
{
    m_homeUrl->setText(QDir::homePath());
}

void StartupSettingsPage::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_homeUrl->setText(settings->homeUrl());
    m_splitView->setChecked(settings->splitView());
    m_editableUrl->setChecked(settings->editableUrl());
    m_filterBar->setChecked(settings->filterBar());
}

#include "startupsettingspage.moc"
