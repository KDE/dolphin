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

#include "behaviorsettingspage.h"

#include "dolphin_generalsettings.h"

#include <KComboBox>
#include <KDialog>
#include <KLocale>

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

#include <views/viewproperties.h>

BehaviorSettingsPage::BehaviorSettingsPage(const KUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_localViewProps(0),
    m_globalViewProps(0),
    m_showToolTips(0),
    m_showSelectionToggle(0),
    m_naturalSorting(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    // View properties
    QGroupBox* viewPropsBox = new QGroupBox(i18nc("@title:group", "View"), this);
    viewPropsBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    m_localViewProps = new QRadioButton(i18nc("@option:radio", "Remember properties for each folder"), viewPropsBox);
    m_globalViewProps = new QRadioButton(i18nc("@option:radio", "Use common properties for all folders"), viewPropsBox);

    QVBoxLayout* viewPropsLayout = new QVBoxLayout(viewPropsBox);
    viewPropsLayout->addWidget(m_localViewProps);
    viewPropsLayout->addWidget(m_globalViewProps);

    // 'Show tooltips'
    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show tooltips"), this);

    // 'Show selection marker'
    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection marker"), this);

    // 'Natural sorting of items'
    m_naturalSorting = new QCheckBox(i18nc("option:check", "Natural sorting of items"), this);

    topLayout->addWidget(viewPropsBox);
    topLayout->addWidget(m_showToolTips);
    topLayout->addWidget(m_showSelectionToggle);
    topLayout->addWidget(m_naturalSorting);
    topLayout->addStretch();

    loadSettings();

    connect(m_localViewProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_globalViewProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_showToolTips, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_showSelectionToggle, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_naturalSorting, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
}

void BehaviorSettingsPage::applySettings()
{
    GeneralSettings* settings = GeneralSettings::self();
    ViewProperties props(m_url);  // read current view properties

    const bool useGlobalViewProps = m_globalViewProps->isChecked();
    settings->setGlobalViewProps(useGlobalViewProps);

    settings->setShowToolTips(m_showToolTips->isChecked());
    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
    settings->writeConfig();

    if (useGlobalViewProps) {
        // Remember the global view properties by applying the current view properties.
        // It is important that GeneralSettings::globalViewProps() is set before
        // the class ViewProperties is used, as ViewProperties uses this setting
        // to find the destination folder for storing the view properties.
        ViewProperties globalProps(m_url);
        globalProps.setDirProperties(props);
    }

    const bool naturalSorting = m_naturalSorting->isChecked();
    if (KGlobalSettings::naturalSorting() != naturalSorting) {
        KConfigGroup group(KGlobal::config(), "KDE");
        group.writeEntry("NaturalSorting", naturalSorting, KConfig::Persistent | KConfig::Global);
        KGlobalSettings::emitChange(KGlobalSettings::NaturalSortingChanged);
    }
}

void BehaviorSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void BehaviorSettingsPage::loadSettings()
{
    const bool useGlobalViewProps = GeneralSettings::globalViewProps();
    m_localViewProps->setChecked(!useGlobalViewProps);
    m_globalViewProps->setChecked(useGlobalViewProps);

    m_showToolTips->setChecked(GeneralSettings::showToolTips());
    m_showSelectionToggle->setChecked(GeneralSettings::showSelectionToggle());
    m_naturalSorting->setChecked(KGlobalSettings::naturalSorting());
}

#include "behaviorsettingspage.moc"
