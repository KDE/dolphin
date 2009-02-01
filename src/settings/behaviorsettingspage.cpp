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

#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

#include <viewproperties.h>

#include <kdialog.h>
#include <klocale.h>
#include <kvbox.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

BehaviorSettingsPage::BehaviorSettingsPage(const KUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_localProps(0),
    m_globalProps(0),
    m_confirmMoveToTrash(0),
    m_confirmDelete(0),
    m_renameInline(0),
    m_showToolTips(0),
    m_showSelectionToggle(0)
{
    const int spacing = KDialog::spacingHint();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(spacing);

    // 'View Properties' box
    QGroupBox* propsBox = new QGroupBox(i18nc("@title:group", "View Properties"), vBox);

    m_localProps = new QRadioButton(i18nc("@option:radio", "Remember view properties for each folder"), propsBox);
    connect(m_localProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    m_globalProps = new QRadioButton(i18nc("@option:radio", "Use common view properties for all folders"), propsBox);
    connect(m_globalProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    QVBoxLayout* propsBoxLayout = new QVBoxLayout(propsBox);
    propsBoxLayout->addWidget(m_localProps);
    propsBoxLayout->addWidget(m_globalProps);

    // 'Ask Confirmation For' box
    QGroupBox* confirmBox = new QGroupBox(i18nc("@title:group", "Ask For Confirmation When"), vBox);
    m_confirmMoveToTrash = new QCheckBox(i18nc("@option:check Ask for Confirmation When",
                                               "Moving files or folders to trash"), confirmBox);
    connect(m_confirmMoveToTrash, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    m_confirmDelete = new QCheckBox(i18nc("@option:check Ask for Confirmation When",
                                          "Deleting files or folders"), confirmBox);
    connect(m_confirmDelete, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    QVBoxLayout* confirmBoxLayout = new QVBoxLayout(confirmBox);
    confirmBoxLayout->addWidget(m_confirmMoveToTrash);
    confirmBoxLayout->addWidget(m_confirmDelete);

    m_renameInline = new QCheckBox(i18nc("@option:check", "Rename inline"), vBox);
    connect(m_renameInline, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show tooltips"), vBox);
    connect(m_showToolTips, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection marker"), vBox);
    connect(m_showSelectionToggle, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
}

void BehaviorSettingsPage::applySettings()
{
    ViewProperties props(m_url);  // read current view properties

    const bool useGlobalProps = m_globalProps->isChecked();

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
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

    settings->setRenameInline(m_renameInline->isChecked());
    settings->setShowToolTips(m_showToolTips->isChecked());
    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
}

void BehaviorSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    //TODO: Reset default settings for both trash and show delete commands (confirmations).
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void BehaviorSettingsPage::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    if (settings->globalViewProps()) {
        m_globalProps->setChecked(true);
    } else {
        m_localProps->setChecked(true);
    }

    KSharedConfig::Ptr kioConfig = KSharedConfig::openConfig("kiorc", KConfig::IncludeGlobals);
    const KConfigGroup confirmationGroup(kioConfig, "Confirmations");
    m_confirmMoveToTrash->setChecked(confirmationGroup.readEntry("ConfirmTrash", false));
    m_confirmDelete->setChecked(confirmationGroup.readEntry("ConfirmDelete", true));

    m_renameInline->setChecked(settings->renameInline());
    m_showToolTips->setChecked(settings->showToolTips());
    m_showSelectionToggle->setChecked(settings->showSelectionToggle());
}

#include "behaviorsettingspage.moc"
