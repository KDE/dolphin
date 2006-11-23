/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "iconsviewsettingspage.h"

#include <qlabel.h>
#include <qslider.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <kiconloader.h>
#include <qfontcombobox.h>
#include <kdialog.h>
#include <klocale.h>
#include <assert.h>
#include <kvbox.h>

#include "iconsmodesettings.h"
#include "dolphinsettings.h"
#include "pixmapviewer.h"

#define GRID_SPACING_BASE 8
#define GRID_SPACING_INC 12

IconsViewSettingsPage::IconsViewSettingsPage(DolphinIconsView::LayoutMode mode,
                                             QWidget* parent) :
    KVBox(parent),
    m_mode(mode),
    m_iconSizeSlider(0),
    m_previewSizeSlider(0),
    m_textWidthBox(0),
    m_gridSpacingBox(0),
    m_fontFamilyBox(0),
    m_fontSizeBox(0),
    m_textlinesCountBox(0),
    m_arrangementBox(0)
{
    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);

    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    assert(settings != 0);

    KHBox* sizesLayout = new KHBox(this);
    sizesLayout->setSpacing(spacing);
    sizesLayout->setSizePolicy(sizePolicy);

    // create 'Icon Size' group including slider and preview
    Q3GroupBox* iconSizeGroup = new Q3GroupBox(2, Qt::Vertical, i18n("Icon Size"), sizesLayout);
    iconSizeGroup->setSizePolicy(sizePolicy);
    iconSizeGroup->setMargin(margin);

    const QColor iconBackgroundColor(KGlobalSettings::baseColor());

    KHBox* iconSizeVBox = new KHBox(iconSizeGroup);
    iconSizeVBox->setSpacing(spacing);
    new QLabel(i18n("Small"), iconSizeVBox);
    m_iconSizeSlider = new QSlider(0, 5, 1, 0,  Qt::Horizontal, iconSizeVBox);
    m_iconSizeSlider->setValue(sliderValue(settings->iconSize()));
    m_iconSizeSlider->setTickmarks(QSlider::TicksBelow);
    connect(m_iconSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotIconSizeChanged(int)));
    new QLabel(i18n("Large"), iconSizeVBox);

    m_iconSizeViewer = new PixmapViewer(iconSizeGroup);
    m_iconSizeViewer->setMinimumWidth(K3Icon::SizeEnormous);
    m_iconSizeViewer->setFixedHeight(K3Icon::SizeEnormous);
    m_iconSizeViewer->setEraseColor(iconBackgroundColor);
    slotIconSizeChanged(m_iconSizeSlider->value());

    if (m_mode == DolphinIconsView::Previews) {
        // create 'Preview Size' group including slider and preview
        Q3GroupBox* previewSizeGroup = new Q3GroupBox(2, Qt::Vertical, i18n("Preview Size"), sizesLayout);
        previewSizeGroup->setSizePolicy(sizePolicy);
        previewSizeGroup->setMargin(margin);

        KHBox* previewSizeVBox = new KHBox(previewSizeGroup);
        previewSizeVBox->setSpacing(spacing);
        new QLabel(i18n("Small"), previewSizeVBox);
        m_previewSizeSlider = new QSlider(0, 5, 1, 0,  Qt::Horizontal, previewSizeVBox);
        m_previewSizeSlider->setValue(sliderValue(settings->previewSize()));
        m_previewSizeSlider->setTickmarks(QSlider::TicksBelow);
        connect(m_previewSizeSlider, SIGNAL(valueChanged(int)),
                this, SLOT(slotPreviewSizeChanged(int)));
        new QLabel(i18n("Large"), previewSizeVBox);

        m_previewSizeViewer = new PixmapViewer(previewSizeGroup);
        m_previewSizeViewer->setMinimumWidth(K3Icon::SizeEnormous);
        m_previewSizeViewer->setFixedHeight(K3Icon::SizeEnormous);
        m_previewSizeViewer->setEraseColor(iconBackgroundColor);

        slotPreviewSizeChanged(m_previewSizeSlider->value());
    }

    Q3GroupBox* textGroup = new Q3GroupBox(2, Qt::Horizontal, i18n("Text"), this);
    textGroup->setSizePolicy(sizePolicy);
    textGroup->setMargin(margin);

    new QLabel(i18n("Font family:"), textGroup);
    m_fontFamilyBox = new QFontComboBox(textGroup);
    m_fontFamilyBox->setCurrentFont(settings->fontFamily());

    new QLabel(i18n("Font size:"), textGroup);
    m_fontSizeBox = new QSpinBox(6, 99, 1, textGroup);
    m_fontSizeBox->setValue(settings->fontSize());

    new QLabel(i18n("Number of lines:"), textGroup);
    m_textlinesCountBox = new QSpinBox(1, 5, 1, textGroup);
    m_textlinesCountBox->setValue(settings->numberOfTexlines());

    new QLabel(i18n("Text width:"), textGroup);
    m_textWidthBox = new QComboBox(textGroup);
    m_textWidthBox->insertItem(i18n("Small"));
    m_textWidthBox->insertItem(i18n("Medium"));
    m_textWidthBox->insertItem(i18n("Large"));

    Q3GroupBox* gridGroup = new Q3GroupBox(2, Qt::Horizontal, i18n("Grid"), this);
    gridGroup->setSizePolicy(sizePolicy);
    gridGroup->setMargin(margin);

    const bool leftToRightArrangement = (settings->arrangement() == "LeftToRight");
    new QLabel(i18n("Arrangement:"), gridGroup);
    m_arrangementBox = new QComboBox(gridGroup);
    m_arrangementBox->insertItem(i18n("Left to right"));
    m_arrangementBox->insertItem(i18n("Top to bottom"));
    m_arrangementBox->setCurrentItem(leftToRightArrangement ? 0 : 1);

    new QLabel(i18n("Grid spacing:"), gridGroup);
    m_gridSpacingBox = new QComboBox(gridGroup);
    m_gridSpacingBox->insertItem(i18n("Small"));
    m_gridSpacingBox->insertItem(i18n("Medium"));
    m_gridSpacingBox->insertItem(i18n("Large"));
    m_gridSpacingBox->setCurrentItem((settings->gridSpacing() - GRID_SPACING_BASE) / GRID_SPACING_INC);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    adjustTextWidthSelection();
}

IconsViewSettingsPage::~IconsViewSettingsPage()
{
}

void IconsViewSettingsPage::applySettings()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    assert(settings != 0);

    const int defaultSize = iconSize(m_iconSizeSlider->value());
    settings->setIconSize(defaultSize);

    int previewSize = (m_mode == DolphinIconsView::Previews) ?
                      iconSize(m_previewSizeSlider->value()) :
                      defaultSize;
    if (previewSize < defaultSize) {
        // assure that the preview size is never smaller than the icon size
        previewSize = defaultSize;
    }
    settings->setPreviewSize(previewSize);

    const int fontSize = m_fontSizeBox->value();

    QString arrangement = (m_arrangementBox->currentItem() == 0) ?
                          "LeftToRight" :
                          "TopToBottom";
    settings->setArrangement(arrangement);
    DolphinSettings::instance().calculateGridSize(m_textWidthBox->currentItem());

    settings->setFontFamily(m_fontFamilyBox->currentFont().family());
    settings->setFontSize(fontSize);
    settings->setNumberOfTexlines(m_textlinesCountBox->value());

    settings->setGridSpacing(GRID_SPACING_BASE +
                             m_gridSpacingBox->currentItem() * GRID_SPACING_INC);
}

void IconsViewSettingsPage::slotIconSizeChanged(int value)
{
    KIconLoader iconLoader;
    m_iconSizeViewer->setPixmap(iconLoader.loadIcon("folder", K3Icon::Desktop, iconSize(value)));

    if (m_previewSizeSlider != 0) {
        int previewSizeValue = m_previewSizeSlider->value();
        if (previewSizeValue < value) {
            // assure that the preview size is never smaller than the icon size
            previewSizeValue = value;
        }
        slotPreviewSizeChanged(previewSizeValue);
    }
}

void IconsViewSettingsPage::slotPreviewSizeChanged(int value)
{
    KIconLoader iconLoader;
    const int iconSizeValue = m_iconSizeSlider->value();
    if (value < iconSizeValue) {
        // assure that the preview size is never smaller than the icon size
        value = iconSizeValue;
    }
    m_previewSizeViewer->setPixmap(iconLoader.loadIcon("preview", K3Icon::Desktop, iconSize(value)));
}

int IconsViewSettingsPage::iconSize(int sliderValue) const
{
    int size = K3Icon::SizeMedium;
    switch (sliderValue) {
        case 0: size = K3Icon::SizeSmall; break;
        case 1: size = K3Icon::SizeSmallMedium; break;
        case 2: size = K3Icon::SizeMedium; break;
        case 3: size = K3Icon::SizeLarge; break;
        case 4: size = K3Icon::SizeHuge; break;
        case 5: size = K3Icon::SizeEnormous; break;
    }
    return size;
}

int IconsViewSettingsPage::sliderValue(int iconSize) const
{
    int value = 0;
    switch (iconSize) {
        case K3Icon::SizeSmall: value = 0; break;
        case K3Icon::SizeSmallMedium: value = 1; break;
        case K3Icon::SizeMedium: value = 2; break;
        case K3Icon::SizeLarge: value = 3; break;
        case K3Icon::SizeHuge: value = 4; break;
        case K3Icon::SizeEnormous: value = 5; break;
        default: break;
    }
    return value;
}

void IconsViewSettingsPage::adjustTextWidthSelection()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    assert(settings != 0);
    m_textWidthBox->setCurrentItem(DolphinSettings::instance().textWidthHint());
}

#include "iconsviewsettingspage.moc"
