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

#include "dolphinsettings.h"
#include "iconsizedialog.h"
#include "pixmapviewer.h"

#include "dolphin_iconsmodesettings.h"

#include <qlabel.h>
#include <qslider.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qfontcombobox.h>

#include <kfontrequester.h>
#include <kiconloader.h>
#include <kdialog.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kvbox.h>

#include <QPushButton>
#include <QListView>

#define GRID_SPACING_BASE 8
#define GRID_SPACING_INC 24

IconsViewSettingsPage::IconsViewSettingsPage(DolphinMainWindow* mainWindow,
                                             QWidget* parent) :
    KVBox(parent),
    m_mainWindow(mainWindow),
    m_iconSize(0),
    m_previewSize(0),
    m_iconSizeButton(0),
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

    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);
    m_iconSize = settings->iconSize();
    m_previewSize = settings->previewSize();

    KHBox* sizesLayout = new KHBox(this);
    sizesLayout->setSpacing(spacing);
    sizesLayout->setSizePolicy(sizePolicy);

    m_iconSizeButton = new QPushButton(i18n("Change icon and preview size..."), this);
    connect(m_iconSizeButton, SIGNAL(clicked()),
            this, SLOT(openIconSizeDialog()));

    Q3GroupBox* textGroup = new Q3GroupBox(2, Qt::Horizontal, i18n("Text"), this);
    textGroup->setSizePolicy(sizePolicy);
    textGroup->setMargin(margin);

    new QLabel(i18n("Font:"), textGroup);
    m_fontRequester = new KFontRequester(textGroup);
    QFont font(settings->fontFamily(),
               settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_fontRequester->setFont(font);

    new QLabel(i18n("Number of lines:"), textGroup);
    m_textlinesCountBox = new QSpinBox(1, 5, 1, textGroup);
    m_textlinesCountBox->setValue(settings->numberOfTextlines());

    new QLabel(i18n("Text width:"), textGroup);
    m_textWidthBox = new QComboBox(textGroup);
    m_textWidthBox->addItem(i18n("Small"));
    m_textWidthBox->addItem(i18n("Medium"));
    m_textWidthBox->addItem(i18n("Large"));

    Q3GroupBox* gridGroup = new Q3GroupBox(2, Qt::Horizontal, i18n("Grid"), this);
    gridGroup->setSizePolicy(sizePolicy);
    gridGroup->setMargin(margin);

    const bool leftToRightArrangement = (settings->arrangement() == QListView::LeftToRight);
    new QLabel(i18n("Arrangement:"), gridGroup);
    m_arrangementBox = new QComboBox(gridGroup);
    m_arrangementBox->addItem(i18n("Left to right"));
    m_arrangementBox->addItem(i18n("Top to bottom"));
    m_arrangementBox->setCurrentIndex(leftToRightArrangement ? 0 : 1);

    new QLabel(i18n("Grid spacing:"), gridGroup);
    m_gridSpacingBox = new QComboBox(gridGroup);
    m_gridSpacingBox->addItem(i18n("Small"));
    m_gridSpacingBox->addItem(i18n("Medium"));
    m_gridSpacingBox->addItem(i18n("Large"));
    m_gridSpacingBox->setCurrentIndex((settings->gridSpacing() - GRID_SPACING_BASE) / GRID_SPACING_INC);

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
    Q_ASSERT(settings != 0);

    settings->setIconSize(m_iconSize);
    settings->setPreviewSize(m_previewSize);

    const QFont font = m_fontRequester->font();
    const int fontSize = font.pointSize();

    const int arrangement = (m_arrangementBox->currentIndex() == 0) ?
                            QListView::LeftToRight :
                            QListView::TopToBottom;

    settings->setArrangement(arrangement);

    // TODO: this is just a very rough testing code to calculate the grid
    // width and height
    const int defaultSize = settings->iconSize();
    int gridWidth = defaultSize;
    int gridHeight = defaultSize;
    const int textSizeIndex = m_textWidthBox->currentIndex();
    if (arrangement == QListView::TopToBottom) {
        gridWidth += 96 + textSizeIndex * 32;
        gridHeight += 64;
    }
    else {
        gridWidth += 128 + textSizeIndex * 64;
    }

    settings->setGridWidth(gridWidth);
    settings->setGridHeight(gridHeight);

    settings->setFontFamily(font.family());
    settings->setFontSize(fontSize);
    settings->setItalicFont(font.italic());
    settings->setBoldFont(font.bold());

    settings->setNumberOfTextlines(m_textlinesCountBox->value());

    settings->setGridSpacing(GRID_SPACING_BASE +
                             m_gridSpacingBox->currentIndex() * GRID_SPACING_INC);
}

void IconsViewSettingsPage::openIconSizeDialog()
{
    IconSizeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_iconSize = dialog.iconSize();
        m_previewSize = dialog.previewSize();
    }
}



void IconsViewSettingsPage::adjustTextWidthSelection()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);
    //m_textWidthBox->setCurrentIndex(DolphinSettings::instance().textWidthHint());
}

#include "iconsviewsettingspage.moc"
