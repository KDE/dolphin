/***************************************************************************
 *   Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                 *
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

#ifndef _NEPOMUK_COMMENT_WIDGET_H_
#define _NEPOMUK_COMMENT_WIDGET_H_

#include <QtGui/QWidget>

class CommentWidget : public QWidget
{
    Q_OBJECT

public:
    CommentWidget( QWidget* parent = 0 );
    ~CommentWidget();

    void setComment( const QString& comment );
    QString comment() const;

Q_SIGNALS:
    void commentChanged( const QString& );

private:
    bool eventFilter( QObject* watched, QEvent* event );

    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void _k_slotEnableEditing() )
};

#endif
