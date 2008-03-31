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

#include "commenteditwidget.h"

#include <QtGui/QTextEdit>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtCore/QEventLoop>
#include <QtCore/QPointer>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMouseEvent>
#include <QtGui/QFont>

#include <KIcon>
#include <KDialog>
#include <KLocale>
#include <KDebug>


class CommentEditWidget::Private
{
public:
    Private( CommentEditWidget* parent )
        : eventLoop( 0 ),
          q( parent ) {
    }

    QEventLoop* eventLoop;
    bool success;
    QTextEdit* textEdit;
    QToolButton* buttonSave;
    QToolButton* buttonCancel;

    QString comment;

    QRect geometryForPopupPos( const QPoint& p ) {
        QSize size = q->sizeHint();

        // we want a little margin
        const int margin = KDialog::marginHint();
        size.setHeight( size.height() + margin*2 );
        size.setWidth( size.width() + margin*2 );

        QRect screen = QApplication::desktop()->screenGeometry( QApplication::desktop()->screenNumber( p ) );

        // calculate popup position
        QPoint pos( p.x() - size.width()/2, p.y() - size.height()/2 );

        // ensure we do not leave the desktop
        if ( pos.x() + size.width() > screen.right() ) {
            pos.setX( screen.right() - size.width() );
        }
        else if ( pos.x() < screen.left() ) {
            pos.setX( screen.left() );
        }

        if ( pos.y() + size.height() > screen.bottom() ) {
            pos.setY( screen.bottom() - size.height() );
        }
        else if ( pos.y() < screen.top() ) {
            pos.setY( screen.top() );
        }

        return QRect( pos, size );
    }

    void _k_saveClicked();
    void _k_cancelClicked();

private:
    CommentEditWidget* q;
};


void CommentEditWidget::Private::_k_saveClicked()
{
    comment = textEdit->toPlainText();
    success = true;
    q->hide();
}


void CommentEditWidget::Private::_k_cancelClicked()
{
    success = false;
    q->hide();
}


CommentEditWidget::CommentEditWidget( QWidget* parent )
    : QFrame( parent ),
      d( new Private( this ) )
{
    setFrameStyle( QFrame::Box|QFrame::Plain );
    setWindowFlags( Qt::Popup );

    d->textEdit = new QTextEdit( this );
    d->textEdit->installEventFilter( this );
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    layout->addWidget( d->textEdit );

    d->buttonSave = new QToolButton( d->textEdit );
    d->buttonCancel = new QToolButton( d->textEdit );
    d->buttonSave->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    d->buttonCancel->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    d->buttonSave->setAutoRaise( true );
    d->buttonCancel->setAutoRaise( true );
    d->buttonSave->setIcon( KIcon( "document-save" ) );
    d->buttonCancel->setIcon( KIcon( "edit-delete" ) );
    d->buttonSave->setText( i18n( "Save" ) );
    d->buttonCancel->setText( i18n( "Cancel" ) );

    QFont fnt( font() );
    fnt.setPointSize( fnt.pointSize()-2 );
    d->buttonSave->setFont( fnt );
    d->buttonCancel->setFont( fnt );

    connect( d->buttonSave, SIGNAL(clicked()),
             this, SLOT( _k_saveClicked() ) );
    connect( d->buttonCancel, SIGNAL(clicked()),
             this, SLOT( _k_cancelClicked() ) );
}


CommentEditWidget::~CommentEditWidget()
{
    delete d;
}


void CommentEditWidget::setComment( const QString& s )
{
    d->comment = s;
}


QString CommentEditWidget::comment()
{
    return d->comment;
}


bool CommentEditWidget::exec( const QPoint& pos )
{
    d->success = false;
    d->textEdit->setPlainText( d->comment );
    d->textEdit->setFocus();
    d->textEdit->moveCursor( QTextCursor::End );
    QEventLoop eventLoop;
    d->eventLoop = &eventLoop;
    setGeometry( d->geometryForPopupPos( pos ) );
    show();

    QPointer<QObject> guard = this;
    (void) eventLoop.exec();
    if ( !guard.isNull() )
        d->eventLoop = 0;
    return d->success;
}


void CommentEditWidget::mousePressEvent( QMouseEvent* e )
{
    // clicking outside of the widget means cancel
    if ( !rect().contains( e->pos() ) ) {
        d->success = false;
        hide();
    }
    else {
        QWidget::mousePressEvent( e );
    }
}


void CommentEditWidget::hideEvent( QHideEvent* e )
{
    Q_UNUSED( e );
    if ( d->eventLoop ) {
        d->eventLoop->exit();
    }
}


void CommentEditWidget::updateButtons()
{
    QSize sbs = d->buttonSave->sizeHint();
    QSize cbs = d->buttonCancel->sizeHint();

    // FIXME: button order
    d->buttonCancel->setGeometry( QRect( QPoint( d->textEdit->width() - cbs.width() - frameWidth(),
                                                 d->textEdit->height() - cbs.height() - frameWidth() ),
                                         cbs ) );
    d->buttonSave->setGeometry( QRect( QPoint( d->textEdit->width() - cbs.width() - sbs.width() - frameWidth(),
                                               d->textEdit->height() - sbs.height() - frameWidth() ),
                                       sbs ) );
}


void CommentEditWidget::resizeEvent( QResizeEvent* e )
{
    QWidget::resizeEvent( e );
    updateButtons();
}


bool CommentEditWidget::eventFilter( QObject* watched, QEvent* event )
{
    if ( watched == d->textEdit && event->type() == QEvent::KeyPress ) {
        QKeyEvent* ke = static_cast<QKeyEvent*>( event );
        kDebug() << "keypress:" << ke->key() << ke->modifiers();
        if ( ( ke->key() == Qt::Key_Enter ||
               ke->key() == Qt::Key_Return ) &&
             ke->modifiers() & Qt::ControlModifier ) {
            d->_k_saveClicked();
            return true;
        }
    }

    return QFrame::eventFilter( watched, event );
}

#include "commenteditwidget.moc"
