/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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

#include <iostream.h>

#include "konq_run.h"
#include "konq_part.h"
#include "konq_misc.h"
#include "konq_shell.h"
#include "konq_mainview.h"

#include <ktrader.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>


#include <kapp.h>


int main( int argc, char **argv )
{
  KApplication app( argc, argv, "konqueror" );



  KGlobal::locale()->insertCatalogue("libkonq"); // needed for apps using libkonq

  QString path = KGlobal::dirs()->saveLocation("data", "kfm/bookmarks", true);
  KonqBookmarkManager bm ( path );
  KonqFileManager fm;

  if ( argc == 1 )
  {
    fm.openFileManagerWindow( 0L );
  }
  else
  {
    for ( int i = 1; i < argc; i++ )
      fm.openFileManagerWindow( argv[ i ] );
  }

  app.exec();

  return 0;
}
