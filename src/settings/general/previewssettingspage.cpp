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

#include <settings/dolphinsettings.h>
#include <settings/serviceitemdelegate.h>
#include <settings/servicemodel.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QScrollBar>
#include <QShowEvent>
#include <QSlider>
#include <QSortFilterProxyModel>
#include <QGridLayout>
#include <QVBoxLayout>

// default settings
namespace {
    const bool UseThumbnails = true;
    const int MaxLocalPreviewSize = 5;  // 5 MB
    const int MaxRemotePreviewSize = 0; // 0 MB
}

PreviewsSettingsPage::PreviewsSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_listView(0),
    m_enabledPreviewPlugins(),
    m_localFileSizeBox(0),
    m_remoteFileSizeBox(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setSpacing(KDialog::spacingHint());
    topLayout->setMargin(KDialog::marginHint());

    // Create group box "Show previews for:"
    QGroupBox* listBox = new QGroupBox(i18nc("@title:group", "Show previews for"), this);

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

    QVBoxLayout* listBoxLayout = new QVBoxLayout(listBox);
    listBoxLayout->addWidget(m_listView);

    // Create group box "Don't create previews for"
    QGroupBox* fileSizeBox = new QGroupBox(i18nc("@title:group", "Do not create previews for"), this);

    QLabel* localFileSizeLabel = new QLabel(i18nc("@label Don't create previews for: <Local files above:> XX MByte",
                                                  "Local files above:"), this);

    m_localFileSizeBox = new KIntSpinBox(this);
    m_localFileSizeBox->setSingleStep(1);
    m_localFileSizeBox->setSuffix(QLatin1String(" MB"));
    m_localFileSizeBox->setRange(0, 9999); /* MB */

    QLabel* remoteFileSizeLabel = new QLabel(i18nc("@label Don't create previews for: <Remote files above:> XX MByte",
                                                   "Remote files above:"), this);

    m_remoteFileSizeBox = new KIntSpinBox(this);
    m_remoteFileSizeBox->setSingleStep(1);
    m_remoteFileSizeBox->setSuffix(QLatin1String(" MB"));
    m_remoteFileSizeBox->setRange(0, 9999); /* MB */

    QGridLayout* fileSizeBoxLayout = new QGridLayout(fileSizeBox);
    fileSizeBoxLayout->addWidget(localFileSizeLabel, 0, 0, Qt::AlignRight);
    fileSizeBoxLayout->addWidget(m_localFileSizeBox, 0, 1);
    fileSizeBoxLayout->addWidget(remoteFileSizeLabel, 1, 0, Qt::AlignRight);
    fileSizeBoxLayout->addWidget(m_remoteFileSizeBox, 1, 1);

    topLayout->addWidget(listBox);
    topLayout->addWidget(fileSizeBox);

    loadSettings();

    connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SIGNAL(changed()));
    connect(m_localFileSizeBox, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
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

    globalConfig.writeEntry("MaximumSize",
                            m_localFileSizeBox->value() * 1024 * 1024,
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.writeEntry("MaximumRemoteSize",
                            m_remoteFileSizeBox->value() * 1024 * 1024,
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.sync();
}

void PreviewsSettingsPage::restoreDefaults()
{
    m_localFileSizeBox->setValue(MaxLocalPreviewSize);
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
        if (service->desktopEntryName() == QLatin1String("jpegrotatedthumbnail")) {
            // Before KDE SC 4.7 thumbnail plugins where not configurable and in addition to
            // the jpegthumbnail-plugin a jpegrotatedthumbnail-plugin was offered. Make this
            // plugin obsolete for users that updated from a previous KDE SC version:
            if (show) {
                m_enabledPreviewPlugins.removeOne(service->desktopEntryName());
                KConfigGroup globalConfig(KGlobal::config(), QLatin1String("PreviewSettings"));
                globalConfig.writeEntry("Plugins", m_enabledPreviewPlugins);
                globalConfig.sync();
            }
            continue;
        }

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

    const int maxLocalByteSize = globalConfig.readEntry("MaximumSize", MaxLocalPreviewSize * 1024 * 1024);
    const int maxLocalMByteSize = maxLocalByteSize / (1024 * 1024);
    m_localFileSizeBox->setValue(maxLocalMByteSize);

    const int maxRemoteByteSize = globalConfig.readEntry("MaximumRemoteSize", MaxRemotePreviewSize * 1024 * 1024);
    const int maxRemoteMByteSize = maxRemoteByteSize / (1024 * 1024);
    m_remoteFileSizeBox->setValue(maxRemoteMByteSize);
}

#include "previewssettingspage.moc"
