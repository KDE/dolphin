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
    m_viewProps(0),
    m_showToolTips(0),
    m_showSelectionToggle(0),
    m_naturalSorting(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    // View properties
    QLabel* viewPropsLabel = new QLabel(i18nc("@label", "View properties:"), this);

    m_viewProps = new KComboBox(this);
    const bool useGlobalProps = true;
    m_viewProps->addItem(i18nc("@option:radio", "Remember view properties for each folder"), !useGlobalProps);
    m_viewProps->addItem(i18nc("@option:radio", "Use common view properties for all folders"), useGlobalProps);

    QHBoxLayout* viewPropsLayout = new QHBoxLayout(this);
    viewPropsLayout->addWidget(viewPropsLabel, 0, Qt::AlignRight);
    viewPropsLayout->addWidget(m_viewProps);

    // 'Show tooltips'
    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show tooltips"), this);

    // 'Show selection marker'
    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection marker"), this);

    // 'Natural sorting of items'
    m_naturalSorting = new QCheckBox(i18nc("option:check", "Natural sorting of items"), this);

    topLayout->addSpacing(KDialog::spacingHint());
    topLayout->addLayout(viewPropsLayout);
    topLayout->addSpacing(KDialog::spacingHint());
    topLayout->addWidget(m_showToolTips);
    topLayout->addWidget(m_showSelectionToggle);
    topLayout->addWidget(m_naturalSorting);
    topLayout->addStretch();

    loadSettings();

    connect(m_viewProps, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
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

    const int index = m_viewProps->currentIndex();
    const bool useGlobalProps = m_viewProps->itemData(index).toBool();
    settings->setGlobalViewProps(useGlobalProps);

    settings->setShowToolTips(m_showToolTips->isChecked());
    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
    settings->writeConfig();

    if (useGlobalProps) {
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
    const int index = (m_viewProps->itemData(0).toBool() == GeneralSettings::globalViewProps()) ? 0 : 1;
    m_viewProps->setCurrentIndex(index);

    m_showToolTips->setChecked(GeneralSettings::showToolTips());
    m_showSelectionToggle->setChecked(GeneralSettings::showSelectionToggle());
    m_naturalSorting->setChecked(KGlobalSettings::naturalSorting());
}

#include "behaviorsettingspage.moc"
