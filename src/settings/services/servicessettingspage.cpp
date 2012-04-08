/***************************************************************************
 *   Copyright (C) 2009-2010 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "dolphin_generalsettings.h"
#include "dolphin_versioncontrolsettings.h"

#include <KConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <kdesktopfileactions.h>
#include <KIcon>
#include <KLocale>
#include <KMessageBox>
#include <knewstuff3/knewstuffbutton.h>
#include <KService>
#include <KServiceTypeTrader>
#include <KStandardDirs>

#include <settings/serviceitemdelegate.h>
#include <settings/servicemodel.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QShowEvent>

namespace
{
    const bool ShowDeleteDefault = false;
    const char* VersionControlServicePrefix = "_version_control_";
    const char* DeleteService = "_delete";
    const char* CopyToMoveToService ="_copy_to_move_to";
}

ServicesSettingsPage::ServicesSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_serviceModel(0),
    m_sortModel(0),
    m_listView(0),
    m_enabledVcsPlugins()
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* label = new QLabel(i18nc("@label:textbox",
                                     "Select which services should "
                                     "be shown in the context menu:"), this);
    label->setWordWrap(true);

    m_listView = new QListView(this);
    ServiceItemDelegate* delegate = new ServiceItemDelegate(m_listView, m_listView);
    m_serviceModel = new ServiceModel(this);
    m_sortModel = new QSortFilterProxyModel(this);
    m_sortModel->setSourceModel(m_serviceModel);
    m_sortModel->setSortRole(Qt::DisplayRole);
    m_listView->setModel(m_sortModel);
    m_listView->setItemDelegate(delegate);
    m_listView->setVerticalScrollMode(QListView::ScrollPerPixel);
    connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SIGNAL(changed()));

    KNS3::Button* downloadButton = new KNS3::Button(i18nc("@action:button", "Download New Services..."),
                                                    "servicemenu.knsrc",
                                                    this);
    connect(downloadButton, SIGNAL(dialogFinished(KNS3::Entry::List)), this, SLOT(loadServices()));

    topLayout->addWidget(label);
    topLayout->addWidget(m_listView);
    topLayout->addWidget(downloadButton);

    m_enabledVcsPlugins = VersionControlSettings::enabledPlugins();
    qSort(m_enabledVcsPlugins);
}

ServicesSettingsPage::~ServicesSettingsPage()
{
}

void ServicesSettingsPage::applySettings()
{
    if (!m_initialized) {
        return;
    }

    KConfig config("kservicemenurc", KConfig::NoGlobals);
    KConfigGroup showGroup = config.group("Show");

    QStringList enabledPlugins;

    const QAbstractItemModel* model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        const QString service = model->data(index, ServiceModel::DesktopEntryNameRole).toString();
        const bool checked = model->data(index, Qt::CheckStateRole).toBool();

        if (service.startsWith(VersionControlServicePrefix)) {
            if (checked) {
                enabledPlugins.append(model->data(index, Qt::DisplayRole).toString());
            }
        } else if (service == QLatin1String(DeleteService)) {
            KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
            KConfigGroup configGroup(globalConfig, "KDE");
            configGroup.writeEntry("ShowDeleteCommand", checked);
            configGroup.sync();
        } else if (service == QLatin1String(CopyToMoveToService)) {
            GeneralSettings::setShowCopyMoveMenu(checked);
            GeneralSettings::self()->writeConfig();
        } else {
            showGroup.writeEntry(service, checked);
        }
    }

    showGroup.sync();

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
    QAbstractItemModel* model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        const QString service = model->data(index, ServiceModel::DesktopEntryNameRole).toString();

        const bool checked = !service.startsWith(VersionControlServicePrefix)
                             && service != QLatin1String(DeleteService)
                             && service != QLatin1String(CopyToMoveToService);
        model->setData(index, checked, Qt::CheckStateRole);
    }
}

void ServicesSettingsPage::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !m_initialized) {
        loadServices();

        loadVersionControlSystems();

        // Add "Show 'Delete' command" as service
        KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::IncludeGlobals);
        KConfigGroup configGroup(globalConfig, "KDE");
        addRow("edit-delete",
               i18nc("@option:check", "Delete"),
               DeleteService,
               configGroup.readEntry("ShowDeleteCommand", ShowDeleteDefault));

        // Add "Show 'Copy To' and 'Move To' commands" as service
        addRow("edit-copy",
               i18nc("@option:check", "'Copy To' and 'Move To' commands"),
               CopyToMoveToService,
               GeneralSettings::showCopyMoveMenu());

        m_sortModel->sort(Qt::DisplayRole);

        m_initialized = true;
    }
    SettingsPageBase::showEvent(event);
}

void ServicesSettingsPage::loadServices()
{
    const KConfig config("kservicemenurc", KConfig::NoGlobals);
    const KConfigGroup showGroup = config.group("Show");

    // Load generic services
    const KService::List entries = KServiceTypeTrader::self()->query("KonqPopupMenu/Plugin");
    foreach (const KSharedPtr<KService>& service, entries) {
        const QString file = KStandardDirs::locate("services", service->entryPath());
        const QList<KServiceAction> serviceActions =
                                    KDesktopFileActions::userDefinedServices(file, true);

        KDesktopFile desktopFile(file);
        const QString subMenuName = desktopFile.desktopGroup().readEntry("X-KDE-Submenu");

        foreach (const KServiceAction& action, serviceActions) {
            const QString serviceName = action.name();
            const bool addService = !action.noDisplay()
                                    && !action.isSeparator()
                                    && !isInServicesList(serviceName);

            if (addService) {
                const QString itemName = subMenuName.isEmpty()
                                         ? action.text()
                                         : i18nc("@item:inmenu", "%1: %2", subMenuName, action.text());
                const bool checked = showGroup.readEntry(serviceName, true);
                addRow(action.icon(), itemName, serviceName, checked);
            }
        }
    }

    // Load service plugins that implement the KFileItemActionPlugin interface
    const KService::List pluginServices = KServiceTypeTrader::self()->query("KFileItemAction/Plugin");
    foreach (const KSharedPtr<KService>& service, pluginServices) {
        const QString desktopEntryName = service->desktopEntryName();
        if (!isInServicesList(desktopEntryName)) {
            const bool checked = showGroup.readEntry(desktopEntryName, true);
            addRow(service->icon(), service->name(), desktopEntryName, checked);
        }
    }

    m_sortModel->sort(Qt::DisplayRole);
}

void ServicesSettingsPage::loadVersionControlSystems()
{
    const QStringList enabledPlugins = VersionControlSettings::enabledPlugins();

    // Create a checkbox for each available version control plugin
    const KService::List pluginServices = KServiceTypeTrader::self()->query("FileViewVersionControlPlugin");
    for (KService::List::ConstIterator it = pluginServices.constBegin(); it != pluginServices.constEnd(); ++it) {
        const QString pluginName = (*it)->name();
        addRow("code-class",
               pluginName,
               VersionControlServicePrefix + pluginName,
               enabledPlugins.contains(pluginName));
    }

    m_sortModel->sort(Qt::DisplayRole);
}

bool ServicesSettingsPage::isInServicesList(const QString& service) const
{
    for (int i = 0; i < m_serviceModel->rowCount(); ++i) {
        const QModelIndex index = m_serviceModel->index(i, 0);
        if (m_serviceModel->data(index, ServiceModel::DesktopEntryNameRole).toString() == service) {
            return true;
        }
    }
    return false;
}

void ServicesSettingsPage::addRow(const QString& icon,
                                  const QString& text,
                                  const QString& value,
                                  bool checked)
{
    m_serviceModel->insertRow(0);

    const QModelIndex index = m_serviceModel->index(0, 0);
    m_serviceModel->setData(index, icon, Qt::DecorationRole);
    m_serviceModel->setData(index, text, Qt::DisplayRole);
    m_serviceModel->setData(index, value, ServiceModel::DesktopEntryNameRole);
    m_serviceModel->setData(index, checked, Qt::CheckStateRole);
}

#include "servicessettingspage.moc"
