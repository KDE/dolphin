/* This file is part of the KDE project
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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

#include "konqsidebarplugin.h"
#include "konqsidebarplugin.moc"
#include <kdebug.h>

KonqSidebar_PluginInterface *KonqSidebarPlugin::getInterfaces(){
  KonqSidebar_PluginInterface *jw1;
  kdDebug()<<"getInterfaces()"<<endl;

  jw1=dynamic_cast<KonqSidebar_PluginInterface *> (parent());

  if (jw1==0) kdDebug()<<"Kann keine Typumwandlung durchführen"<<endl; else kdDebug()<<"Translation worked"<<endl;
  return jw1;
 
}
