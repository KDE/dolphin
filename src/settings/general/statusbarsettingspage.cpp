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

#include <QCheckBox>
#include <QVBoxLayout>

StatusBarSettingsPage::StatusBarSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_showZoomSlider(0),
    m_showSpaceInfo(0)
{
    m_showZoomSlider = new QCheckBox(i18nc("@option:check", "Show zoom slider"), this);
    m_showSpaceInfo = new QCheckBox(i18nc("@option:check", "Show space information"), this);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->addSpacing(KDialog::spacingHint());
    topLayout->addWidget(m_showZoomSlider);
    topLayout->addWidget(m_showSpaceInfo);
    topLayout->addStretch();

    loadSettings();

    connect(m_showZoomSlider, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_showSpaceInfo, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
}

StatusBarSettingsPage::~StatusBarSettingsPage()
{
}

void StatusBarSettingsPage::applySettings()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->setShowZoomSlider(m_showZoomSlider->isChecked());
    settings->setShowSpaceInfo(m_showSpaceInfo->isChecked());
    settings->writeConfig();
}

void StatusBarSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void StatusBarSettingsPage::loadSettings()
{
    m_showZoomSlider->setChecked(GeneralSettings::showZoomSlider());
    m_showSpaceInfo->setChecked(GeneralSettings::showSpaceInfo());
}

#include "statusbarsettingspage.moc"
