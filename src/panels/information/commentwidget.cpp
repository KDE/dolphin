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

#include "commentwidget.h"
#include "commenteditwidget.h"

#include <QtGui/QLabel>
#include <QtGui/QTextEdit>
#include <QtGui/QLayout>
#include <QtGui/QCursor>
#include <QtCore/QEvent>

#include <KLocale>


class CommentWidget::Private
{
public:
    Private( CommentWidget* parent )
        : q( parent ) {
    }

    void update();
    void _k_slotEnableEditing();

    QLabel* label;
    CommentEditWidget* edit;

    QString comment;

private:
    CommentWidget* q;
};


void CommentWidget::Private::update()
{
    if ( comment.isEmpty() ) {
        label->setText( "<p align=center><a style=\"font-size:small;\" href=\"addComment\">" + i18nc( "@label", "Add Comment..." ) + "</a>" );
    }
    else {
        label->setText( "<p>" + comment + "<p align=center><a style=\"font-size:small;\" href=\"addComment\">" + i18nc( "@label", "Change Comment..." ) + "</a>" );
    }
}


void CommentWidget::Private::_k_slotEnableEditing()
{
    CommentEditWidget w;
    w.setComment( comment );
    if ( w.exec( QCursor::pos() ) ) {
        comment = w.comment();
        update();
        emit q->commentChanged( comment );
    }
}



CommentWidget::CommentWidget( QWidget* parent )
    : QWidget( parent ),
      d( new Private( this ) )
{
    d->label = new QLabel( this );
    d->label->setWordWrap( true );
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    layout->addWidget( d->label );
    d->update();
    connect( d->label, SIGNAL( linkActivated( const QString& ) ), this, SLOT( _k_slotEnableEditing() ) );
}


CommentWidget::~CommentWidget()
{
    delete d;
}


void CommentWidget::setComment( const QString& comment )
{
    d->comment = comment;
    d->update();
}


QString CommentWidget::comment() const
{
    return d->comment;
}


bool CommentWidget::eventFilter( QObject* watched, QEvent* event )
{
    return QWidget::eventFilter( watched, event );
}

#include "commentwidget.moc"
