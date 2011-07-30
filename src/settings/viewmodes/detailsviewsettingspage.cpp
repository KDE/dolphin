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

#include "detailsviewsettingspage.h"

#include "iconsizegroupbox.h"
#include "dolphinfontrequester.h"
#include "dolphin_detailsmodesettings.h"

#include <KDialog>
#include <KLocale>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>

#include <views/zoomlevelinfo.h>

DetailsViewSettingsPage::DetailsViewSettingsPage(QWidget* parent) :
    ViewSettingsPageBase(parent),
    m_iconSizeGroupBox(0),
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

    // create "Text" properties
    QWidget* textGroup = new QGroupBox(i18nc("@title:group", "Text"), this);
    textGroup->setSizePolicy(sizePolicy);

    QLabel* fontLabel = new QLabel(i18nc("@label:listbox", "Font:"), textGroup);
    m_fontRequester = new DolphinFontRequester(textGroup);

    QHBoxLayout* textLayout = new QHBoxLayout(textGroup);
    textLayout->addWidget(fontLabel, 0, Qt::AlignRight);
    textLayout->addWidget(m_fontRequester);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    loadSettings();

    connect(m_iconSizeGroupBox, SIGNAL(defaultSizeChanged(int)), this, SIGNAL(changed()));
    connect(m_iconSizeGroupBox, SIGNAL(previewSizeChanged(int)), this, SIGNAL(changed()));
    connect(m_fontRequester, SIGNAL(changed()), this, SIGNAL(changed()));
}

DetailsViewSettingsPage::~DetailsViewSettingsPage()
{
}

void DetailsViewSettingsPage::applySettings()
{
    const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->defaultSizeValue());
    const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->previewSizeValue());
    DetailsModeSettings::setIconSize(iconSize);
    DetailsModeSettings::setPreviewSize(previewSize);

    const QFont font = m_fontRequester->font();
    DetailsModeSettings::setUseSystemFont(m_fontRequester->mode() == DolphinFontRequester::SystemFont);
    DetailsModeSettings::setFontFamily(font.family());
    DetailsModeSettings::setFontSize(font.pointSizeF());
    DetailsModeSettings::setItalicFont(font.italic());
    DetailsModeSettings::setFontWeight(font.weight());

    DetailsModeSettings::self()->writeConfig();
}

void DetailsViewSettingsPage::restoreDefaults()
{
    DetailsModeSettings::self()->useDefaults(true);
    loadSettings();
    DetailsModeSettings::self()->useDefaults(false);
}

void DetailsViewSettingsPage::loadSettings()
{
    const QSize iconSize(DetailsModeSettings::iconSize(), DetailsModeSettings::iconSize());
    const int iconSizeValue = ZoomLevelInfo::zoomLevelForIconSize(iconSize);
    m_iconSizeGroupBox->setDefaultSizeValue(iconSizeValue);

    const QSize previewSize(DetailsModeSettings::previewSize(), DetailsModeSettings::previewSize());
    const int previewSizeValue = ZoomLevelInfo::zoomLevelForIconSize(previewSize);
    m_iconSizeGroupBox->setPreviewSizeValue(previewSizeValue);

    if (DetailsModeSettings::useSystemFont()) {
        m_fontRequester->setMode(DolphinFontRequester::SystemFont);
    } else {
        QFont font(DetailsModeSettings::fontFamily(),
                   qRound(DetailsModeSettings::fontSize()));
        font.setItalic(DetailsModeSettings::italicFont());
        font.setWeight(DetailsModeSettings::fontWeight());
        font.setPointSizeF(DetailsModeSettings::fontSize());
        m_fontRequester->setMode(DolphinFontRequester::CustomFont);
        m_fontRequester->setCustomFont(font);
    }
}

#include "detailsviewsettingspage.moc"
