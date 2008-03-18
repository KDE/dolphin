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

#include "dolphinfontrequester.h"

#include <kfontdialog.h>
#include <kglobalsettings.h>
#include <klocale.h>

#include <QComboBox>
#include <QEvent>
#include <QPushButton>

DolphinFontRequester::DolphinFontRequester(QWidget* parent) :
    KHBox(parent),
    m_modeCombo(0),
    m_chooseFontButton(0),
    m_mode(SystemFont),
    m_customFont()
{
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(i18nc("@item:inlistbox Font", "System Font"));
    m_modeCombo->addItem(i18nc("@item:inlistbox Font", "Custom Font"));
    connect(m_modeCombo, SIGNAL(activated(int)),
            this, SLOT(changeMode(int)));

    m_chooseFontButton = new QPushButton(i18nc("@action:button Choose font", "Choose..."), this);
    connect(m_chooseFontButton, SIGNAL(clicked()),
            this, SLOT(openFontDialog()));

    changeMode(m_modeCombo->currentIndex());
}

DolphinFontRequester::~DolphinFontRequester()
{
}

void DolphinFontRequester::setMode(Mode mode)
{
    m_mode = mode;
    m_modeCombo->setCurrentIndex(m_mode);
    m_modeCombo->setFont(font());
    m_chooseFontButton->setEnabled(m_mode == CustomFont);
}

DolphinFontRequester::Mode DolphinFontRequester::mode() const
{
    return m_mode;
}

QFont DolphinFontRequester::font() const
{
    return (m_mode == CustomFont) ? m_customFont : KGlobalSettings::generalFont();
}

void DolphinFontRequester::setCustomFont(const QFont& font)
{
    m_customFont = font;
}

QFont DolphinFontRequester::customFont() const
{
    return m_customFont;
}

bool DolphinFontRequester::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        m_modeCombo->setFont(font());
    }
    return KHBox::event(event);
}

void DolphinFontRequester::openFontDialog()
{
    QFont font;
    const int result = KFontDialog::getFont(font,
                                            KFontChooser::NoDisplayFlags,
                                            this);
    if (result == KFontDialog::Accepted) {
        m_customFont = font;
        m_modeCombo->setFont(m_customFont);
    }
}

void DolphinFontRequester::changeMode(int index)
{
    setMode((index == CustomFont) ? CustomFont : SystemFont);
}

#include "dolphinfontrequester.moc"
