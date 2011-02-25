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

ServicesSettingsPage::ServicesSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_listView(0),
    m_vcsGroupBox(0),
    m_vcsPluginsMap(),
    m_enabledVcsPlugins()
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* label = new QLabel(i18nc("@label:textbox",
                                     "Select which services should "
                                     "be shown in the context menu:"), this);
    label->setWordWrap(true);

    m_listView = new QListView(this);
    ServiceItemDelegate* delegate = new ServiceItemDelegate(m_listView, m_listView);
    ServiceModel* serviceModel = new ServiceModel(this);
    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(serviceModel);
    proxyModel->setSortRole(Qt::DisplayRole);
    m_listView->setModel(proxyModel);
    m_listView->setItemDelegate(delegate);
    m_listView->setVerticalScrollMode(QListView::ScrollPerPixel);
    connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SIGNAL(changed()));

    KNS3::Button* downloadButton = new KNS3::Button(i18nc("@action:button", "Download New Services..."),
                                                    "servicemenu.knsrc",
                                                    this);
    connect(downloadButton, SIGNAL(dialogFinished(KNS3::Entry::List)), this, SLOT(loadServices()));

    m_vcsGroupBox = new QGroupBox(i18nc("@title:group", "Version Control Systems"), this);
    // Only show the version control group box, if a version
    // control system could be found (see loadVersionControlSystems())
    m_vcsGroupBox->hide();

    topLayout->addWidget(label);
    topLayout->addWidget(m_listView);
    topLayout->addWidget(downloadButton);
    topLayout->addWidget(m_vcsGroupBox);

    m_enabledVcsPlugins = VersionControlSettings::enabledPlugins();
}

ServicesSettingsPage::~ServicesSettingsPage()
{
}

void ServicesSettingsPage::applySettings()
{
    if (!m_initialized) {
        return;
    }

    // Apply service menu settingsentries
    KConfig config("kservicemenurc", KConfig::NoGlobals);
    KConfigGroup showGroup = config.group("Show");

    const QAbstractItemModel* model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        const bool show = model->data(index, Qt::CheckStateRole).toBool();
        const QString service = model->data(index, ServiceModel::DesktopEntryNameRole).toString();
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
    QAbstractItemModel* model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        model->setData(index, true, Qt::CheckStateRole);
    }
}

void ServicesSettingsPage::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !m_initialized) {
        loadServices();
        loadVersionControlSystems();
        m_initialized = true;
    }
    SettingsPageBase::showEvent(event);
}

void ServicesSettingsPage::loadServices()
{
    QAbstractItemModel* model = m_listView->model();

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
                const bool show = showGroup.readEntry(serviceName, true);

                model->insertRow(0);
                const QModelIndex index = model->index(0, 0);
                model->setData(index, action.icon(), Qt::DecorationRole);
                model->setData(index, show, Qt::CheckStateRole);
                model->setData(index, itemName, Qt::DisplayRole);
                model->setData(index, serviceName, ServiceModel::DesktopEntryNameRole);
            }
        }
    }

    // Load service plugins that implement the KFileItemActionPlugin interface
    const KService::List pluginServices = KServiceTypeTrader::self()->query("KFileItemAction/Plugin");
    foreach (const KSharedPtr<KService>& service, pluginServices) {
        const QString desktopEntryName = service->desktopEntryName();
        if (!isInServicesList(desktopEntryName)) {
            const bool show = showGroup.readEntry(desktopEntryName, true);

            model->insertRow(0);
            const QModelIndex index = model->index(0, 0);
            model->setData(index, service->icon(), Qt::DecorationRole);
            model->setData(index, show, Qt::CheckStateRole);
            model->setData(index, service->name(), Qt::DisplayRole);
            model->setData(index, desktopEntryName, ServiceModel::DesktopEntryNameRole);
        }
    }

    model->sort(Qt::DisplayRole);
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
    QAbstractItemModel* model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        if (model->data(index, ServiceModel::DesktopEntryNameRole).toString() == service) {
            return true;
        }
    }
    return false;
}

#include "servicessettingspage.moc"
