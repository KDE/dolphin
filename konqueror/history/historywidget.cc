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

#include <qbuttongroup.h>
#include <qdatetime.h>
#include <qfont.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qpainter.h>
#include <qvbox.h>

#include <kiconloader.h>
#include <klocale.h>

#include "historywidget.h"


HistoryWidget::HistoryWidget( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    QFont font;
    font.setBold( true );
    m_htmlPixmap = SmallIcon( "html" );

    QBoxLayout *layout = new QVBoxLayout( this );
    layout->setAutoAdd( true );

    QLabel *label = new QLabel( i18n("History"), this );
    label->setAlignment( AlignCenter );
    label->setMaximumHeight( label->sizeHint().height() );
    
    
    m_days[Older] = i18n("Older");
    m_days[Monday] = i18n("Monday");
    m_days[Tuesday] = i18n("Tuesday");
    m_days[Wednesday] = i18n("Wednesday");
    m_days[Thursday] = i18n("Thursday");
    m_days[Friday] = i18n("Friday");
    m_days[Saturday] = i18n("Saturday");
    m_days[Sunday] = i18n("Sunday");
    m_days[Today] = i18n("Today");

    m_buttons = new QButtonGroup( this );
    QVBoxLayout *l2 = new QVBoxLayout( m_buttons );
    connect( m_buttons, SIGNAL( clicked( int )), SLOT( slotClicked( int )));
    
    // automatic assignment of ids [0..7] ... for the buttongroup
    QButton *b;
    for ( int i = 0; i < 8; i++ ) {
	b = new ArrowButton( m_buttons );
	l2->addWidget( b );
    }
    l2->addStretch( 10 );

    // static button texts
    b = m_buttons->find( Older );
    b->setFont( font );
    b->setText( m_days[Older] );
    b = m_buttons->find( 7 );
    b->setText( m_days[Today] );
    b->setFont( font );
    

    // the view with the day-button and the listview
    m_historyView = new QVBox( this );

    m_dayButton = new ArrowButton( m_historyView );
    m_dayButton->setDirection( ArrowButton::Up );
    m_dayButton->setFont( font );
    connect( m_dayButton, SIGNAL(clicked()), this, SLOT( backToWeek()) );

    m_listView = new QListView( m_historyView );
    m_listView->setRootIsDecorated( true );
    m_listView->header()->hide();
    m_listView->addColumn(QString::null);
    
    backToWeek();
}

HistoryWidget::~HistoryWidget()
{
}


// set the days as text of the buttons
void HistoryWidget::updateWeek()
{
    int day = QDate::currentDate().addDays( -1 ).dayOfWeek();

    for ( int i = 6; i > 0; i-- ) {
	m_buttons->find( i )->setText( m_days[day] );
	if ( --day == Older )
	    day = Sunday;
    }
}


void HistoryWidget::slotClicked( int id )
{
    m_dayButton->setText( m_buttons->find( id )->text() );
    m_buttons->hide();
    fill();
    m_historyView->show();
}

void HistoryWidget::backToWeek()
{
    updateWeek();
    m_historyView->hide();
    m_buttons->show();
}


void HistoryWidget::fill()
{
    m_listView->clear();
    
    //FAKE  gis will do it
    QListViewItem *q1,*q2,*q3,*q4,*q5,*q6;

    q1=new  QListViewItem (m_listView,"www.kde.org");
    q2=new  QListViewItem (q1,"index.html");
    q3=new  QListViewItem (q1,"artist.html");

    q4=new  QListViewItem (m_listView,"www.kde.org");
    q5=new  QListViewItem (m_listView,"index.html");
    q6=new  QListViewItem (q5,"artist.html");

    q1->setPixmap(0,m_htmlPixmap);
    q5->setPixmap(0,m_htmlPixmap);
}


///////////////////////////////////////////////////////////////////


ArrowButton::ArrowButton( QWidget *parent, const char *name )
    : QPushButton(  parent, name )
{
    m_direction = Down;
    setFlat( true );
    
    m_arrowUp = SmallIcon( "up" );
    m_arrowDown = SmallIcon( "down" );
}

void ArrowButton::setDirection( ArrowDirection d )
{
    m_direction = d;
    repaint();
}

void ArrowButton::paintEvent( QPaintEvent *event )
{
    QPushButton::paintEvent(event);
    QPainter paint(this);
    paint.setClipRect( event->rect() );
    
    //up or down?
    if ( m_direction == Up )
	paint.drawPixmap( width() - (m_arrowUp.width()+3),
			  (height() - m_arrowUp.height())/2, m_arrowUp );
    else
	paint.drawPixmap( width() - (m_arrowDown.width()+3),
			  (height() - m_arrowDown.height())/2, m_arrowDown );

    //shadow / highlight
    QColorGroup gr = colorGroup();
    QPen pen(white,1);
    paint.setPen(gr.light());
    paint.drawLine(0,0,width(),0);
    paint.setPen(gr.dark());
    paint.drawLine(0,height()-1,width(),height()-1);
}


#include "historywidget.moc"
