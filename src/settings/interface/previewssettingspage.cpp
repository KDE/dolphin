/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "previewssettingspage.h"

#include "dolphin_generalsettings.h"
#include "settings/servicemodel.h"

#include <KContextualHelpButton>
#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <KPluginMetaData>

#include <QCheckBox>
#include <QFormLayout>
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
const bool EnableRemoteFolderThumbnail = false;
}

PreviewsSettingsPage::PreviewsSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_initialized(false)
    , m_listView(nullptr)
    , m_enabledPreviewPlugins()
    , m_localFileSizeBox(nullptr)
    , m_remoteFileSizeBox(nullptr)
    , m_enableRemoteFolderThumbnail(nullptr)
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

    // i18n: This label forms a full sentence together with the spinbox content.
    // Depending on the option chosen in the spinbox, it reads "Show previews for [files below n MiB]"
    // or "Show previews for [files of any size]".
    QLabel *localFileSizeLabel = new QLabel(i18nc("@label:spinbox", "Show previews for"), this);

    m_localFileSizeBox = new QSpinBox(this);
    m_localFileSizeBox->setSingleStep(1);
    m_localFileSizeBox->setPrefix(i18nc("used as a prefix in a spinbox showing e.g. 'Show previews for [files below 3 MiB]'", "files below "));
    m_localFileSizeBox->setSuffix(i18nc("Mebibytes; used as a suffix in a spinbox showing e.g. '3 MiB'", " MiB"));
    m_localFileSizeBox->setRange(0, 9999999); /* MB */
    m_localFileSizeBox->setSpecialValueText(i18nc("e.g. 'Show previews for [files of any size]'", "files of any size"));

    QHBoxLayout *localFileSizeBoxLayout = new QHBoxLayout();
    localFileSizeBoxLayout->addWidget(localFileSizeLabel);
    localFileSizeBoxLayout->addWidget(m_localFileSizeBox);

    QLabel *remoteFileSizeLabel = new QLabel(i18nc("@label:spinbox", "Show previews for"), this);

    m_remoteFileSizeBox = new QSpinBox(this);
    m_remoteFileSizeBox->setSingleStep(1);
    m_remoteFileSizeBox->setPrefix(i18nc("used as a prefix in a spinbox showing e.g. 'Show previews for [files below 3 MiB]'", "files below "));
    m_remoteFileSizeBox->setSuffix(i18nc("Mebibytes; used as a suffix in a spinbox showing e.g. '3 MiB'", " MiB"));
    m_remoteFileSizeBox->setRange(0, 9999999); /* MB */
    m_remoteFileSizeBox->setSpecialValueText(i18nc("e.g. 'Show previews for [no file]'", "no file"));

    QHBoxLayout *remoteFileSizeBoxLayout = new QHBoxLayout();
    remoteFileSizeBoxLayout->addWidget(remoteFileSizeLabel);
    remoteFileSizeBoxLayout->addWidget(m_remoteFileSizeBox);

    // Enable remote folder thumbnail option
    m_enableRemoteFolderThumbnail = new QCheckBox(i18nc("@option:check", "Show previews for folders"), this);

    // Make the "Enable preview for remote folder" enabled only when "Remote file limit" is superior to 0
    m_enableRemoteFolderThumbnail->setEnabled(m_remoteFileSizeBox->value() > 0);
    connect(m_remoteFileSizeBox, &QSpinBox::valueChanged, this, [this](int i) {
        m_enableRemoteFolderThumbnail->setEnabled(i > 0);
    });

    const auto helpButtonInfo = xi18nc("@info",
                                       "<para>Creating <emphasis>previews</emphasis> for remote folders is "
                                       "very intensive in terms of network resource usage.</para>"
                                       "<para>Disable this if navigating remote folders in Dolphin "
                                       "is slow or when accessing storage over metered connections.</para>");
    auto contextualHelpButton = new KContextualHelpButton{helpButtonInfo, m_enableRemoteFolderThumbnail, this};

    QHBoxLayout *enableRemoteFolderThumbnailLayout = new QHBoxLayout();
    enableRemoteFolderThumbnailLayout->addWidget(m_enableRemoteFolderThumbnail);
    enableRemoteFolderThumbnailLayout->addWidget(contextualHelpButton);

    QFormLayout *formLayout = new QFormLayout(this);

    QLabel *localGroupLabel = new QLabel(i18nc("@title:group", "Local storage:"));
    // Makes sure it has the same height as the labeled sizeBoxLayout
    localGroupLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    formLayout->addRow(localGroupLabel, localFileSizeBoxLayout);

    QLabel *remoteGroupLabel = new QLabel(i18nc("@title:group", "Remote storage:"));
    remoteGroupLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    formLayout->addRow(remoteGroupLabel, remoteFileSizeBoxLayout);

    formLayout->addRow(QString(), enableRemoteFolderThumbnailLayout);

    topLayout->addWidget(showPreviewsLabel);
    topLayout->addWidget(m_listView);
    topLayout->addLayout(formLayout);

    // So that m_listView takes up all available space
    topLayout->setStretchFactor(m_listView, 1);

    loadSettings();

    connect(m_listView, &QListView::clicked, this, &PreviewsSettingsPage::changed);
    connect(m_localFileSizeBox, &QSpinBox::valueChanged, this, &PreviewsSettingsPage::changed);
    connect(m_remoteFileSizeBox, &QSpinBox::valueChanged, this, &PreviewsSettingsPage::changed);
    connect(m_enableRemoteFolderThumbnail, &QCheckBox::toggled, this, &PreviewsSettingsPage::changed);
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
            const bool checked = model->data(index, Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
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

    globalConfig.writeEntry("EnableRemoteFolderThumbnail", m_enableRemoteFolderThumbnail->isChecked(), KConfigBase::Normal | KConfigBase::Global);

    globalConfig.sync();
}

void PreviewsSettingsPage::restoreDefaults()
{
    m_localFileSizeBox->setValue(DefaultMaxLocalPreviewSize);
    m_remoteFileSizeBox->setValue(DefaultMaxRemotePreviewSize);
    m_enableRemoteFolderThumbnail->setChecked(EnableRemoteFolderThumbnail);
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
        model->setData(index, show ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
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

    m_enableRemoteFolderThumbnail->setChecked(globalConfig.readEntry("EnableRemoteFolderThumbnail", EnableRemoteFolderThumbnail));
}

#include "moc_previewssettingspage.cpp"
