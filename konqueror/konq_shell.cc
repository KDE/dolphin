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

KonqShell::KonqShell()
{

  KStdAccel accel;

  m_paShellClose = new KAction( i18n( "Close" ), accel.close(), this, SLOT( slotClose() ), actionCollection(), "konqueror_shell_close" );
  m_paShellQuit = new KAction( i18n( "Quit" ), accel.quit(), this, SLOT( slotQuit() ), actionCollection(), "konqueror_shell_quit" );

}

KonqShell::~KonqShell()
{
}

void KonqShell::slotClose()
{
  delete this;
}

void KonqShell::slotQuit()
{
  kapp->exit();
}

QString KonqShell::configFile() const
{
  return readConfigFile( "konqueror_shell.rc" );
}

