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

#include <komApplication.h>
#include <kded_instance.h>
#include <krun.h>
#include "kdesktop.h"

class clientApp : public KOMApplication,
                  public KFileManager
{
public:

  clientApp( int &argc, char **argv, const QCString& rAppName )
    : KOMApplication ( argc, argv, rAppName )
    {
      kded = new KdedInstance( argc, argv, komapp_orb );
      trader = kded->ktrader();
      activator = kded->kactivator();
    };

  ~clientApp() { /* delete kded */ } ;

  /** Parse command-line arguments and "do it" */
  int doIt( int argc, char **argv );

  /** Implements KFileManager interface */
  virtual bool openFileManagerWindow(const char* _url);

protected:

  void initRegistry();

  bool getKonqy();
  bool getKDesky();

  KdedInstance *kded;
  KTrader *trader;
  KActivator *activator;

  CORBA::Object_var m_vKonqy;
  KDesktopIf_ptr m_vKDesky;
};

#endif
