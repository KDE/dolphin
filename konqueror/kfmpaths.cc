/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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

#include "kfmpaths.h"
#include <kconfig.h>
#include <qdir.h>
#include <kapp.h>
#include <qstring.h>

string* KfmPaths::s_desktopPath = 0L;
string* KfmPaths::s_templatePath = 0L;
string* KfmPaths::s_autostartPath = 0L;
string* KfmPaths::s_trashPath = 0L;

void KfmPaths::initStatic() 
{
  if ( s_desktopPath == 0L )
    s_desktopPath = new string;
  if ( s_templatePath == 0L )
    s_templatePath = new string;
  if ( s_autostartPath == 0L )
    s_autostartPath = new string;
  if ( s_trashPath == 0L )
    s_trashPath = new string;

  QString _desktopPath;
  QString _templatePath;
  QString _autostartPath;
  QString _trashPath;
    
  KConfig *config = kapp->getConfig();
  config->setGroup( "Paths" );

  // Desktop Path
  _desktopPath = QDir::homeDirPath() + "/Desktop/";
  _desktopPath = config->readEntry( "Desktop", _desktopPath);
  if ( _desktopPath.right(1) != "/")
    _desktopPath += "/";
  
  // Templates Path
  _templatePath = _desktopPath + "Templates/";
  _templatePath = config->readEntry( "Templates" , _templatePath);
  if ( _templatePath.right(1) != "/")
    _templatePath += "/";

  // Autostart Path
  _autostartPath = _desktopPath + "Autostart/";
  _autostartPath = config->readEntry( "Autostart" , _autostartPath);
  if ( _autostartPath.right(1) != "/")
    _autostartPath += "/";

  // Trash Path
  _trashPath = _desktopPath + "Trash/";
  _trashPath = config->readEntry( "Trash" , _trashPath);
  if ( _autostartPath.right(1) != "/")
    _autostartPath += "/";

  *s_desktopPath = _desktopPath;
  *s_templatePath = _templatePath;
  *s_autostartPath = _autostartPath;
  *s_trashPath = _trashPath;
}

const char* KfmPaths::desktopPath()
{
  return s_desktopPath->c_str();
}
 
const char* KfmPaths::templatesPath()
{
  return s_templatePath->c_str();
}

const char* KfmPaths::autostartPath()
{
  return s_autostartPath->c_str();
}

const char* KfmPaths::trashPath()
{
  return s_trashPath->c_str();
}
