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
#include <khbox.h>
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
#include <QBoxLayout>

// default settings
const bool USE_THUMBNAILS = true;
const int MAX_PREVIEW_SIZE = 5; // 5 MB

PreviewsSettingsPage::PreviewsSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_previewPluginsList(0),
    m_enabledPreviewPlugins(),
    m_maxPreviewSize(0),
    m_spinBox(0),
    m_useFileThumbnails(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setSpacing(KDialog::spacingHint());
    topLayout->setMargin(KDialog::marginHint());

    QLabel* listDescription = new QLabel(i18nc("@label", "Show previews for:"), this);

    m_previewPluginsList = new QListWidget(this);
    m_previewPluginsList->setSortingEnabled(true);
    m_previewPluginsList->setSelectionMode(QAbstractItemView::NoSelection);
    connect(m_previewPluginsList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SIGNAL(changed()));

    KHBox* hBox = new KHBox(this);
    hBox->setSpacing(KDialog::spacingHint());

    new QLabel(i18nc("@label:slider", "Maximum file size:"), hBox);
    m_maxPreviewSize = new QSlider(Qt::Horizontal, hBox);
    m_maxPreviewSize->setPageStep(10);
    m_maxPreviewSize->setSingleStep(1);
    m_maxPreviewSize->setTickPosition(QSlider::TicksBelow);
    m_maxPreviewSize->setRange(1, 100); /* MB */

    m_spinBox = new KIntSpinBox(hBox);
    m_spinBox->setSingleStep(1);
    m_spinBox->setSuffix(" MB");
    m_spinBox->setRange(1, 100); /* MB */

    connect(m_maxPreviewSize, SIGNAL(valueChanged(int)),
            m_spinBox, SLOT(setValue(int)));
    connect(m_spinBox, SIGNAL(valueChanged(int)),
            m_maxPreviewSize, SLOT(setValue(int)));

    connect(m_maxPreviewSize, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));
    connect(m_spinBox, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));

    m_useFileThumbnails = new QCheckBox(i18nc("@option:check", "Use thumbnails embedded in files"), this);
    connect(m_useFileThumbnails, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    topLayout->addWidget(listDescription);
    topLayout->addWidget(m_previewPluginsList);
    topLayout->addWidget(hBox);
    topLayout->addWidget(m_useFileThumbnails);

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

    KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    globalConfig.writeEntry("Plugins", m_enabledPreviewPlugins);

    const int byteCount = m_maxPreviewSize->value() * 1024 * 1024; // value() returns size in MB
    globalConfig.writeEntry("MaximumSize",
                            byteCount,
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.writeEntry("UseFileThumbnails",
                            m_useFileThumbnails->isChecked(),
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.sync();
}

void PreviewsSettingsPage::restoreDefaults()
{
    m_maxPreviewSize->setValue(MAX_PREVIEW_SIZE);
    m_useFileThumbnails->setChecked(USE_THUMBNAILS);
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
    const KService::List plugins = KServiceTypeTrader::self()->query("ThumbCreator");
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
                                                                << "directorythumbnail"
                                                                << "imagethumbnail"
                                                                << "jpegthumbnail");

    // TODO: The default value of 5 MB must match with the default value inside
    // kdelibs/kio/kio/previewjob.cpp. Maybe a static getter method in PreviewJob
    // should be added for getting the default size?
    const int min = 1;   // MB
    const int max = 100; // MB

    const int maxByteSize = globalConfig.readEntry("MaximumSize", MAX_PREVIEW_SIZE * 1024 * 1024);
    int maxMByteSize = maxByteSize / (1024 * 1024);
    if (maxMByteSize < min) {
        maxMByteSize = min;
    } else if (maxMByteSize > max) {
        maxMByteSize = max;
    }
    m_maxPreviewSize->setValue(maxMByteSize);

    const bool useFileThumbnails = globalConfig.readEntry("UseFileThumbnails", USE_THUMBNAILS);
    m_useFileThumbnails->setChecked(useFileThumbnails);
}

#include "previewssettingspage.moc"
