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
   KLineEditDlg l(i18n("Execute shell command:"),part->currentItem(), part->widget() );
   //KLineEditDlg l( i18n("Execute shell command:"), m_pListView->currentItem()?m_pListView->url().path()+QString("/")+m_pListView->currentItem()->text(0):"", m_pListView );
   if ( l.exec() )
   {
      QString chDir="cd ";
      chDir+=part->url().path();
      chDir+="; ";
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

