/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright (C) 2000 qwertz <kraftw@gmx.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <qevent.h>
#include <qpushbutton.h>

class QButtonGroup;
class QListView;
class QVBox;
class ArrowButton;

class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    HistoryWidget( QWidget *parent=0, const char *name=0 );
    ~HistoryWidget();

private:
    enum Button { Older = 0, Monday = 1, Tuesday = 2, Wednesday = 3,
		  Thursday = 4, Friday = 5, Saturday = 6, Sunday = 7,
		  Today = 8
    };

    QButtonGroup *m_buttons;
    QPixmap m_htmlPixmap;
    QString m_days[9];

    QVBox *m_historyView;
    QListView *m_listView;
    ArrowButton *m_dayButton;

    void updateWeek();
    void fill();

private slots:
    void slotClicked( int id );
    void backToWeek();

};


class ArrowButton : public QPushButton
{
    Q_OBJECT

public:
    enum ArrowDirection { Up, Down };

    ArrowButton( QWidget *parent, const char *name=0 );
    void setDirection( ArrowDirection d );
    ArrowDirection direction() const { return m_direction; }

protected:
    virtual void paintEvent( QPaintEvent* );

private:
    ArrowDirection m_direction;
    QPixmap m_arrowUp, m_arrowDown;

};

#endif // HISTORYWIDGET_H
