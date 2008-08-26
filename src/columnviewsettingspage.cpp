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

#include "columnviewsettingspage.h"

#include "dolphinfontrequester.h"
#include "dolphinsettings.h"
#include "dolphin_columnmodesettings.h"
#include "iconsizegroupbox.h"
#include "zoomlevelinfo.h"

#include <kdialog.h>
#include <klocale.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QRadioButton>

ColumnViewSettingsPage::ColumnViewSettingsPage(QWidget* parent) :
    ViewSettingsPageBase(parent),
    m_iconSizeGroupBox(0),
    m_fontRequester(0),
    m_textWidthBox(0)
{
    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);
    
    // Create "Icon" properties
    m_iconSizeGroupBox = new IconSizeGroupBox(this);
    m_iconSizeGroupBox->setSizePolicy(sizePolicy);
    
    const int min = ZoomLevelInfo::minimumLevel();
    const int max = ZoomLevelInfo::maximumLevel();
    m_iconSizeGroupBox->setDefaultSizeRange(min, max);
    m_iconSizeGroupBox->setPreviewSizeRange(min, max);
    
    connect(m_iconSizeGroupBox, SIGNAL(defaultSizeChanged(int)),
            this, SIGNAL(changed()));
    connect(m_iconSizeGroupBox, SIGNAL(previewSizeChanged(int)),
            this, SIGNAL(changed()));

    // create "Text" properties
    QGroupBox* textGroup = new QGroupBox(i18nc("@title:group", "Text"), this);
    textGroup->setSizePolicy(sizePolicy);

    QLabel* fontLabel = new QLabel(i18nc("@label:listbox", "Font:"), textGroup);
    m_fontRequester = new DolphinFontRequester(textGroup);
    connect(m_fontRequester, SIGNAL(changed()), this, SIGNAL(changed()));

    QLabel* textWidthLabel = new QLabel(i18nc("@label:listbox", "Text width:"), textGroup);
    m_textWidthBox = new QComboBox(textGroup);
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Small"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Medium"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Large"));
    connect(m_textWidthBox, SIGNAL(activated(int)), this, SIGNAL(changed()));

    QGridLayout* textGroupLayout = new QGridLayout(textGroup);
    textGroupLayout->addWidget(fontLabel, 0, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_fontRequester, 0, 1);
    textGroupLayout->addWidget(textWidthLabel, 1, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_textWidthBox, 1, 1);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    loadSettings();
}

ColumnViewSettingsPage::~ColumnViewSettingsPage()
{
}

void ColumnViewSettingsPage::applySettings()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();

    const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->defaultSizeValue());
    const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->previewSizeValue());
    settings->setIconSize(iconSize);
    settings->setPreviewSize(previewSize);

    const QFont font = m_fontRequester->font();
    settings->setUseSystemFont(m_fontRequester->mode() == DolphinFontRequester::SystemFont);
    settings->setFontFamily(font.family());
    settings->setFontSize(font.pointSize());
    settings->setItalicFont(font.italic());
    settings->setFontWeight(font.weight());

    // TODO:
    //const int columnWidth = 150 + (m_columnWidthSlider->value() * 50);
    //settings->setColumnWidth(columnWidth);
}

void ColumnViewSettingsPage::restoreDefaults()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    settings->setDefaults();
    loadSettings();
}

void ColumnViewSettingsPage::loadSettings()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();

    const QSize iconSize(settings->iconSize(), settings->iconSize());
    const int iconSizeValue = ZoomLevelInfo::zoomLevelForIconSize(iconSize);
    m_iconSizeGroupBox->setDefaultSizeValue(iconSizeValue);
    
    const QSize previewSize(settings->previewSize(), settings->previewSize());
    const int previewSizeValue = ZoomLevelInfo::zoomLevelForIconSize(previewSize);
    m_iconSizeGroupBox->setPreviewSizeValue(previewSizeValue);

    if (settings->useSystemFont()) {
        m_fontRequester->setMode(DolphinFontRequester::SystemFont);
    } else {
        QFont font(settings->fontFamily(),
                   settings->fontSize());
        font.setItalic(settings->italicFont());
        font.setWeight(settings->fontWeight());
        m_fontRequester->setMode(DolphinFontRequester::CustomFont);
        m_fontRequester->setCustomFont(font);
    }

    // TODO:
    //m_columnWidthSlider->setValue((settings->columnWidth() - 150) / 50);
}

#include "columnviewsettingspage.moc"
