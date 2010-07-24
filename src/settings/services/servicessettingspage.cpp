/***************************************************************************
 *   Copyright (C) 2009-2010 by Peter Penz <peter.penz@gmx.at>             *
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

#include "servicessettingspage.h"

#include "dolphin_versioncontrolsettings.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kdesktopfileactions.h>
#include <kicon.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knewstuff3/knewstuffbutton.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kstandarddirs.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShowEvent>

ServicesSettingsPage::ServicesSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_servicesList(0),
    m_vcsGroupBox(0),
    m_vcsPluginsMap(),
    m_enabledVcsPlugins()
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* label = new QLabel(i18nc("@label:textbox",
                                     "Select which services should "
                                     "be shown in the context menu."), this);
    label->setWordWrap(true);

    m_servicesList = new QListWidget(this);
    m_servicesList->setSortingEnabled(true);
    m_servicesList->setSelectionMode(QAbstractItemView::NoSelection);
    connect(m_servicesList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SIGNAL(changed()));

    KNS3::Button* downloadButton = new KNS3::Button(i18nc("@action:button", "Download New Services..."),
                                                    "servicemenu.knsrc",
                                                    this);
    connect(downloadButton, SIGNAL(dialogFinished(KNS3::Entry::List)), this, SLOT(loadServices()));

    m_vcsGroupBox = new QGroupBox(i18nc("@title:group", "Version Control Systems"), this);
    // Only show the version control group box, if a version
    // control system could be found (see loadVersionControlSystems())
    m_vcsGroupBox->hide();

    topLayout->addWidget(label);
    topLayout->addWidget(m_servicesList);
    topLayout->addWidget(downloadButton);
    topLayout->addWidget(m_vcsGroupBox);

    m_enabledVcsPlugins = VersionControlSettings::enabledPlugins();
}

ServicesSettingsPage::~ServicesSettingsPage()
{
}

void ServicesSettingsPage::applySettings()
{
    // Apply service menu settings
    KConfig config("kservicemenurc", KConfig::NoGlobals);
    KConfigGroup showGroup = config.group("Show");

    const int count = m_servicesList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_servicesList->item(i);
        const bool show = (item->checkState() == Qt::Checked);
        const QString service = item->data(Qt::UserRole).toString();
        showGroup.writeEntry(service, show);
    }

    showGroup.sync();

    // Apply version control settings
    QStringList enabledPlugins;
    QMap<QString, QCheckBox*>::const_iterator it = m_vcsPluginsMap.constBegin();
    while (it != m_vcsPluginsMap.constEnd()) {
        if (it.value()->isChecked()) {
            enabledPlugins.append(it.key());
        }
        ++it;
    }

    if (m_enabledVcsPlugins != enabledPlugins) {
        VersionControlSettings::setEnabledPlugins(enabledPlugins);
        VersionControlSettings::self()->writeConfig();

        KMessageBox::information(window(),
                                 i18nc("@info", "Dolphin must be restarted to apply the "
                                                "updated version control systems settings."),
                                 QString(), // default title
                                 QLatin1String("ShowVcsRestartInformation"));
    }
}

void ServicesSettingsPage::restoreDefaults()
{
    const int count = m_servicesList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_servicesList->item(i);
        item->setCheckState(Qt::Checked);
    }
}

void ServicesSettingsPage::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !m_initialized) {
        QMetaObject::invokeMethod(this, "loadServices", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "loadVersionControlSystems", Qt::QueuedConnection);
        m_initialized = true;
    }
    SettingsPageBase::showEvent(event);
}

void ServicesSettingsPage::loadServices()
{
    const KConfig config("kservicemenurc", KConfig::NoGlobals);
    const KConfigGroup showGroup = config.group("Show");

    const KService::List entries = KServiceTypeTrader::self()->query("KonqPopupMenu/Plugin");
    foreach (const KSharedPtr<KService>& service, entries) {
        const QString file = KStandardDirs::locate("services", service->entryPath());
        const QList<KServiceAction> serviceActions =
                                    KDesktopFileActions::userDefinedServices(file, true);

        KDesktopFile desktopFile(file);
        const QString subMenuName = desktopFile.desktopGroup().readEntry("X-KDE-Submenu");

        foreach (const KServiceAction& action, serviceActions) {
            const QString service = action.name();
            const bool addService = !action.noDisplay()
                                    && !action.isSeparator()
                                    && !isInServicesList(service);

            if (addService) {
                const QString itemName = subMenuName.isEmpty()
                                         ? action.text()
                                         : i18nc("@item:inmenu", "%1: %2", subMenuName, action.text());
                QListWidgetItem* item = new QListWidgetItem(KIcon(action.icon()),
                                                            itemName,
                                                            m_servicesList);
                item->setData(Qt::UserRole, service);
                const bool show = showGroup.readEntry(service, true);
                item->setCheckState(show ? Qt::Checked : Qt::Unchecked);
            }
        }
    }
}

void ServicesSettingsPage::loadVersionControlSystems()
{
    const QStringList enabledPlugins = VersionControlSettings::enabledPlugins();

    // Create a checkbox for each available version control plugin
    const KService::List pluginServices = KServiceTypeTrader::self()->query("FileViewVersionControlPlugin");
    for (KService::List::ConstIterator it = pluginServices.constBegin(); it != pluginServices.constEnd(); ++it) {
        const QString pluginName = (*it)->name();
        QCheckBox* checkBox = new QCheckBox(pluginName, m_vcsGroupBox);
        checkBox->setChecked(enabledPlugins.contains(pluginName));
        connect(checkBox, SIGNAL(clicked()), this, SIGNAL(changed()));
        m_vcsPluginsMap.insert(pluginName, checkBox);
    }

    // Add the checkboxes into a grid layout of 2 columns
    QGridLayout* layout = new QGridLayout(m_vcsGroupBox);
    const int maxRows = (m_vcsPluginsMap.count() + 1) / 2;

    int index = 0;
    QMap<QString, QCheckBox*>::const_iterator it = m_vcsPluginsMap.constBegin();
    while (it != m_vcsPluginsMap.constEnd()) {
        const int column = index / maxRows;
        const int row = index % maxRows;
        layout->addWidget(it.value(), row, column);
        ++it;
        ++index;
    }

    m_vcsGroupBox->setVisible(!m_vcsPluginsMap.isEmpty());
}

bool ServicesSettingsPage::isInServicesList(const QString& service) const
{
    const int count = m_servicesList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_servicesList->item(i);
        if (item->data(Qt::UserRole).toString() == service) {
            return true;
        }
    }
    return false;
}

#include "servicessettingspage.moc"
