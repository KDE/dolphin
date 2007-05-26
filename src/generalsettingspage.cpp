/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
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

#include "generalsettingspage.h"

#include <kdialog.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kvbox.h>

#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>

#include "dolphinsettings.h"
#include "dolphinmainwindow.h"
#include "dolphinview.h"

#include "dolphin_generalsettings.h"

GeneralSettingsPage::GeneralSettingsPage(DolphinMainWindow* mainWin, QWidget* parent) :
    SettingsPageBase(parent),
    m_mainWindow(mainWin),
    m_homeUrl(0),
    m_splitView(0),
    m_editableUrl(0),
    m_filterBar(0),
    m_showDeleteCommand(0),
    m_confirmMoveToTrash(0),
    m_confirmDelete(0)
{
    const int spacing = KDialog::spacingHint();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(spacing);

    // create 'Home URL' editor
    QGroupBox* homeBox = new QGroupBox(i18n("Home Folder"), vBox);

    KHBox* homeUrlBox = new KHBox(homeBox);
    homeUrlBox->setSpacing(spacing);

    new QLabel(i18n("Location:"), homeUrlBox);
    m_homeUrl = new QLineEdit(homeUrlBox);

    QPushButton* selectHomeUrlButton = new QPushButton(KIcon("folder-open"), QString(), homeUrlBox);
    connect(selectHomeUrlButton, SIGNAL(clicked()),
            this, SLOT(selectHomeUrl()));

    KHBox* buttonBox = new KHBox(homeBox);
    buttonBox->setSpacing(spacing);

    QPushButton* useCurrentButton = new QPushButton(i18n("Use Current Location"), buttonBox);
    connect(useCurrentButton, SIGNAL(clicked()),
            this, SLOT(useCurrentLocation()));
    QPushButton* useDefaultButton = new QPushButton(i18n("Use Default Location"), buttonBox);
    connect(useDefaultButton, SIGNAL(clicked()),
            this, SLOT(useDefaultLocation()));

    QVBoxLayout* homeBoxLayout = new QVBoxLayout(homeBox);
    homeBoxLayout->addWidget(homeUrlBox);
    homeBoxLayout->addWidget(buttonBox);

    QGroupBox* startBox = new QGroupBox(i18n("Start"), vBox);

    // create 'Split view', 'Editable location' and 'Filter bar' checkboxes
    m_splitView = new QCheckBox(i18n("Split view"), startBox);
    m_editableUrl = new QCheckBox(i18n("Editable location"), startBox);
    m_filterBar = new QCheckBox(i18n("Filter bar"),startBox);

    QVBoxLayout* startBoxLayout = new QVBoxLayout(startBox);
    startBoxLayout->addWidget(m_splitView);
    startBoxLayout->addWidget(m_editableUrl);
    startBoxLayout->addWidget(m_filterBar);

    // create 'Ask Confirmation For' group
    KSharedConfig::Ptr konqConfig = KSharedConfig::openConfig("konquerorrc", KConfig::IncludeGlobals);
    const KConfigGroup trashConfig(konqConfig, "Trash");

    QGroupBox* confirmBox = new QGroupBox(i18n("Ask Confirmation For"), vBox);

    m_confirmMoveToTrash = new QCheckBox(i18n("Move to trash"), confirmBox);
    m_confirmMoveToTrash->setChecked(trashConfig.readEntry("ConfirmTrash", false));

    m_confirmDelete = new QCheckBox(i18n("Delete"), confirmBox);
    m_confirmDelete->setChecked(trashConfig.readEntry("ConfirmDelete", true));

    QVBoxLayout* confirmBoxLayout = new QVBoxLayout(confirmBox);
    confirmBoxLayout->addWidget(m_confirmMoveToTrash);
    confirmBoxLayout->addWidget(m_confirmDelete);

    // create 'Show the command 'Delete' in context menu' checkbox
    m_showDeleteCommand = new QCheckBox(i18n("Show the command 'Delete' in context menu"), vBox);
    const KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
    const KConfigGroup kdeConfig(globalConfig, "KDE");
    m_showDeleteCommand->setChecked(kdeConfig.readEntry("ShowDeleteCommand", false));

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();
}

GeneralSettingsPage::~GeneralSettingsPage()
{
}

void GeneralSettingsPage::applySettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    const KUrl url(m_homeUrl->text());
    KFileItem fileItem(S_IFDIR, KFileItem::Unknown, url);
    if (url.isValid() && fileItem.isDir()) {
        settings->setHomeUrl(url.prettyUrl());
    }

    settings->setSplitView(m_splitView->isChecked());
    settings->setEditableUrl(m_editableUrl->isChecked());
    settings->setFilterBar(m_filterBar->isChecked());

    KSharedConfig::Ptr konqConfig = KSharedConfig::openConfig("konquerorrc", KConfig::IncludeGlobals);
    KConfigGroup trashConfig(konqConfig, "Trash");
    trashConfig.writeEntry("ConfirmTrash", m_confirmMoveToTrash->isChecked());
    trashConfig.writeEntry("ConfirmDelete", m_confirmDelete->isChecked());

    KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
    KConfigGroup kdeConfig(globalConfig, "KDE");
    kdeConfig.writeEntry("ShowDeleteCommand", m_showDeleteCommand->isChecked());
    kdeConfig.sync();
}

void GeneralSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setDefaults();

    // TODO: reset default settings for trash and show delete command...
    //KSharedConfig::Ptr konqConfig = KSharedConfig::openConfig("konquerorrc", KConfig::IncludeGlobals);
    //KConfigGroup trashConfig(konqConfig, "Trash");
    //KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
    //KConfigGroup kdeConfig(globalConfig, "KDE");

    loadSettings();
}

void GeneralSettingsPage::selectHomeUrl()
{
    const QString homeUrl(m_homeUrl->text());
    KUrl url(KFileDialog::getExistingDirectoryUrl(homeUrl));
    if (!url.isEmpty()) {
        m_homeUrl->setText(url.prettyUrl());
    }
}

void GeneralSettingsPage::useCurrentLocation()
{
    const DolphinView* view = m_mainWindow->activeView();
    m_homeUrl->setText(view->url().prettyUrl());
}

void GeneralSettingsPage::useDefaultLocation()
{
    m_homeUrl->setText("file://" + QDir::homePath());
}

void GeneralSettingsPage::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_homeUrl->setText(settings->homeUrl());
    m_splitView->setChecked(settings->splitView());
    m_editableUrl->setChecked(settings->editableUrl());
    m_filterBar->setChecked(settings->filterBar());
}

#include "generalsettingspage.moc"
