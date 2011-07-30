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

#include "iconsviewsettingspage.h"

#include "dolphinfontrequester.h"
#include "iconsizegroupbox.h"

#include "dolphin_iconsmodesettings.h"

#include <KDialog>
#include <KIconLoader>
#include <KGlobalSettings>
#include <KLocale>
#include <KComboBox>
#include <KNumInput>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>

#include <views/zoomlevelinfo.h>

IconsViewSettingsPage::IconsViewSettingsPage(QWidget* parent) :
    ViewSettingsPageBase(parent),
    m_iconSizeGroupBox(0),
    m_textWidthBox(0),
    m_fontRequester(0)
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

    // Create 'Text' group for selecting the font, the number of lines
    // and the text width
    QGroupBox* textGroup = new QGroupBox(i18nc("@title:group", "Text"), this);
    textGroup->setSizePolicy(sizePolicy);

    QLabel* fontLabel = new QLabel(i18nc("@label:listbox", "Font:"), textGroup);
    m_fontRequester = new DolphinFontRequester(textGroup);

    QLabel* textWidthLabel = new QLabel(i18nc("@label:listbox", "Text width:"), textGroup);
    m_textWidthBox = new KComboBox(textGroup);
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Small"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Medium"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Large"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Huge"));

    QGridLayout* textGroupLayout = new QGridLayout(textGroup);
    textGroupLayout->addWidget(fontLabel, 0, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_fontRequester, 0, 1);
    textGroupLayout->addWidget(textWidthLabel, 2, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_textWidthBox, 2, 1);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    loadSettings();

    connect(m_iconSizeGroupBox, SIGNAL(defaultSizeChanged(int)), this, SIGNAL(changed()));
    connect(m_iconSizeGroupBox, SIGNAL(previewSizeChanged(int)), this, SIGNAL(changed()));
    connect(m_fontRequester, SIGNAL(changed()), this, SIGNAL(changed()));
    connect(m_textWidthBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
}

IconsViewSettingsPage::~IconsViewSettingsPage()
{
}

void IconsViewSettingsPage::applySettings()
{
    const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->defaultSizeValue());
    const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->previewSizeValue());
    IconsModeSettings::setIconSize(iconSize);
    IconsModeSettings::setPreviewSize(previewSize);

    const QFont font = m_fontRequester->font();
    IconsModeSettings::setUseSystemFont(m_fontRequester->mode() == DolphinFontRequester::SystemFont);
    IconsModeSettings::setFontFamily(font.family());
    IconsModeSettings::setFontSize(font.pointSizeF());
    IconsModeSettings::setItalicFont(font.italic());
    IconsModeSettings::setFontWeight(font.weight());

    IconsModeSettings::setTextWidthIndex(m_textWidthBox->currentIndex());

    IconsModeSettings::self()->writeConfig();
}

void IconsViewSettingsPage::restoreDefaults()
{
    IconsModeSettings::self()->useDefaults(true);
    loadSettings();
    IconsModeSettings::self()->useDefaults(false);
}

void IconsViewSettingsPage::loadSettings()
{
    const QSize iconSize(IconsModeSettings::iconSize(), IconsModeSettings::iconSize());
    const int iconSizeValue = ZoomLevelInfo::zoomLevelForIconSize(iconSize);
    m_iconSizeGroupBox->setDefaultSizeValue(iconSizeValue);

    const QSize previewSize(IconsModeSettings::previewSize(), IconsModeSettings::previewSize());
    const int previewSizeValue = ZoomLevelInfo::zoomLevelForIconSize(previewSize);
    m_iconSizeGroupBox->setPreviewSizeValue(previewSizeValue);

    if (IconsModeSettings::useSystemFont()) {
        m_fontRequester->setMode(DolphinFontRequester::SystemFont);
    } else {
        QFont font(IconsModeSettings::fontFamily(),
                   qRound(IconsModeSettings::fontSize()));
        font.setItalic(IconsModeSettings::italicFont());
        font.setWeight(IconsModeSettings::fontWeight());
        font.setPointSizeF(IconsModeSettings::fontSize());
        m_fontRequester->setMode(DolphinFontRequester::CustomFont);
        m_fontRequester->setCustomFont(font);
    }

    m_textWidthBox->setCurrentIndex(IconsModeSettings::textWidthIndex());
}

#include "iconsviewsettingspage.moc"
