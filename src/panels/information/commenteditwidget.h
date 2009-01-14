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

#ifndef _COMMENT_EDIT_WIDGET_H_
#define _COMMENT_EDIT_WIDGET_H_

#include <QtGui/QFrame>

class QResizeEvent;
class QMouseEvent;
class QHideEvent;

class CommentEditWidget : public QFrame
{
    Q_OBJECT

public:
    CommentEditWidget( QWidget* parent = 0 );
    ~CommentEditWidget();

    void setComment( const QString& s );
    QString comment();

    /**
     * Show the comment widget at position pos.
     * \return true if the user chose to save the comment,
     * false otherwise.
     */
    bool exec( const QPoint& pos );

    bool eventFilter( QObject* watched, QEvent* event );

private:
    void updateButtons();
    void resizeEvent( QResizeEvent* );
    void mousePressEvent( QMouseEvent* e );
    void hideEvent( QHideEvent* e );

    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void _k_saveClicked() )
    Q_PRIVATE_SLOT( d, void _k_cancelClicked() )
};

#endif
