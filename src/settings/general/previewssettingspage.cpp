/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "previewssettingspage.h"

#include "dolphin_generalsettings.h"
#include "settings/serviceitemdelegate.h"
#include "settings/servicemodel.h"

#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <KPluginMetaData>

#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QScroller>
#include <QShowEvent>
#include <QSortFilterProxyModel>
#include <QSpinBox>

// default settings
namespace
{
const int DefaultMaxLocalPreviewSize = 0; // 0 MB
const int DefaultMaxRemotePreviewSize = 0; // 0 MB
}

PreviewsSettingsPage::PreviewsSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_initialized(false)
    , m_listView(nullptr)
    , m_enabledPreviewPlugins()
    , m_localFileSizeBox(nullptr)
    , m_remoteFileSizeBox(nullptr)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);

    QLabel *showPreviewsLabel = new QLabel(i18nc("@title:group", "Show previews in the view for:"), this);

    m_listView = new QListView(this);
    QScroller::grabGesture(m_listView->viewport(), QScroller::TouchGesture);

    ServiceModel *serviceModel = new ServiceModel(this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(serviceModel);
    proxyModel->setSortRole(Qt::DisplayRole);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_listView->setModel(proxyModel);
    m_listView->setVerticalScrollMode(QListView::ScrollPerPixel);
    m_listView->setUniformItemSizes(true);

    QLabel *localFileSizeLabel = new QLabel(i18n("Skip previews for local files above:"), this);

    m_localFileSizeBox = new QSpinBox(this);
    m_localFileSizeBox->setSingleStep(1);
    m_localFileSizeBox->setSuffix(i18nc("Mebibytes; used as a suffix in a spinbox showing e.g. '3 MiB'", " MiB"));
    m_localFileSizeBox->setRange(0, 9999999); /* MB */
    m_localFileSizeBox->setSpecialValueText(i18n("No limit"));

    QHBoxLayout *localFileSizeBoxLayout = new QHBoxLayout();
    localFileSizeBoxLayout->addWidget(localFileSizeLabel);
    localFileSizeBoxLayout->addStretch(0);
    localFileSizeBoxLayout->addWidget(m_localFileSizeBox);

    QLabel *remoteFileSizeLabel = new QLabel(i18nc("@label", "Skip previews for remote files above:"), this);

    m_remoteFileSizeBox = new QSpinBox(this);
    m_remoteFileSizeBox->setSingleStep(1);
    m_remoteFileSizeBox->setSuffix(i18nc("Mebibytes; used as a suffix in a spinbox showing e.g. '3 MiB'", " MiB"));
    m_remoteFileSizeBox->setRange(0, 9999999); /* MB */
    m_remoteFileSizeBox->setSpecialValueText(i18n("No previews"));

    QHBoxLayout *remoteFileSizeBoxLayout = new QHBoxLayout();
    remoteFileSizeBoxLayout->addWidget(remoteFileSizeLabel);
    remoteFileSizeBoxLayout->addStretch(0);
    remoteFileSizeBoxLayout->addWidget(m_remoteFileSizeBox);

    topLayout->addWidget(showPreviewsLabel);
    topLayout->addWidget(m_listView);
    topLayout->addLayout(localFileSizeBoxLayout);
    topLayout->addLayout(remoteFileSizeBoxLayout);

    loadSettings();

    connect(m_listView, &QListView::clicked, this, &PreviewsSettingsPage::changed);
    connect(m_localFileSizeBox, &QSpinBox::valueChanged, this, &PreviewsSettingsPage::changed);
    connect(m_remoteFileSizeBox, &QSpinBox::valueChanged, this, &PreviewsSettingsPage::changed);
}

PreviewsSettingsPage::~PreviewsSettingsPage()
{
}

void PreviewsSettingsPage::applySettings()
{
    const QAbstractItemModel *model = m_listView->model();
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

    if (!m_localFileSizeBox->value()) {
        globalConfig.deleteEntry("MaximumSize", KConfigBase::Normal | KConfigBase::Global);
    } else {
        const qulonglong maximumLocalSize = static_cast<qulonglong>(m_localFileSizeBox->value()) * 1024 * 1024;
        globalConfig.writeEntry("MaximumSize", maximumLocalSize, KConfigBase::Normal | KConfigBase::Global);
    }

    const qulonglong maximumRemoteSize = static_cast<qulonglong>(m_remoteFileSizeBox->value()) * 1024 * 1024;
    globalConfig.writeEntry("MaximumRemoteSize", maximumRemoteSize, KConfigBase::Normal | KConfigBase::Global);

    globalConfig.sync();
}

void PreviewsSettingsPage::restoreDefaults()
{
    m_localFileSizeBox->setValue(DefaultMaxLocalPreviewSize);
    m_remoteFileSizeBox->setValue(DefaultMaxRemotePreviewSize);
}

void PreviewsSettingsPage::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !m_initialized) {
        loadPreviewPlugins();
        m_initialized = true;
    }
    SettingsPageBase::showEvent(event);
}

void PreviewsSettingsPage::loadPreviewPlugins()
{
    QAbstractItemModel *model = m_listView->model();

    const QVector<KPluginMetaData> plugins = KIO::PreviewJob::availableThumbnailerPlugins();
    for (const KPluginMetaData &plugin : plugins) {
        const bool show = m_enabledPreviewPlugins.contains(plugin.pluginId());

        model->insertRow(0);
        const QModelIndex index = model->index(0, 0);
        model->setData(index, show, Qt::CheckStateRole);
        model->setData(index, plugin.name(), Qt::DisplayRole);
        model->setData(index, plugin.pluginId(), ServiceModel::DesktopEntryNameRole);
    }

    model->sort(Qt::DisplayRole);
}

void PreviewsSettingsPage::loadSettings()
{
    const KConfigGroup globalConfig(KSharedConfig::openConfig(), QStringLiteral("PreviewSettings"));
    m_enabledPreviewPlugins = globalConfig.readEntry("Plugins", KIO::PreviewJob::defaultPlugins());

    const qulonglong defaultLocalPreview = static_cast<qulonglong>(DefaultMaxLocalPreviewSize) * 1024 * 1024;
    const qulonglong maxLocalByteSize = globalConfig.readEntry("MaximumSize", defaultLocalPreview);
    const int maxLocalMByteSize = maxLocalByteSize / (1024 * 1024);
    m_localFileSizeBox->setValue(maxLocalMByteSize);

    const qulonglong defaultRemotePreview = static_cast<qulonglong>(DefaultMaxRemotePreviewSize) * 1024 * 1024;
    const qulonglong maxRemoteByteSize = globalConfig.readEntry("MaximumRemoteSize", defaultRemotePreview);
    const int maxRemoteMByteSize = maxRemoteByteSize / (1024 * 1024);
    m_remoteFileSizeBox->setValue(maxRemoteMByteSize);
}

#include "moc_previewssettingspage.cpp"
