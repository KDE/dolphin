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

#include <qprinter.h>
#include "kfm_mainwindow.h"
#include "kfm_part.h"

#include <qkeycode.h>

#include <kapp.h>
#include <kiconloader.h>

#include <opMenuBarManager.h>
#include <opToolBarManager.h>
#include <opStatusBarManager.h>

enum ids { TOOLBAR_OPEN, TOOLBAR_SAVE, TOOLBAR_PRINT, TOOLBAR_RELOAD };

KfmMainWindow::KfmMainWindow()
{
  cout << "+KfmMainWindow" << endl;

  m_pFileMenu = 0L;
  m_pHelpMenu = 0L;
  
  // create the menu bar
  (void)menuBarManager();
  
  // create the toolbar manager to handle the toolbars of the embedded parts
  (void)toolBarManager();
  
  (void)statusBarManager();

  // build a toolbar and insert some buttons
  opToolBar()->insertButton(Icon("fileopen.xpm"),TOOLBAR_OPEN, SIGNAL( clicked() ),
			    this, SLOT( slotFileOpen() ), true, i18n("Open File"));
  opToolBar()->insertButton(Icon("filefloppy.xpm"), TOOLBAR_SAVE,
			    SIGNAL( clicked() ), this, SLOT( slotFileSave() ),
			    true, i18n("Save File") );
  opToolBar()->setItemEnabled( TOOLBAR_SAVE, false );
  opToolBar()->insertButton(Icon("fileprint.xpm"), TOOLBAR_PRINT,
			    SIGNAL( clicked() ), this, SLOT( slotFilePrint() ),
			    true, i18n("Print") );
  opToolBar()->setItemEnabled( TOOLBAR_PRINT, false );
  opToolBar()->insertButton(Icon("reload.xpm"),TOOLBAR_RELOAD, SIGNAL( clicked() ),
			    this, SLOT( slotReload() ), true, i18n("Reload"));

  m_pPart = new KfmPart( this );
  m_pPart->setMainWindow( interface() );
  setView( m_pPart );

  menuBarManager()->create( m_pPart->id() );
  toolBarManager()->create( m_pPart->id() );
  statusBarManager()->create( m_pPart->id() );
}

KfmMainWindow::~KfmMainWindow()
{ 
  cerr << "KfmMainWindow::~KfmMainWindow()" << endl;
}

OPMainWindowIf* KfmMainWindow::interface()
{
  if ( m_pInterface == 0L )
  {    
    m_pKfmInterface = new KfmMainWindowIf( this );
    m_pInterface = m_pKfmInterface;
  }
  
  return m_pInterface;
}

KfmMainWindowIf* KfmMainWindow::kfmInterface()
{
  if ( m_pInterface == 0L )
  {
      
    m_pKfmInterface = new KfmMainWindowIf( this );
    m_pInterface = m_pKfmInterface;
  }
  
  return m_pKfmInterface;
}

void KfmMainWindow::createFileMenu( OPMenuBar* _menubar )
{
  // Do we loose control over the menubar ?
  if ( _menubar == 0L )
  {
    m_pFileMenu = 0L;
    return;
  }
  
  // Usual Qt way of dealing with menus. We dont have to go the
  // CORBA way here, because the menubar is ours. That means it runs
  // in the same process.
  m_pFileMenu = new OPMenu( _menubar );

  m_idMenuFile_OpenURL = m_pFileMenu->insertItem( Icon( "filenew.xpm" ),
						 i18n( "&Open Location" ), this,
						 SLOT( slotUpdate() ),
						 CTRL + Key_O );
  m_pFileMenu->insertSeparator(-1);
  
  m_idMenuFile_Quit = m_pFileMenu->insertItem( i18n( "&Quit" ), this,
					       SLOT( slotQuit() ),
					       CTRL + Key_Q );

  _menubar->insertItem( i18n( "&File" ), m_pFileMenu );
}

void KfmMainWindow::createHelpMenu( OPMenuBar* _menubar )
{
  // Do we loose control over the menubar ?
  if ( _menubar == 0L )
  {
    m_pHelpMenu = 0L;
    return;
  }
  
  // Ask for the help menu. This is needed since
  // we share the help menu with the embedded components
  m_pHelpMenu = _menubar->helpMenu();
  // No help menu yet ?
  if ( m_pHelpMenu == 0L )
  {    
    m_pHelpMenu = new OPMenu( _menubar );

    _menubar->insertSeparator();
    _menubar->insertItem( i18n( "&Help" ), m_pHelpMenu );
  }
  else
    m_pHelpMenu->insertSeparator();

  // Insert our item
  m_idMenuHelp_About = m_pHelpMenu->insertItem( i18n( "&About Konqueror" ),
						this, SLOT( slotHelpAbout() ) );
}

void KfmMainWindow::slotHelpAbout()
{
}

void KfmMainWindow::slotQuit()
{
  delete this;
  
  kapp->exit();
}



KfmMainWindowIf::KfmMainWindowIf( KfmMainWindow* _main ) :
  OPMainWindowIf( _main )
{
  m_pKfmWin = _main;
}

KfmMainWindowIf::~KfmMainWindowIf()
{
}


#include "kfm_mainwindow.moc"

