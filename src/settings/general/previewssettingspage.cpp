/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "previewssettingspage.h"

#include "dolphin_generalsettings.h"
#include "configurepreviewplugindialog.h"

#include <KConfigGroup>
#include <KDialog>
#include <KGlobal>
#include <KLocale>
#include <KNumInput>
#include <KServiceTypeTrader>
#include <KService>

#include <settings/serviceitemdelegate.h>
#include <settings/servicemodel.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QScrollBar>
#include <QShowEvent>
#include <QSlider>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

// default settings
namespace {
    const bool UseThumbnails = true;
    const int MaxRemotePreviewSize = 0; // 0 MB
}

PreviewsSettingsPage::PreviewsSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_listView(0),
    m_enabledPreviewPlugins(),
    m_remoteFileSizeBox(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* showPreviewsLabel = new QLabel(i18nc("@title:group", "Show previews for:"), this);

    m_listView = new QListView(this);

    ServiceItemDelegate* delegate = new ServiceItemDelegate(m_listView, m_listView);
    connect(delegate, SIGNAL(requestServiceConfiguration(QModelIndex)),
            this, SLOT(configureService(QModelIndex)));

    ServiceModel* serviceModel = new ServiceModel(this);
    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(serviceModel);
    proxyModel->setSortRole(Qt::DisplayRole);

    m_listView->setModel(proxyModel);
    m_listView->setItemDelegate(delegate);
    m_listView->setVerticalScrollMode(QListView::ScrollPerPixel);

    QLabel* remoteFileSizeLabel = new QLabel(i18nc("@label", "Skip previews for remote files above:"), this);

    m_remoteFileSizeBox = new KIntSpinBox(this);
    m_remoteFileSizeBox->setSingleStep(1);
    m_remoteFileSizeBox->setSuffix(QLatin1String(" MB"));
    m_remoteFileSizeBox->setRange(0, 9999999); /* MB */

    QHBoxLayout* fileSizeBoxLayout = new QHBoxLayout(this);
    fileSizeBoxLayout->addWidget(remoteFileSizeLabel, 0, Qt::AlignRight);
    fileSizeBoxLayout->addWidget(m_remoteFileSizeBox);

    topLayout->addSpacing(KDialog::spacingHint());
    topLayout->addWidget(showPreviewsLabel);
    topLayout->addWidget(m_listView);
    topLayout->addLayout(fileSizeBoxLayout);

    loadSettings();

    connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SIGNAL(changed()));
    connect(m_remoteFileSizeBox, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
}

PreviewsSettingsPage::~PreviewsSettingsPage()
{
}

void PreviewsSettingsPage::applySettings()
{
    const QAbstractItemModel* model = m_listView->model();
    const int rowCount = model->rowCount();
    if (rowCount > 0) {
        m_enabledPreviewPlugins.clear();
        for (int i = 0; i < rowCount; ++i) {
            const QModelIndex index = model->index(i, 0);
            const bool checked = model->data(index, Qt::CheckStateRole).toBool();
            if (checked) {
                const QString enabledPlugin = model->data(index, Qt::UserRole).toString();
                m_enabledPreviewPlugins.append(enabledPlugin);
            }
        }
    }

    KConfigGroup globalConfig(KGlobal::config(), QLatin1String("PreviewSettings"));
    globalConfig.writeEntry("Plugins", m_enabledPreviewPlugins);

    const qulonglong maximumRemoteSize = static_cast<qulonglong>(m_remoteFileSizeBox->value()) * 1024 * 1024;
    globalConfig.writeEntry("MaximumRemoteSize",
                            maximumRemoteSize,
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.sync();
}

void PreviewsSettingsPage::restoreDefaults()
{
    m_remoteFileSizeBox->setValue(MaxRemotePreviewSize);
}

void PreviewsSettingsPage::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !m_initialized) {
        loadPreviewPlugins();
        m_initialized = true;
    }
    SettingsPageBase::showEvent(event);
}

void PreviewsSettingsPage::configureService(const QModelIndex& index)
{
    const QAbstractItemModel* model = index.model();
    const QString pluginName = model->data(index).toString();
    const QString desktopEntryName = model->data(index, ServiceModel::DesktopEntryNameRole).toString();

    ConfigurePreviewPluginDialog* dialog = new ConfigurePreviewPluginDialog(pluginName, desktopEntryName, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void PreviewsSettingsPage::loadPreviewPlugins()
{
    QAbstractItemModel* model = m_listView->model();

    const KService::List plugins = KServiceTypeTrader::self()->query(QLatin1String("ThumbCreator"));
    foreach (const KSharedPtr<KService>& service, plugins) {
        const bool configurable = service->property("Configurable", QVariant::Bool).toBool();
        const bool show = m_enabledPreviewPlugins.contains(service->desktopEntryName());

        model->insertRow(0);
        const QModelIndex index = model->index(0, 0);
        model->setData(index, show, Qt::CheckStateRole);
        model->setData(index, configurable, ServiceModel::ConfigurableRole);
        model->setData(index, service->name(), Qt::DisplayRole);
        model->setData(index, service->desktopEntryName(), ServiceModel::DesktopEntryNameRole);
    }

    model->sort(Qt::DisplayRole);
}

void PreviewsSettingsPage::loadSettings()
{
    KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    m_enabledPreviewPlugins = globalConfig.readEntry("Plugins", QStringList()
                                                     << QLatin1String("directorythumbnail")
                                                     << QLatin1String("imagethumbnail")
                                                     << QLatin1String("jpegthumbnail"));

    // If the user is upgrading from KDE <= 4.6, we must check if he had the 'jpegrotatedthumbnail' plugin enabled.
    // This plugin does not exist any more in KDE >= 4.7, so we have to replace it with the 'jpegthumbnail' plugin.
    //
    // Note that the upgrade to the correct plugin is done already in KFilePreviewGenerator. However, if Konqueror is
    // opened in web browsing mode and the Settings dialog is opened, we might end up here before KFilePreviewGenerator's
    // constructor is ever called -> the plugin replacement should be done here as well.
    if (m_enabledPreviewPlugins.contains(QLatin1String("jpegrotatedthumbnail"))) {
        m_enabledPreviewPlugins.removeAll(QLatin1String("jpegrotatedthumbnail"));
        m_enabledPreviewPlugins.append(QLatin1String("jpegthumbnail"));
        globalConfig.writeEntry("Plugins", m_enabledPreviewPlugins);
        globalConfig.sync();
    }

    const qulonglong defaultRemotePreview = static_cast<qulonglong>(MaxRemotePreviewSize) * 1024 * 1024;
    const qulonglong maxRemoteByteSize = globalConfig.readEntry("MaximumRemoteSize", defaultRemotePreview);
    const int maxRemoteMByteSize = maxRemoteByteSize / (1024 * 1024);
    m_remoteFileSizeBox->setValue(maxRemoteMByteSize);
}

#include "previewssettingspage.moc"
