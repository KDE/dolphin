/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>

#include "kshellcmddialog.h"
#include "kshellcmdexecutor.h"

KShellCommandDialog::KShellCommandDialog(const QString& title, const QString& command, QWidget* parent, bool modal)
   :KDialog(parent)
{
   setModal( modal );
   QVBoxLayout * box=new QVBoxLayout (this,marginHint(),spacingHint());

   QLabel *label=new QLabel(title,this);
   m_shell=new KShellCommandExecutor(command,this);

   cancelButton= new KPushButton(KStdGuiItem::cancel(), this);
   closeButton= new KPushButton(KStdGuiItem::close(), this);
   closeButton->setDefault(true);

   label->resize(label->sizeHint());
   m_shell->resize(m_shell->sizeHint());
   closeButton->setFixedSize(closeButton->sizeHint());
   cancelButton->setFixedSize(cancelButton->sizeHint());

   box->addWidget(label,0);
   box->addWidget(m_shell,1);

   QHBoxLayout * hlayout = new QHBoxLayout(box);
   hlayout->setSpacing( spacingHint() );
   hlayout->addWidget( cancelButton );
   hlayout->addWidget( closeButton );

   m_shell->setFocus();

   connect(cancelButton, SIGNAL(clicked()), m_shell, SLOT(slotFinished()));
   connect(m_shell, SIGNAL(finished()), this, SLOT(disableStopButton()));
   connect(closeButton,SIGNAL(clicked()), this, SLOT(slotClose()));
}

KShellCommandDialog::~KShellCommandDialog()
{
   delete m_shell;
   m_shell=0;
}

void KShellCommandDialog::disableStopButton()
{
   cancelButton->setEnabled(false);
}

void KShellCommandDialog::slotClose()
{
   delete m_shell;
   m_shell=0;
   accept();
}

//blocking
int KShellCommandDialog::executeCommand()
{
   if (m_shell==0)
      return 0;
   //kDebug()<<"---------- KShellCommandDialog::executeCommand()"<<endl;
   m_shell->exec();
   return exec();
}

#include "kshellcmddialog.moc"
