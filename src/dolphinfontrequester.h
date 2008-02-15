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

#ifndef DOLPHINFONTREQUESTER_H
#define DOLPHINFONTREQUESTER_H

#include <khbox.h>

#include <QFont>

class QComboBox;
class QPushButton;

/**
 * @brief Allows to select between using the system font or a custom font.
 */
class DolphinFontRequester : public KHBox
{
    Q_OBJECT

public:
    enum Mode
    {
        SystemFont = 0,
        CustomFont = 1
    };

    DolphinFontRequester(QWidget* parent);
    virtual ~DolphinFontRequester();

    void setMode(Mode mode);
    Mode mode() const;

    /**
     * Returns the custom font (see DolphinFontRequester::customFont()),
     * if the mode is \a CustomFont, otherwise the system font is
     * returned.
     */
    QFont font() const;

    void setCustomFont(const QFont& font);
    QFont customFont() const;

protected:
    bool event(QEvent* event);

private slots:
    void openFontDialog();
    void changeMode(int index);

private:
    QComboBox* m_modeCombo;
    QPushButton* m_chooseFontButton;

    Mode m_mode;
    QFont m_customFont;
};

#endif
