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

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QBoxLayout>

#include <kconfiggroup.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <khbox.h>
#include <kvbox.h>

PreviewsSettingsPage::PreviewsSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_maxPreviewSize(0),
    m_spinBox(0),
    m_useFileThumbnails(0)
{
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(KDialog::spacingHint());
    vBox->setMargin(KDialog::marginHint());

    new QLabel("TODO: a major rewrite of this dialog will be done in 4.3", vBox);

    KHBox* hBox = new KHBox(vBox);
    hBox->setSpacing(KDialog::spacingHint());

    new QLabel(i18nc("@label:slider", "Maximum file size:"), hBox);
    m_maxPreviewSize = new QSlider(Qt::Horizontal, hBox);
    m_maxPreviewSize->setPageStep(10);
    m_maxPreviewSize->setSingleStep(1);
    m_maxPreviewSize->setTickPosition(QSlider::TicksBelow);

    m_spinBox = new QSpinBox(hBox);
    m_spinBox->setSingleStep(1);
    m_spinBox->setSuffix(" MB");

    connect(m_maxPreviewSize, SIGNAL(valueChanged(int)),
            m_spinBox, SLOT(setValue(int)));
    connect(m_spinBox, SIGNAL(valueChanged(int)),
            m_maxPreviewSize, SLOT(setValue(int)));

    connect(m_maxPreviewSize, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));
    connect(m_spinBox, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));

    m_useFileThumbnails = new QCheckBox(i18nc("@option:check", "Use thumbnails embedded in files"), vBox);
    connect(m_useFileThumbnails, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    loadSettings();
}


PreviewsSettingsPage::~PreviewsSettingsPage()
{
}

void PreviewsSettingsPage::applySettings()
{
    KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
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
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void PreviewsSettingsPage::loadSettings()
{
    const int min = 1;   // MB
    const int max = 100; // MB
    m_maxPreviewSize->setRange(min, max);

    KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    // TODO: The default value of 5 MB must match with the default value inside
    // kdelibs/kio/kio/previewjob.cpp. Maybe a static getter method in PreviewJob
    // should be added for getting the default size?
    const int maxByteSize = globalConfig.readEntry("MaximumSize", 5 * 1024 * 1024 /* 5 MB */);
    int maxMByteSize = maxByteSize / (1024 * 1024);
    if (maxMByteSize < 1) {
        maxMByteSize = 1;
    } else if (maxMByteSize > max) {
        maxMByteSize = max;
    }
    m_spinBox->setRange(min, max);

    m_maxPreviewSize->setValue(maxMByteSize);
    m_spinBox->setValue(m_maxPreviewSize->value());

    const bool useFileThumbnails = globalConfig.readEntry("UseFileThumbnails", true);
    m_useFileThumbnails->setChecked(useFileThumbnails);
}

#include "previewssettingspage.moc"
