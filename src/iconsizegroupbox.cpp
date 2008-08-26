/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "iconsizegroupbox.h"

#include <klocale.h>

#include <QGridLayout>
#include <QLabel>
#include <QSlider>

IconSizeGroupBox::IconSizeGroupBox(QWidget* parent) :
    QGroupBox(i18nc("@title:group", "Icon Size"), parent),
    m_defaultSizeSlider(0),
    m_previewSizeSlider(0)
{
    QLabel* defaultLabel = new QLabel(i18nc("@label:listbox", "Default:"), this);
    m_defaultSizeSlider = new QSlider(Qt::Horizontal, this);
    m_defaultSizeSlider->setPageStep(1);
    m_defaultSizeSlider->setTickPosition(QSlider::TicksBelow);
    connect(m_defaultSizeSlider, SIGNAL(sliderMoved(int)),
            this, SIGNAL(defaultSizeChanged(int)));
    
    QLabel* previewLabel = new QLabel(i18nc("@label:listbox", "Preview:"), this);
    m_previewSizeSlider = new QSlider(Qt::Horizontal, this);
    m_previewSizeSlider->setPageStep(1);
    m_previewSizeSlider->setTickPosition(QSlider::TicksBelow);
    connect(m_previewSizeSlider, SIGNAL(sliderMoved(int)),
            this, SIGNAL(defaultSizeChanged(int)));
    
    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(defaultLabel, 0, 0);
    layout->addWidget(m_defaultSizeSlider, 0, 1);
    layout->addWidget(previewLabel, 1, 0);
    layout->addWidget(m_previewSizeSlider, 1, 1);
}

IconSizeGroupBox::~IconSizeGroupBox()
{
}

void IconSizeGroupBox::setDefaultSizeRange(int min, int max)
{
    m_defaultSizeSlider->setRange(min, max);
}

void IconSizeGroupBox::setPreviewSizeRange(int min, int max)
{
    m_previewSizeSlider->setRange(min, max);
}

void IconSizeGroupBox::setDefaultSizeValue(int value)
{
    m_defaultSizeSlider->setValue(value);
}

int IconSizeGroupBox::defaultSizeValue() const
{
    return m_defaultSizeSlider->value();
}

void IconSizeGroupBox::setPreviewSizeValue(int value)
{
    m_previewSizeSlider->setValue(value);
}

int IconSizeGroupBox::previewSizeValue() const
{
    return m_previewSizeSlider->value();
}

#include "iconsizegroupbox.moc"
