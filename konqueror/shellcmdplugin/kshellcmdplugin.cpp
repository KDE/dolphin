/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kshellcmdplugin.h"
#include <kaction.h>
#include <kinstance.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <konq_dirpart.h>
#include <klocale.h>
#include <kprocess.h>
#include "kshellcmddialog.h"

KShellCmdPlugin::KShellCmdPlugin( QObject* parent, const char* name )
    : KParts::Plugin( parent, name )
{
    new KAction( i18n( "&Execute Shell Command" ), CTRL+Key_E, this,
                 SLOT( slotExecuteShellCommand() ), actionCollection(), "executeshellcommand" );
}

void KShellCmdPlugin::slotExecuteShellCommand()
{
   KonqDirPart * part = dynamic_cast<KonqDirPart *>(parent());
   if ( !part )
   {
      KMessageBox::sorry(0L, "KShellCmdPlugin::slotExecuteShellCommand: Program error, please report a bug.");
      return;
   }
   if (!part->url().isLocalFile())
   {
      KMessageBox::sorry(part->widget(),i18n("Executing shell commands works only on local directories."));
      return;
   }
   bool isLocal = part->url().isLocalFile();
   QString defaultValue;
   if ( part->currentItem() )
      // Putting the complete path to the selected file isn't really necessary, since
      // we'll cd to the directory first (if isLocal).
      defaultValue = isLocal ? "./"+part->currentItem()->name() : part->currentItem()->url().prettyURL();
   else
      defaultValue = isLocal ? part->url().path() : part->url().prettyURL();
   KLineEditDlg l(i18n("Execute shell command in current directory:"), defaultValue, part->widget() );
   if ( l.exec() )
   {
      QString chDir;
      if ( isLocal )
      {
         chDir="cd ";
         chDir+=KShellProcess::quote(part->url().path());
         chDir+="; ";
      }
      chDir+=l.text();

      KShellCommandDialog *shellCmdDialog=new KShellCommandDialog(i18n("Output from command: \"%1\"").arg(l.text()),chDir,part->widget(),true);
      shellCmdDialog->resize(500,300);
      shellCmdDialog->executeCommand();
      delete shellCmdDialog;
   }
}


KShellCmdPluginFactory::KShellCmdPluginFactory( QObject* parent, const char* name )
  : KLibFactory( parent, name )
{
  s_instance = new KInstance("KPluginFactory");
}

KShellCmdPluginFactory::~KShellCmdPluginFactory()
{
  delete s_instance;
}

QObject* KShellCmdPluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
    return new KShellCmdPlugin( parent, name );
}

extern "C"
{
  void* init_libkshellcmdplugin()
  {
    return new KShellCmdPluginFactory;
  }

}

KInstance* KShellCmdPluginFactory::s_instance = 0L;

#include "kshellcmdplugin.moc"

