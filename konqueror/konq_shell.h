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

#ifndef __konq_shell_h__
#define __konq_shell_h__

#include <shell.h>

class KAction;
class KToggleAction;
class KHelpMenu;

class KonqShell : public Shell
{
  Q_OBJECT
public:
  KonqShell();
  ~KonqShell();

  // HACK for konq_mainview
  KToggleAction * menuBarAction() { return m_paShowMenuBar; }

public slots:
  void slotShowMenuBar();
  void slotShowStatusBar();
  void slotShowToolBar();
  void slotShowLocationBar();
  void slotShowBookmarkBar();
  void slotQuit();
  
protected:
  virtual QString configFile() const;

private:
  KAction *m_paShellClose;
  KAction *m_paShellQuit;
  KAction *m_paShellHelpAboutKDE;
  KHelpMenu *m_helpMenu;

  KToggleAction *m_paShowMenuBar;
  KToggleAction *m_paShowStatusBar;
  KToggleAction *m_paShowToolBar;
  KToggleAction *m_paShowLocationBar;
  KToggleAction *m_paShowBookmarkBar;
  
};

#endif
