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

#include "konq_shell.h"

#include <kaction.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kapp.h>
#include <kstdaccel.h>
#include <kstddirs.h>
#include <khelpmenu.h>
#include <part.h>

KonqShell::KonqShell()
{
  KStdAccel accel;

  m_helpMenu = new KHelpMenu( this );

  m_paShellClose = new KAction( i18n( "Close" ), BarIcon( "close" ), accel.close(), this, SLOT( close() ), actionCollection(), "konqueror_shell_close" );
  //  m_paShellQuit = new KAction( i18n( "Quit" ), BarIcon( "exit" ), accel.quit(), this, SLOT( slotQuit() ), actionCollection(), "konqueror_shell_quit" );
  m_paShellHelpAboutKDE = new KAction( i18n( "About &KDE..." ), 0, m_helpMenu, SLOT( aboutKDE() ), actionCollection(), "konqueror_shell_aboutkde" );

  m_paShowMenuBar = new KToggleAction( i18n( "Show &Menubar" ), 0, actionCollection(), "showmenubar" );
  m_paShowStatusBar = new KToggleAction( i18n( "Show &Statusbar" ), 0, actionCollection(), "showstatusbar" );
  m_paShowToolBar = new KToggleAction( i18n( "Show &Toolbar" ), 0, actionCollection(), "showtoolbar" );
  m_paShowLocationBar = new KToggleAction( i18n( "Show &Locationbar" ), 0, actionCollection(), "showlocationbar" );

  m_paShowMenuBar->setChecked( true );
  m_paShowStatusBar->setChecked( true );
  m_paShowToolBar->setChecked( true );
  m_paShowLocationBar->setChecked( true );

  connect( m_paShowMenuBar, SIGNAL( activated() ), this, SLOT( slotShowMenuBar() ) );
  connect( m_paShowStatusBar, SIGNAL( activated() ), this, SLOT( slotShowStatusBar() ) );
  connect( m_paShowToolBar, SIGNAL( activated() ), this, SLOT( slotShowToolBar() ) );
  connect( m_paShowLocationBar, SIGNAL( activated() ), this, SLOT( slotShowLocationBar() ) );

}

KonqShell::~KonqShell()
{
  delete rootPart();
}

void KonqShell::slotShowMenuBar()
{
  if (menuBar()->isVisible())
    menuBar()->enable( KMenuBar::Hide );
  else
    menuBar()->enable( KMenuBar::Show );
}

void KonqShell::slotShowStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->enable( KStatusBar::Hide );
  else
    statusBar()->enable( KStatusBar::Show );
}

void KonqShell::slotShowToolBar()
{
  KToolBar * bar = viewToolBar( "mainToolBar" );
  if (!bar) return;
  if (bar->isVisible())
    bar->enable( KToolBar::Hide );
  else
    bar->enable( KToolBar::Show );
}

void KonqShell::slotShowLocationBar()
{
  KToolBar * bar = viewToolBar( "locationToolBar" );
  if (!bar) return;
  if (bar->isVisible())
    bar->enable( KToolBar::Hide );
  else
    bar->enable( KToolBar::Show );
}

void KonqShell::slotQuit()
{
  kapp->exit();
}

QString KonqShell::configFile() const
{
  return readConfigFile( locate( "data", "konqueror/konqueror_shell.rc" ) );
}

#include "konq_shell.moc"
