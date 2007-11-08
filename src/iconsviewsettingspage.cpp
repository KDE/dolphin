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

#include "dolphin_iconsmodesettings.h"

#include <kdialog.h>
#include <kfontrequester.h>
#include <kiconloader.h>
#include <kglobalsettings.h>
#include <klocale.h>

#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QGridLayout>
#include <QtCore/QDebug>

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

    m_iconSizeButton = new QPushButton(i18nc("@action:button", "Change Icon && Preview Size..."), this);
    connect(m_iconSizeButton, SIGNAL(clicked()),
            this, SLOT(openIconSizeDialog()));

    // create 'Text' group for selecting the font, the number of lines
    // and the text width
    QGroupBox* textGroup = new QGroupBox(i18nc("@title:group", "Text"), this);
    textGroup->setSizePolicy(sizePolicy);

    QLabel* fontLabel = new QLabel(i18nc("@label:listbox", "Font:"), textGroup);
    m_fontRequester = new KFontRequester(textGroup);

    QLabel* textlinesCountLabel = new QLabel(i18nc("@label:textbox", "Number of lines:"), textGroup);
    m_textlinesCountBox = new QSpinBox(textGroup);
    m_textlinesCountBox->setMinimum(1);
    m_textlinesCountBox->setMaximum(5);

    QLabel* textWidthLabel = new QLabel(i18nc("@label:listbox", "Text width:"), textGroup);
    m_textWidthBox = new QComboBox(textGroup);
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Small"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Medium"));
    m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Large"));

    QGridLayout* textGroupLayout = new QGridLayout(textGroup);
    textGroupLayout->addWidget(fontLabel, 0, 0);
    textGroupLayout->addWidget(m_fontRequester, 0, 1);
    textGroupLayout->addWidget(textlinesCountLabel, 1, 0);
    textGroupLayout->addWidget(m_textlinesCountBox, 1, 1);
    textGroupLayout->addWidget(textWidthLabel, 2, 0);
    textGroupLayout->addWidget(m_textWidthBox, 2, 1);

    // create the 'Grid' group for selecting the arrangement and the grid spacing
    QGroupBox* gridGroup = new QGroupBox(i18nc("@title:group", "Grid"), this);
    gridGroup->setSizePolicy(sizePolicy);

    QLabel* arrangementLabel = new QLabel(i18nc("@label:listbox", "Arrangement:"), gridGroup);
    m_arrangementBox = new QComboBox(gridGroup);
    m_arrangementBox->addItem(i18nc("@item:inlistbox Arrangement", "Left to Right"));
    m_arrangementBox->addItem(i18nc("@item:inlistbox Arrangement", "Top to Bottom"));

    QLabel* gridSpacingLabel = new QLabel(i18nc("@label:listbox", "Grid spacing:"), gridGroup);
    m_gridSpacingBox = new QComboBox(gridGroup);
    m_gridSpacingBox->addItem(i18nc("@item:inlistbox Grid spacing", "Small"));
    m_gridSpacingBox->addItem(i18nc("@item:inlistbox Grid spacing", "Medium"));
    m_gridSpacingBox->addItem(i18nc("@item:inlistbox Grid spacing", "Large"));

    QGridLayout* gridGroupLayout = new QGridLayout(gridGroup);
    gridGroupLayout->addWidget(arrangementLabel, 0, 0);
    gridGroupLayout->addWidget(m_arrangementBox, 0, 1);
    gridGroupLayout->addWidget(gridSpacingLabel, 1, 0);
    gridGroupLayout->addWidget(m_gridSpacingBox, 1, 1);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    loadSettings();
}

IconsViewSettingsPage::~IconsViewSettingsPage()
{}

void IconsViewSettingsPage::applySettings()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

    settings->setIconSize(m_iconSize);
    settings->setPreviewSize(m_previewSize);

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

    settings->setFontFamily(font.family());
    settings->setFontSize(font.pointSize());
    settings->setItalicFont(font.italic());
    settings->setBoldFont(font.bold());

    settings->setNumberOfTextlines(numberOfTextlines);

    settings->setGridSpacing(GridSpacingBase +
                             m_gridSpacingBox->currentIndex() * GridSpacingInc);
}

void IconsViewSettingsPage::restoreDefaults()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    settings->setDefaults();
    loadSettings();
}

void IconsViewSettingsPage::openIconSizeDialog()
{
    IconSizeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_iconSize = dialog.iconSize();
        m_previewSize = dialog.previewSize();
    }
}

void IconsViewSettingsPage::loadSettings()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();

    m_iconSize = settings->iconSize();
    m_previewSize = settings->previewSize();

    QFont font(settings->fontFamily(),
               settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_fontRequester->setFont(font);

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
    m_gridSpacingBox->setCurrentIndex((settings->gridSpacing() - GridSpacingBase) / GridSpacingInc);
}

#include "iconsviewsettingspage.moc"
