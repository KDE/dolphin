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

#include <KDialog>
#include <KLocale>

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

#include <views/viewproperties.h>

const bool CONFIRM_TRASH = false;
const bool CONFIRM_DELETE = true;

BehaviorSettingsPage::BehaviorSettingsPage(const KUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_localProps(0),
    m_globalProps(0),
    m_confirmMoveToTrash(0),
    m_confirmDelete(0),
    m_renameInline(0),
    m_showToolTips(0),
    m_showSelectionToggle(0),
    m_naturalSorting(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    // 'View Properties' box
    QGroupBox* propsBox = new QGroupBox(i18nc("@title:group", "View Properties"), this);
    propsBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    m_localProps = new QRadioButton(i18nc("@option:radio", "Remember view properties for each folder"), propsBox);

    m_globalProps = new QRadioButton(i18nc("@option:radio", "Use common view properties for all folders"), propsBox);

    QVBoxLayout* propsBoxLayout = new QVBoxLayout(propsBox);
    propsBoxLayout->addWidget(m_localProps);
    propsBoxLayout->addWidget(m_globalProps);

    // 'Ask Confirmation For' box
    QGroupBox* confirmBox = new QGroupBox(i18nc("@title:group", "Ask For Confirmation When"), this);
    confirmBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_confirmMoveToTrash = new QCheckBox(i18nc("@option:check Ask for Confirmation When",
                                               "Moving files or folders to trash"), confirmBox);
    m_confirmDelete = new QCheckBox(i18nc("@option:check Ask for Confirmation When",
                                          "Deleting files or folders"), confirmBox);
    m_confirmClosingMultipleTabs = new QCheckBox(i18nc("@option:check Ask for Confirmation When",
                                                       "Closing windows with multiple tabs"), confirmBox);

    QVBoxLayout* confirmBoxLayout = new QVBoxLayout(confirmBox);
    confirmBoxLayout->addWidget(m_confirmMoveToTrash);
    confirmBoxLayout->addWidget(m_confirmDelete);
    confirmBoxLayout->addWidget(m_confirmClosingMultipleTabs);

    // 'Rename inline'
    m_renameInline = new QCheckBox(i18nc("@option:check", "Rename inline"), this);

    // 'Show tooltips'
    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show tooltips"), this);

    // 'Show selection marker'
    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection marker"), this);

    // 'Natural sorting of items'
    m_naturalSorting = new QCheckBox(i18nc("option:check", "Natural sorting of items"), this);

    topLayout->addWidget(propsBox);
    topLayout->addWidget(confirmBox);
    topLayout->addWidget(m_renameInline);
    topLayout->addWidget(m_showToolTips);
    topLayout->addWidget(m_showSelectionToggle);
    topLayout->addWidget(m_naturalSorting);
    topLayout->addStretch();

    loadSettings();

    connect(m_localProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_globalProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_confirmMoveToTrash, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_confirmDelete, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_confirmClosingMultipleTabs, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_renameInline, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_showToolTips, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_showSelectionToggle, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_naturalSorting, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
}

void BehaviorSettingsPage::applySettings()
{
    ViewProperties props(m_url);  // read current view properties

    const bool useGlobalProps = m_globalProps->isChecked();

    GeneralSettings* settings = GeneralSettings::self();
    settings->setGlobalViewProps(useGlobalProps);

    if (useGlobalProps) {
        // Remember the global view properties by applying the current view properties.
        // It is important that GeneralSettings::globalViewProps() is set before
        // the class ViewProperties is used, as ViewProperties uses this setting
        // to find the destination folder for storing the view properties.
        ViewProperties globalProps(m_url);
        globalProps.setDirProperties(props);
    }

    KSharedConfig::Ptr kioConfig = KSharedConfig::openConfig("kiorc", KConfig::NoGlobals);
    KConfigGroup confirmationGroup(kioConfig, "Confirmations");
    confirmationGroup.writeEntry("ConfirmTrash", m_confirmMoveToTrash->isChecked());
    confirmationGroup.writeEntry("ConfirmDelete", m_confirmDelete->isChecked());
    confirmationGroup.sync();

    settings->setConfirmClosingMultipleTabs(m_confirmClosingMultipleTabs->isChecked());
    settings->setRenameInline(m_renameInline->isChecked());
    settings->setShowToolTips(m_showToolTips->isChecked());
    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
    settings->writeConfig();

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
    m_confirmMoveToTrash->setChecked(CONFIRM_TRASH);
    m_confirmDelete->setChecked(CONFIRM_DELETE);
}

void BehaviorSettingsPage::loadSettings()
{
    if (GeneralSettings::globalViewProps()) {
        m_globalProps->setChecked(true);
    } else {
        m_localProps->setChecked(true);
    }

    KSharedConfig::Ptr kioConfig = KSharedConfig::openConfig("kiorc", KConfig::IncludeGlobals);
    const KConfigGroup confirmationGroup(kioConfig, "Confirmations");
    m_confirmMoveToTrash->setChecked(confirmationGroup.readEntry("ConfirmTrash", CONFIRM_TRASH));
    m_confirmDelete->setChecked(confirmationGroup.readEntry("ConfirmDelete", CONFIRM_DELETE));

    m_confirmClosingMultipleTabs->setChecked(GeneralSettings::confirmClosingMultipleTabs());
    m_renameInline->setChecked(GeneralSettings::renameInline());
    m_showToolTips->setChecked(GeneralSettings::showToolTips());
    m_showSelectionToggle->setChecked(GeneralSettings::showSelectionToggle());
    m_naturalSorting->setChecked(KGlobalSettings::naturalSorting());
}

#include "behaviorsettingspage.moc"
