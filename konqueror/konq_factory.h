/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 David Faure <faure@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __konq_factory_h__
#define __konq_factory_h__ $Id$

#include <qstring.h>
#include <qstringlist.h>

#include "konq_defs.h"

#include <klibglobal.h>
#include <klibloader.h>

class BrowserView;
class KLibGlobal;

class KonqFactory : public KLibFactory
{
  Q_OBJECT
public:
  KonqFactory();

  static BrowserView *createView( const QString &serviceType,
			          QStringList &serviceTypes,
				  Konqueror::DirectoryDisplayMode dirMode = Konqueror::LargeIcons );

  virtual QObject* create( QObject* parent = 0, const char* name = 0, const char* classname = "QObject" );

  static KLibGlobal *global();				

private:
  static KLibGlobal *s_global;
};

#endif
