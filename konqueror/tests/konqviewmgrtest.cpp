/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqviewmgrtest.h"

#include <QApplication>
#include <QLabel>
#include <QTimer>

MyMainWindow::MyMainWindow( QWidget* parent )
    : QMainWindow( parent )
{
    QLabel* widget1 = new QLabel( "widget1" );
    setCentralWidget( widget1 );
    QTimer::singleShot( 10, this, SLOT( slotSwitchCentralWidget() ) );
}

void MyMainWindow::slotSwitchCentralWidget()
{
    QLabel* widget2 = new QLabel( "widget2" );
    setCentralWidget( widget2 );
}

int main( int argc, char** argv ) {
    QApplication app( argc, argv );

    MyMainWindow* mw = new MyMainWindow;

    mw->show();

    return app.exec();
}

#include "konqviewmgrtest.moc"
