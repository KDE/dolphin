/*

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/



#ifndef __NSPluginCallbackIface_h__
#define __NSPluginCallbackIface_h__


#include <q3cstring.h>
#include <dcopobject.h>


class NSPluginCallbackIface : virtual public DCOPObject
{
  K_DCOP

k_dcop:

  virtual ASYNC requestURL(QString url, QString target) = 0;
  virtual ASYNC postURL(QString url, QString target, QByteArray data, QString mime) = 0;
  virtual ASYNC statusMessage( QString msg ) = 0;
  virtual ASYNC evalJavaScript( int id, QString script ) = 0;

};


#endif

