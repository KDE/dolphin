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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <qstring.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <klocale.h>

#include "kshellcmddialog.h"
#include "kshellcmdexecutor.h"

KShellCommandDialog::KShellCommandDialog(const QString& title, const QString& command, QWidget* parent, bool modal)
   :KDialog(parent,"p",modal)
{
   QVBoxLayout * box=new QVBoxLayout (this,marginHint(),spacingHint());

   QLabel *label=new QLabel(title,this);
   m_shell=new KShellCommandExecutor(command,this);

   QHBox *buttonsBox=new QHBox(this);
   buttonsBox->setSpacing(spacingHint());

   stopButton=new QPushButton(i18n("Cancel"),buttonsBox);
   QPushButton *closeButton=new QPushButton(i18n("Close"),buttonsBox);
   closeButton->setDefault(true);


   label->resize(label->sizeHint());
   m_shell->resize(m_shell->sizeHint());
   closeButton->setFixedSize(closeButton->sizeHint());
   stopButton->setFixedSize(stopButton->sizeHint());

   box->addWidget(label,0);
   box->addWidget(m_shell,1);
   box->addWidget(buttonsBox,0);

   m_shell->setFocus();

   connect(stopButton, SIGNAL(clicked()), m_shell, SLOT(slotFinished()));
   connect(m_shell, SIGNAL(finished()), this, SLOT(disableStopButton()));
   connect(closeButton,SIGNAL(clicked()), this, SLOT(slotClose()));
};

KShellCommandDialog::~KShellCommandDialog()
{
   delete m_shell;
   m_shell=0;
};

void KShellCommandDialog::disableStopButton()
{
   stopButton->setEnabled(false);
};

void KShellCommandDialog::slotClose()
{
   delete m_shell;
   m_shell=0;
   accept();
};

//blocking
int KShellCommandDialog::executeCommand()
{
   if (m_shell==0)
      return 0;
   //kdDebug()<<"---------- KShellCommandDialog::executeCommand()"<<endl;
   m_shell->exec();
   return exec();
};

#include "kshellcmddialog.moc"
