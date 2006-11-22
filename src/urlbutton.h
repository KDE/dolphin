/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
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

#ifndef UrlBUTTON_H
#define UrlBUTTON_H

#include <qpushbutton.h>
//Added by qt3to4:
#include <QEvent>

class KUrl;
class UrlNavigator;
class QPainter;

/**
 * @brief Base class for buttons of the Url navigator.
 *
 * Each button of the Url navigator contains an Url, which
 * is set as soon as the button has been clicked.
*
 * @author Peter Penz
 */
class UrlButton : public QPushButton
{
    Q_OBJECT

public:
    UrlButton(UrlNavigator* parent);
    virtual ~UrlButton();

    UrlNavigator* urlNavigator() const;

protected:
    enum DisplayHint {
        ActivatedHint = 1,
        EnteredHint = 2,
        DraggedHint = 4,
        PopupActiveHint = 8
    };

    void setDisplayHintEnabled(DisplayHint hint, bool enable);
    bool isDisplayHintEnabled(DisplayHint hint) const;

    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);

    QColor mixColors(const QColor& c1, const QColor& c2) const;

private:
    int m_displayHint;
    UrlNavigator* m_urlNavigator;
};

#endif
