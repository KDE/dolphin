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

#include "iconsviewsettingspage.h"

#include "dolphinfontrequester.h"
#include "settings/dolphinsettings.h"
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
#include <QListView>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>

#include <views/zoomlevelinfo.h>

IconsViewSettingsPage::IconsViewSettingsPage(QWidget* parent) :
    ViewSettingsPageBase(parent),
    m_iconSizeGroupBox(0),
    m_textWidthBox(0),
    m_fontRequester(0),
    m_textlinesCountBox(0),
    m_arrangementBox(0),
    m_gridSpacingBox(0)
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

    // create 'Text' group for selecting the font, the number of lines
    // and the text width
    QGroupBox* textGroup = new QGroupBox(i18nc("@title:group", "Text"), this);
    textGroup->setSizePolicy(sizePolicy);

    QLabel* fontLabel = new QLabel(i18nc("@label:listbox", "Font:"), textGroup);
    m_fontRequester = new DolphinFontRequester(textGroup);

    QLabel* textlinesCountLabel = new QLabel(i18nc("@label:textbox", "Number of lines:"), textGroup);
    m_textlinesCountBox = new KIntSpinBox(textGroup);
    m_textlinesCountBox->setMinimum(1);
    m_textlinesCountBox->setMaximum(5);

    QLabel* textWidthLabel = new QLabel(i18nc("@label:listbox", "Text width:"), textGroup);
    m_textWidthBox = new KComboBox(textGroup);
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Small"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Medium"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Large"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Huge"));

    QGridLayout* textGroupLayout = new QGridLayout(textGroup);
    textGroupLayout->addWidget(fontLabel, 0, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_fontRequester, 0, 1);
    textGroupLayout->addWidget(textlinesCountLabel, 1, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_textlinesCountBox, 1, 1);
    textGroupLayout->addWidget(textWidthLabel, 2, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_textWidthBox, 2, 1);

    // create the 'Grid' group for selecting the arrangement and the grid spacing
    QGroupBox* gridGroup = new QGroupBox(i18nc("@title:group", "Grid"), this);
    gridGroup->setSizePolicy(sizePolicy);

    QLabel* arrangementLabel = new QLabel(i18nc("@label:listbox", "Arrangement:"), gridGroup);
    m_arrangementBox = new KComboBox(gridGroup);
    m_arrangementBox->addItem(i18nc("@item:inlistbox Arrangement", "Columns"));
    m_arrangementBox->addItem(i18nc("@item:inlistbox Arrangement", "Rows"));

    QLabel* gridSpacingLabel = new QLabel(i18nc("@label:listbox", "Grid spacing:"), gridGroup);
    m_gridSpacingBox = new KComboBox(gridGroup);
    m_gridSpacingBox->addItem(i18nc("@item:inlistbox Grid spacing", "None"));
    m_gridSpacingBox->addItem(i18nc("@item:inlistbox Grid spacing", "Small"));
    m_gridSpacingBox->addItem(i18nc("@item:inlistbox Grid spacing", "Medium"));
    m_gridSpacingBox->addItem(i18nc("@item:inlistbox Grid spacing", "Large"));

    QGridLayout* gridGroupLayout = new QGridLayout(gridGroup);
    gridGroupLayout->addWidget(arrangementLabel, 0, 0, Qt::AlignRight);
    gridGroupLayout->addWidget(m_arrangementBox, 0, 1);
    gridGroupLayout->addWidget(gridSpacingLabel, 1, 0, Qt::AlignRight);
    gridGroupLayout->addWidget(m_gridSpacingBox, 1, 1);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    loadSettings();

    connect(m_iconSizeGroupBox, SIGNAL(defaultSizeChanged(int)), this, SIGNAL(changed()));
    connect(m_iconSizeGroupBox, SIGNAL(previewSizeChanged(int)), this, SIGNAL(changed()));
    connect(m_fontRequester, SIGNAL(changed()), this, SIGNAL(changed()));
    connect(m_textlinesCountBox, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(m_textWidthBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
    connect(m_arrangementBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
    connect(m_gridSpacingBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
}

IconsViewSettingsPage::~IconsViewSettingsPage()
{
}

void IconsViewSettingsPage::applySettings()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

    const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->defaultSizeValue());
    const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(m_iconSizeGroupBox->previewSizeValue());
    settings->setIconSize(iconSize);
    settings->setPreviewSize(previewSize);

    const QFont font = m_fontRequester->font();
    const int fontHeight = QFontMetrics(font).height();

    const int arrangement = (m_arrangementBox->currentIndex() == 0) ?
                            QListView::LeftToRight :
                            QListView::TopToBottom;
    settings->setArrangement(arrangement);

    const int numberOfTextlines = m_textlinesCountBox->value();

    const int defaultSize = settings->iconSize();
    int itemWidth = defaultSize;
    int itemHeight = defaultSize;
    const int textSizeIndex = m_textWidthBox->currentIndex();
    if (arrangement == QListView::TopToBottom) {
        itemWidth += TopToBottomBase + textSizeIndex * TopToBottomInc;
        itemHeight += fontHeight * numberOfTextlines + 10;
    } else {
        itemWidth += LeftToRightBase + textSizeIndex * LeftToRightInc;
    }

    settings->setItemWidth(itemWidth);
    settings->setItemHeight(itemHeight);

    settings->setUseSystemFont(m_fontRequester->mode() == DolphinFontRequester::SystemFont);
    settings->setFontFamily(font.family());
    settings->setFontSize(font.pointSizeF());
    settings->setItalicFont(font.italic());
    settings->setFontWeight(font.weight());

    settings->setNumberOfTextlines(numberOfTextlines);

    const int index = m_gridSpacingBox->currentIndex();
    if (index == 0) {
        // No grid spacing
        settings->setGridSpacing(0);
    } else {
        settings->setGridSpacing(GridSpacingBase + (index - 1) * GridSpacingInc);
    }

    settings->writeConfig();
}

void IconsViewSettingsPage::restoreDefaults()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void IconsViewSettingsPage::loadSettings()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

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
                   qRound(settings->fontSize()));
        font.setItalic(settings->italicFont());
        font.setWeight(settings->fontWeight());
        font.setPointSizeF(settings->fontSize());
        m_fontRequester->setMode(DolphinFontRequester::CustomFont);
        m_fontRequester->setCustomFont(font);
    }

    m_textlinesCountBox->setValue(settings->numberOfTextlines());

    const bool leftToRightArrangement = (settings->arrangement() == QListView::LeftToRight);
    int textWidthIndex = 0;
    const int remainingWidth = settings->itemWidth() - settings->iconSize();
    if (leftToRightArrangement) {
        textWidthIndex = (remainingWidth - LeftToRightBase) / LeftToRightInc;
    } else {
        textWidthIndex = (remainingWidth - TopToBottomBase) / TopToBottomInc;
    }
    // ensure that chosen index is always valid
    textWidthIndex = qMax(textWidthIndex, 0);
    textWidthIndex = qMin(textWidthIndex, m_textWidthBox->count() - 1);

    m_textWidthBox->setCurrentIndex(textWidthIndex);
    m_arrangementBox->setCurrentIndex(leftToRightArrangement ? 0 : 1);

    const int spacing = settings->gridSpacing();
    const int index = (spacing <= 0) ? 0 : 1 + (spacing - GridSpacingBase) / GridSpacingInc;
    m_gridSpacingBox->setCurrentIndex(index);
}

#include "iconsviewsettingspage.moc"
