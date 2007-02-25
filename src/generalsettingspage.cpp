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

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "dolphinsettings.h"
#include "dolphinmainwindow.h"
#include "dolphinview.h"

#include "dolphin_generalsettings.h"

GeneralSettingsPage::GeneralSettingsPage(DolphinMainWindow* mainWin,QWidget* parent) :
    SettingsPageBase(parent),
    m_mainWindow(mainWin),
    m_homeUrl(0),
    m_startSplit(0),
    m_startEditable(0),
    m_showDeleteCommand(0)
{
    const int spacing = KDialog::spacingHint();
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(spacing);

    // create 'Home URL' editor
    QGroupBox* homeBox = new QGroupBox(i18n("Home folder"), vBox);

    KHBox* homeUrlBox = new KHBox(homeBox);
    homeUrlBox->setSpacing(spacing);

    new QLabel(i18n("Location:"), homeUrlBox);
    m_homeUrl = new QLineEdit(settings->homeUrl(), homeUrlBox);

    QPushButton* selectHomeUrlButton = new QPushButton(SmallIcon("folder"), QString(), homeUrlBox);
    connect(selectHomeUrlButton, SIGNAL(clicked()),
            this, SLOT(selectHomeUrl()));

    KHBox* buttonBox = new KHBox(homeBox);
    buttonBox->setSpacing(spacing);

    QPushButton* useCurrentButton = new QPushButton(i18n("Use current location"), buttonBox);
    connect(useCurrentButton, SIGNAL(clicked()),
            this, SLOT(useCurrentLocation()));
    QPushButton* useDefaultButton = new QPushButton(i18n("Use default location"), buttonBox);
    connect(useDefaultButton, SIGNAL(clicked()),
            this, SLOT(useDefaultLocation()));

    QVBoxLayout* homeBoxLayout = new QVBoxLayout(homeBox);
    homeBoxLayout->addWidget(homeUrlBox);
    homeBoxLayout->addWidget(buttonBox);

    QGroupBox* startBox = new QGroupBox(i18n("Start"), vBox);

    // create 'Start with split view' checkbox
    m_startSplit = new QCheckBox(i18n("Start with split view"), startBox);
    m_startSplit->setChecked(settings->splitView());

    // create 'Start with editable navigation bar' checkbox
    m_startEditable = new QCheckBox(i18n("Start with editable navigation bar"), startBox);
    m_startEditable->setChecked(settings->editableUrl());

    QVBoxLayout* startBoxLayout = new QVBoxLayout(startBox);
    startBoxLayout->addWidget(m_startSplit);
    startBoxLayout->addWidget(m_startEditable);

    m_showDeleteCommand = new QCheckBox(i18n("Show the command 'Delete' in context menu"), vBox);
    // TODO: use global config like in Konqueror or is this a custom setting for Dolphin?
    m_showDeleteCommand->setChecked(settings->showDeleteCommand());

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);
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

    settings->setSplitView(m_startSplit->isChecked());
    settings->setEditableUrl(m_startEditable->isChecked());
    settings->setShowDeleteCommand(m_showDeleteCommand->isChecked());
}

void GeneralSettingsPage::selectHomeUrl()
{
    const QString homeUrl(m_homeUrl->text());
    KUrl url(KFileDialog::getExistingUrl(homeUrl));
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

#include "generalsettingspage.moc"
