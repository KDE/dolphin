/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __kfmclient_h
#define __kfmclient_h

#include <opApplication.h>
#include "konqueror.h"

class clientApp : public OPApplication
{
public:

  clientApp( int &argc, char **argv, const QString& rAppName = QString::null )
    : OPApplication ( argc, argv, rAppName ) { };

  ~clientApp() {} ;

  /** Parse command-line arguments and "do it" */
  int doIt( int argc, char **argv );

protected:

  void openURL( const char *s );

  bool getIOR();

  Konqueror::Application_ptr m_vApp;

};

#endif
