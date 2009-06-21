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

    QTextEdit* textEdit;
    QLabel* addComment;

    QString comment;

private:
    CommentWidget* q;
};


void CommentWidget::Private::update()
{
    textEdit->setText( comment );
    const bool hasComment = !comment.isEmpty();
    textEdit->setVisible(hasComment);
    addComment->setVisible(!hasComment);
}


void CommentWidget::Private::_k_slotEnableEditing()
{
    textEdit->show();
    textEdit->setFocus();
    addComment->hide();
}



CommentWidget::CommentWidget( QWidget* parent )
    : QWidget( parent ),
      d( new Private( this ) )
{
    d->textEdit = new QTextEdit( this );
    d->textEdit->installEventFilter( this );
    d->textEdit->setMinimumHeight( 100 );
    d->textEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

    d->addComment = new QLabel( this );
    d->addComment->setText( "<p align=center><a style=\"font-size:small;\" href=\"addComment\">" + i18nc( "@label", "Add Comment..." ) + "</a>" );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    layout->addWidget( d->textEdit );
    layout->addWidget( d->addComment );
    d->update();
    connect( d->addComment, SIGNAL( linkActivated( const QString& ) ), this, SLOT( _k_slotEnableEditing() ) );
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


QString CommentWidget::editorText() const
{
    return d->textEdit->toPlainText();
}

bool CommentWidget::eventFilter( QObject* watched, QEvent* event )
{
    if ( watched == d->textEdit && event->type() == QEvent::FocusOut ) {
        const QString currentComment = editorText();
        if ( currentComment != d->comment ) {
            d->comment = currentComment;
            emit commentChanged( currentComment );
        }
        d->update();
    }
    return QWidget::eventFilter( watched, event );
}

#include "commentwidget.moc"
