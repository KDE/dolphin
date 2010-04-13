/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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
#include "dolphinsettings.h"

#include "dolphin_generalsettings.h"

#include <kconfiggroup.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <KNumInput>
#include <kservicetypetrader.h>
#include <kservice.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QRadioButton>
#include <QShowEvent>
#include <QSlider>
#include <QGridLayout>

#include <kdebug.h>

// default settings
namespace {
    const bool UseThumbnails = true;
    const int MaxLocalPreviewSize = 5;  // 5 MB
    const int MaxRemotePreviewSize = 0; // 0 MB
}

PreviewsSettingsPage::PreviewsSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_previewPluginsList(0),
    m_enabledPreviewPlugins(),
    m_localFileSizeBox(0),
    m_remoteFileSizeBox(0)
{
    kDebug() << "--------- constructing!";
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setSpacing(KDialog::spacingHint());
    topLayout->setMargin(KDialog::marginHint());

    QLabel* listDescription = new QLabel(i18nc("@label", "Show previews for:"), this);

    m_previewPluginsList = new QListWidget(this);
    m_previewPluginsList->setSortingEnabled(true);
    m_previewPluginsList->setSelectionMode(QAbstractItemView::NoSelection);
    connect(m_previewPluginsList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SIGNAL(changed()));

    QLabel* localFileSizeLabel = new QLabel(i18nc("@label", "Local file size maximum:"), this);

    m_localFileSizeBox = new KIntSpinBox(this);
    m_localFileSizeBox->setSingleStep(1);
    m_localFileSizeBox->setSuffix(QLatin1String(" MB"));
    m_localFileSizeBox->setRange(0, 9999); /* MB */
    connect(m_localFileSizeBox, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));

    QLabel* remoteFileSizeLabel = new QLabel(i18nc("@label", "Remote file size maximum:"), this);
    m_remoteFileSizeBox = new KIntSpinBox(this);
    m_remoteFileSizeBox->setSingleStep(1);
    m_remoteFileSizeBox->setSuffix(QLatin1String(" MB"));
    m_remoteFileSizeBox->setRange(0, 9999); /* MB */
    connect(m_remoteFileSizeBox, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(localFileSizeLabel, 0, 0);
    gridLayout->addWidget(m_localFileSizeBox, 0, 1);
    gridLayout->addWidget(remoteFileSizeLabel, 1, 0);
    gridLayout->addWidget(m_remoteFileSizeBox, 1, 1);

    topLayout->addWidget(listDescription);
    topLayout->addWidget(m_previewPluginsList);
    topLayout->addLayout(gridLayout);

    loadSettings();
}


PreviewsSettingsPage::~PreviewsSettingsPage()
{
}

void PreviewsSettingsPage::applySettings()
{
    m_enabledPreviewPlugins.clear();
    const int count = m_previewPluginsList->count();
    for (int i = 0; i < count; ++i) {
        const QListWidgetItem* item = m_previewPluginsList->item(i);
        if (item->checkState() == Qt::Checked) {
            const QString enabledPlugin = item->data(Qt::UserRole).toString();
            m_enabledPreviewPlugins.append(enabledPlugin);
        }
    }

    KConfigGroup globalConfig(KGlobal::config(), QLatin1String("PreviewSettings"));
    globalConfig.writeEntry("Plugins", m_enabledPreviewPlugins);

    globalConfig.writeEntry("MaximumSize",
                            m_localFileSizeBox->value() * 1024 * 1024,
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.writeEntry("RemoteMaximumSize",
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
        QMetaObject::invokeMethod(this, "loadPreviewPlugins", Qt::QueuedConnection);
        m_initialized = true;
    }
    SettingsPageBase::showEvent(event);
}

void PreviewsSettingsPage::loadPreviewPlugins()
{
    const KService::List plugins = KServiceTypeTrader::self()->query(QLatin1String("ThumbCreator"));
    foreach (const KSharedPtr<KService>& service, plugins) {
        QListWidgetItem* item = new QListWidgetItem(service->name(),
                                                    m_previewPluginsList);
        item->setData(Qt::UserRole, service->desktopEntryName());
        const bool show = m_enabledPreviewPlugins.contains(service->desktopEntryName());
        item->setCheckState(show ? Qt::Checked : Qt::Unchecked);
    }
}

void PreviewsSettingsPage::loadSettings()
{
    KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    m_enabledPreviewPlugins = globalConfig.readEntry("Plugins", QStringList()
                                                     << QLatin1String("directorythumbnail")
                                                     << QLatin1String("imagethumbnail")
                                                     << QLatin1String("jpegthumbnail"));
    kDebug() << "----------------- enabled plugins:";
    foreach (const QString& plugin, m_enabledPreviewPlugins) {
        kDebug() << plugin;
    }

    // TODO: The default value of 5 MB must match with the default value inside
    // kdelibs/kio/kio/previewjob.cpp. Maybe a static getter method in PreviewJob
    // should be added for getting the default size?
    const int maxLocalByteSize = globalConfig.readEntry("MaximumSize", MaxLocalPreviewSize * 1024 * 1024);
    const int maxLocalMByteSize = maxLocalByteSize / (1024 * 1024);
    m_localFileSizeBox->setValue(maxLocalMByteSize);

    const int maxRemoteByteSize = globalConfig.readEntry("MaximumSize", MaxRemotePreviewSize * 1024 * 1024);
    const int maxRemoteMByteSize = maxRemoteByteSize / (1024 * 1024);
    m_localFileSizeBox->setValue(maxRemoteMByteSize);
}

#include "previewssettingspage.moc"
