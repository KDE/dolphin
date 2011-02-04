/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "statusbarsettingspage.h"

#include <dolphin_generalsettings.h>

#include <KDialog>
#include <KLocale>
#include <KVBox>

#include <settings/dolphinsettings.h>

#include <QCheckBox>
#include <QVBoxLayout>

StatusBarSettingsPage::StatusBarSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_showZoomSlider(0),
    m_showSpaceInfo(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(KDialog::spacingHint());

    m_showZoomSlider = new QCheckBox(i18nc("@option:check", "Show zoom slider"), vBox);

    m_showSpaceInfo = new QCheckBox(i18nc("@option:check", "Show space information"), vBox);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();

    connect(m_showZoomSlider, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_showSpaceInfo, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
}

StatusBarSettingsPage::~StatusBarSettingsPage()
{
}

void StatusBarSettingsPage::applySettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setShowZoomSlider(m_showZoomSlider->isChecked());
    settings->setShowSpaceInfo(m_showSpaceInfo->isChecked());
    settings->writeConfig();
}

void StatusBarSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void StatusBarSettingsPage::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_showZoomSlider->setChecked(settings->showZoomSlider());
    m_showSpaceInfo->setChecked(settings->showSpaceInfo());
}

#include "statusbarsettingspage.moc"
