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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "iconsizedialog.h"

#include "dolphinsettings.h"
#include "pixmapviewer.h"

#include "dolphin_iconsmodesettings.h"

#include <kcolorscheme.h>
#include <klocale.h>
#include <kvbox.h>

#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QBoxLayout>

IconSizeDialog::IconSizeDialog(QWidget* parent) :
    KDialog(parent),
    m_iconSize(0),
    m_previewSize(0),
    m_iconSizeSlider(0),
    m_iconSizeViewer(0),
    m_previewSizeSlider(0),
    m_previewSizeViewer(0),
    m_iconLoader()
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    Q_ASSERT(settings != 0);
    m_iconSize = settings->iconSize();
    m_previewSize = settings->previewSize();

    const int spacing = KDialog::spacingHint();

    setCaption(i18nc("@title:window", "Change Icon & Preview Size"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    QWidget* main = new QWidget();
    QHBoxLayout* topLayout = new QHBoxLayout();

    // create 'Icon Size' group including slider and preview
    QGroupBox* iconSizeBox = new QGroupBox(i18nc("@title:group", "Icon Size"), main);

    const QColor iconBackgroundColor = KColorScheme(QPalette::Active, KColorScheme::View).background().color();

    KHBox* iconSizeHBox = new KHBox(iconSizeBox);
    iconSizeHBox->setSpacing(spacing);
    new QLabel(i18nc("@item:inrange Icon Size", "Small"), iconSizeHBox);
    m_iconSizeSlider = new QSlider(0, 5, 1, 0,  Qt::Horizontal, iconSizeHBox);
    m_iconSizeSlider->setValue(sliderValue(settings->iconSize()));
    m_iconSizeSlider->setTickmarks(QSlider::TicksBelow);
    connect(m_iconSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(updateIconSize(int)));
    new QLabel(i18nc("@item:inrange Icon Size", "Large"), iconSizeHBox);

    m_iconSizeViewer = new PixmapViewer(iconSizeBox, PixmapViewer::SizeTransition);
    m_iconSizeViewer->setMinimumWidth(K3Icon::SizeEnormous);
    m_iconSizeViewer->setFixedHeight(K3Icon::SizeEnormous);
    m_iconSizeViewer->setEraseColor(iconBackgroundColor);
    updateIconSize(m_iconSizeSlider->value());

    QVBoxLayout* iconSizeLayout = new QVBoxLayout(iconSizeBox);
    iconSizeLayout->addWidget(iconSizeHBox);
    iconSizeLayout->addWidget(m_iconSizeViewer);

    // create 'Preview Size' group including slider and preview
    QGroupBox* previewSizeBox = new QGroupBox(i18nc("@title:group", "Preview Size"), main);

    KHBox* previewSizeHBox = new KHBox(previewSizeBox);
    previewSizeHBox->setSpacing(spacing);
    new QLabel(i18nc("@item:inrange Preview Size", "Small"), previewSizeHBox);
    m_previewSizeSlider = new QSlider(0, 5, 1, 0,  Qt::Horizontal, previewSizeHBox);
    m_previewSizeSlider->setValue(sliderValue(settings->previewSize()));
    m_previewSizeSlider->setTickmarks(QSlider::TicksBelow);
    connect(m_previewSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(updatePreviewSize(int)));
    new QLabel(i18nc("@item:inrange Preview Size", "Large"), previewSizeHBox);

    m_previewSizeViewer = new PixmapViewer(previewSizeBox, PixmapViewer::SizeTransition);
    m_previewSizeViewer->setMinimumWidth(K3Icon::SizeEnormous);
    m_previewSizeViewer->setFixedHeight(K3Icon::SizeEnormous);
    m_previewSizeViewer->setEraseColor(iconBackgroundColor);
    updatePreviewSize(m_previewSizeSlider->value());

    QVBoxLayout* previewSizeLayout = new QVBoxLayout(previewSizeBox);
    previewSizeLayout->addWidget(previewSizeHBox);
    previewSizeLayout->addWidget(m_previewSizeViewer);

    topLayout->addWidget(iconSizeBox);
    topLayout->addWidget(previewSizeBox);
    main->setLayout(topLayout);
    setMainWidget(main);
}

IconSizeDialog::~IconSizeDialog()
{
}

void IconSizeDialog::slotButtonClicked(int button)
{
    if (button == Ok) {
        m_iconSize = iconSize(m_iconSizeSlider->value());

        m_previewSize = iconSize(m_previewSizeSlider->value());
        if (m_previewSize < m_iconSize) {
            // assure that the preview size is never smaller than the icon size
            m_previewSize = m_iconSize;
        }
    }

    KDialog::slotButtonClicked(button);
}

void IconSizeDialog::updateIconSize(int value)
{
    m_iconSizeViewer->setPixmap(m_iconLoader.loadIcon("folder", K3Icon::Desktop, iconSize(value)));
    if (m_previewSizeSlider != 0) {
        updatePreviewSize(m_previewSizeSlider->value());
    }
}

void IconSizeDialog::updatePreviewSize(int value)
{
    const int iconSizeValue = m_iconSizeSlider->value();
    if (value < iconSizeValue) {
        // assure that the preview size is never smaller than the icon size
        value = iconSizeValue;
    }
    m_previewSizeViewer->setPixmap(m_iconLoader.loadIcon("preview", K3Icon::Desktop, iconSize(value)));
}

int IconSizeDialog::iconSize(int sliderValue) const
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

int IconSizeDialog::sliderValue(int iconSize) const
{
    int value = 0;
    switch (iconSize) {
    case K3Icon::SizeSmall:       value = 0; break;
    case K3Icon::SizeSmallMedium: value = 1; break;
    case K3Icon::SizeMedium:      value = 2; break;
    case K3Icon::SizeLarge:       value = 3; break;
    case K3Icon::SizeHuge:        value = 4; break;
    case K3Icon::SizeEnormous:    value = 5; break;
    default: break;
    }
    return value;
}

#include "iconsizedialog.moc"
