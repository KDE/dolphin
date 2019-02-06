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
#include "settings/serviceitemdelegate.h"
#include "settings/servicemodel.h"

#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <KServiceTypeTrader>

#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QSpinBox>

// default settings
namespace {
    const int MaxRemotePreviewSize = 0; // 0 MB
}

PreviewsSettingsPage::PreviewsSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_listView(nullptr),
    m_enabledPreviewPlugins(),
    m_remoteFileSizeBox(nullptr)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* showPreviewsLabel = new QLabel(i18nc("@title:group", "Show previews in the view for:"), this);

    m_listView = new QListView(this);

    ServiceItemDelegate* delegate = new ServiceItemDelegate(m_listView, m_listView);
    connect(delegate, &ServiceItemDelegate::requestServiceConfiguration,
            this, &PreviewsSettingsPage::configureService);

    ServiceModel* serviceModel = new ServiceModel(this);
    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(serviceModel);
    proxyModel->setSortRole(Qt::DisplayRole);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_listView->setModel(proxyModel);
    m_listView->setItemDelegate(delegate);
    m_listView->setVerticalScrollMode(QListView::ScrollPerPixel);

    QLabel* remoteFileSizeLabel = new QLabel(i18nc("@label", "Skip previews for remote files above:"), this);

    m_remoteFileSizeBox = new QSpinBox(this);
    m_remoteFileSizeBox->setSingleStep(1);
    m_remoteFileSizeBox->setSuffix(QStringLiteral(" MB"));
    m_remoteFileSizeBox->setRange(0, 9999999); /* MB */

    QHBoxLayout* fileSizeBoxLayout = new QHBoxLayout();
    fileSizeBoxLayout->addWidget(remoteFileSizeLabel, 0, Qt::AlignRight);
    fileSizeBoxLayout->addWidget(m_remoteFileSizeBox);

    topLayout->addWidget(showPreviewsLabel);
    topLayout->addWidget(m_listView);
    topLayout->addLayout(fileSizeBoxLayout);

    loadSettings();

    connect(m_listView, &QListView::clicked, this, &PreviewsSettingsPage::changed);
    connect(m_remoteFileSizeBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PreviewsSettingsPage::changed);
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

    KConfigGroup globalConfig(KSharedConfig::openConfig(), QStringLiteral("PreviewSettings"));
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

    const KService::List plugins = KServiceTypeTrader::self()->query(QStringLiteral("ThumbCreator"));
    foreach (const KService::Ptr& service, plugins) {
        const bool configurable = service->property(QStringLiteral("Configurable"), QVariant::Bool).toBool();
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
    const KConfigGroup globalConfig(KSharedConfig::openConfig(), QStringLiteral("PreviewSettings"));
    m_enabledPreviewPlugins = globalConfig.readEntry("Plugins", KIO::PreviewJob::defaultPlugins());

    const qulonglong defaultRemotePreview = static_cast<qulonglong>(MaxRemotePreviewSize) * 1024 * 1024;
    const qulonglong maxRemoteByteSize = globalConfig.readEntry("MaximumRemoteSize", defaultRemotePreview);
    const int maxRemoteMByteSize = maxRemoteByteSize / (1024 * 1024);
    m_remoteFileSizeBox->setValue(maxRemoteMByteSize);
}

