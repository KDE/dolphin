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

#ifndef __kfm_mainwindow_h__
#define __kfm_mainwindow_h__

class KfmMainWindow;
class KfmGui;

#include <opMainWindow.h>
#include <opMainWindowIf.h>
#include <opMenu.h>

#include "kfm.h"

class KfmMainWindowIf : virtual public OPMainWindowIf,
			virtual public KFM::MainWindow_skel
{
public:
  KfmMainWindowIf( KfmMainWindow* _main );
  virtual ~KfmMainWindowIf();

protected:
  KfmMainWindow* m_pKfmWin;
};

class KfmMainWindow : public OPMainWindow
{
  Q_OBJECT
public:
  // C++
  KfmMainWindow( const char *url );
  ~KfmMainWindow();

  virtual OPMainWindowIf* interface();
  virtual KfmMainWindowIf* kfmInterface();

protected slots:
  void slotHelpAbout();
  void slotQuit();

protected:
  void createFileMenu( OPMenuBar* _menubar );
  void createHelpMenu( OPMenuBar* _menubar );
  
  KfmMainWindowIf* m_pKfmInterface;

  OPMenu* m_pFileMenu;
  CORBA::Long m_idMenuFile_OpenURL;
  CORBA::Long m_idMenuFile_Quit;

  OPMenu* m_pHelpMenu;
  CORBA::Long m_idMenuHelp_About;

  KfmGui* m_pPart;
};

#endif
