/*
   kproxyexceptiondlg.h - Proxy exception dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   (GPL) version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write
   to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include "kproxydlg.h"

class KExpLineEdit : public KLineEdit
{
public:
  KExpLineEdit( QWidget* parent, const char* name=0L );

protected:
  virtual void keyPressEvent( QKeyEvent * e );

};

KExpLineEdit::KExpLineEdit( QWidget* parent, const char* name )
            :KLineEdit( parent, name )
{
    // For now do not accept any drops since they
    // might contain characters we do not accept.
    // TODO: Re-implement ::dropEvent to allow
    // acceptable formats...
    setAcceptDrops( false );
}

void KExpLineEdit::keyPressEvent( QKeyEvent* e )
{
    int key = e->key();
    QString keycode = e->text();
    if ( (key >= Qt::Key_Escape && key <= Qt::Key_Help) ||
          key == Qt::Key_Period || (cursorPosition() > 0 &&
          key == Qt::Key_Minus) || (!keycode.isEmpty() &&
          keycode.unicode()->isLetterOrNumber()) )
    {
        KLineEdit::keyPressEvent(e);
        return;
    }
    e->accept();
}

KProxyExceptionDlg::KProxyExceptionDlg( QWidget* parent,  const char* name )
                   :KDialog( parent, name, true )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this,
                                               1.9*KDialog::marginHint(),
                                               KDialog::spacingHint() );

    QLabel* label = new QLabel( i18n("Enter address (URL) that should be "
                                     "excluded from using a proxy server:"),
                                this, "lb_excpetions" );
    label->setSizePolicy( QSizePolicy( QSizePolicy::Preferred,
                                       QSizePolicy::Fixed,
                                       label->sizePolicy().hasHeightForWidth() ) );
    label->setAlignment( (QLabel::WordBreak | QLabel::AlignTop |
                          QLabel::AlignLeft) );
    mainLayout->addWidget( label );

    le_exceptions = new KExpLineEdit( this, "le_exceptions" );
    le_exceptions->setMinimumWidth( le_exceptions->fontMetrics().width('W') * 20 );
    le_exceptions->setSizePolicy( QSizePolicy( QSizePolicy::Preferred,
                                               QSizePolicy::Fixed,
                                               le_exceptions->sizePolicy().hasHeightForWidth() ) );
    QWhatsThis::add( le_exceptions, i18n("<qt>Enter the site name(s) that "
                                         "should be exempted from using the "
                                         "proxy server(s) specified above.<p>"
                                         "Note that the reverse is true if "
                                         "the \"<code>Only use proxy for "
                                         "enteries in this list</code>\" "
                                         "box is checked.  That is only URLs "
                                         "that match one of the addresses in "
                                         "this would be allowed to use proxy "
                                         "servers") );
    mainLayout->addWidget( le_exceptions );
    QSpacerItem* spacer = new QSpacerItem( 16, 16, QSizePolicy::Minimum,
                                           QSizePolicy::Fixed );
    mainLayout->addItem( spacer );

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    spacer = new QSpacerItem( 20, 20, QSizePolicy::MinimumExpanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    pb_ok = new QPushButton( i18n("&OK"), this, "pb_ok" );
    pb_ok->setEnabled( false );
    hlay->addWidget( pb_ok );

    pb_cancel = new QPushButton( i18n("&Cancel"), this, "pb_cancel" );
    hlay->addWidget( pb_cancel );
    mainLayout->addLayout( hlay );
    le_exceptions->setFocus();

    connect( pb_ok, SIGNAL( clicked() ), SLOT( accept() ) );
    connect( le_exceptions, SIGNAL( clicked() ), pb_ok, SLOT( animateClick() ) );
    connect( pb_cancel, SIGNAL( clicked() ), SLOT( reject() ) );
    connect( le_exceptions, SIGNAL( textChanged( const QString& ) ),
             SLOT( slotTextChanged( const QString& ) ) );
}

KProxyExceptionDlg::~KProxyExceptionDlg()
{
}

void KProxyExceptionDlg::accept()
{
    QDialog::accept();
}

void KProxyExceptionDlg::reject()
{
    QDialog::reject();
}

void KProxyExceptionDlg::slotTextChanged( const QString& text )
{
    pb_ok->setEnabled( !text.isEmpty() );
}

QString KProxyExceptionDlg::exception() const
{
    return le_exceptions->text();
}

void KProxyExceptionDlg::setException( const QString& text )
{
    if ( !text.isEmpty() )
        le_exceptions->setText( text );
}
