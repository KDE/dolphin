/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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
// $Id$

#include <qmultilineedit.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include <kapp.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kprocess.h>

#include "kbugreport.h"

#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

#include <version.h> // konqueror specific !

KBugReport::KBugReport( QWidget * parent )
  : KDialogBase( Plain,
                 i18n("Submit a KDE bug report"),
                 Ok | Cancel,
                 Ok,
                 parent,
                 "KBugReport",
                 true, // modal
                 true // separator
                 )
{
  m_process = 0L;
  QWidget * parent = plainPage();
  QLabel * tmpLabel;
  QVBoxLayout * lay = new QVBoxLayout( parent, KDialog::marginHint(), KDialog::spacingHint() );

  // From
  QHBoxLayout * hlay = new QHBoxLayout( 0L, KDialog::marginHint(), KDialog::spacingHint() );
  tmpLabel = new QLabel( i18n("From : "), parent );
  hlay->addWidget( tmpLabel );
  m_from = new QLabel( parent );
  hlay->addWidget( m_from, 10 );
  QPushButton * configureEmail = new QPushButton( i18n("Configure E-Mail"), parent );
  hlay->addWidget( configureEmail );
  connect( configureEmail, SIGNAL( clicked() ), this, SLOT( slotConfigureEmail() ) );
  lay->addLayout(hlay);

  slotSetFrom();

  // Program name
  tmpLabel = new QLabel( i18n("Application : ")+kapp->name(), parent );
  lay->addWidget( tmpLabel );

  // Version : TODO : take it from the future kdelibs class containing it
  m_strVersion = QString(KONQUEROR_VERSION) +" (KDE "+KDE_VERSION_STRING+")";
  m_version = new QLabel( i18n("Version : ")+m_strVersion, parent );
  lay->addWidget( m_version );

  // Severity
  m_bgSeverity = new QButtonGroup( i18n("Severity"), parent );
  QGridLayout *bgLay = new QGridLayout(m_bgSeverity,2,4,10,5);
  bgLay->addRowSpacing(0,10);
  bgLay->setRowStretch(0,0);
  bgLay->setRowStretch(1,1);
  m_bgSeverity->setExclusive( TRUE );
  const char * sevNames[5] = { "critical", "grave", "normal", "wishlist", "i18n" };
  const QString sevTexts[5] = { i18n("Critical"), i18n("Grave"), i18n("Normal"), i18n("Wishlist"), i18n("Translation") };
  for (int i = 0 ; i < 5 ; i++ )
  {
    // Store the severity string as the name
    QRadioButton * rb = new QRadioButton( sevTexts[i], m_bgSeverity, sevNames[i] );
    bgLay->addWidget(rb,1,i);
    if (i==2) rb->setChecked(true); // default : "normal"
  }
  lay->addWidget( m_bgSeverity );

  // Subject
  hlay = new QHBoxLayout( 0L, KDialog::marginHint(), KDialog::spacingHint() );
  tmpLabel = new QLabel( i18n("Subject : "), parent );
  hlay->addWidget( tmpLabel );
  m_subject = new QLineEdit( parent );
  m_subject->setFocus();
  hlay->addWidget( m_subject );
  lay->addLayout(hlay);

  // A little help label
  QLabel * label = new QLabel(
    i18n("Enter below the text (in English if possible) that you wish to submit for the bug report\n"
         "If you press Ok, and a mail will be sent to the maintainer of this program and to the KDE buglist\n"
         "You can access the previously reported bugs and wishes at http://bugs.kde.org/"), parent, "label");
  lay->addWidget( label );

  // The multiline-edit
  m_lineedit = new QMultiLineEdit( parent, "QMultiLineEdit" );
  m_lineedit->setMinimumHeight( 180 ); // make it big
  lay->addWidget( m_lineedit, 10 /*stretch*/ );
}

KBugReport::~KBugReport()
{
}

void KBugReport::slotConfigureEmail()
{
  m_process = new KProcess;
  *m_process << "kcmshell" << "System/email";
  connect(m_process, SIGNAL(processExited(KProcess *)), this, SLOT(slotSetFrom()));
  if (!m_process->start())
  {
    debug("Couldn't start kcmshell..");
    delete m_process;
  }
}

void KBugReport::slotSetFrom()
{
  delete m_process;
  KConfig emailConf( "emaildefaults" );
  emailConf.setGroup( "UserInfo" );
  struct passwd *p;
  p = getpwuid(getuid());
  m_from->setText( emailConf.readEntry( "EmailAddress", p->pw_name ) );
}

int KBugReport::exec()
{
  int result = KDialogBase::exec();

  if ( result == QDialog::Accepted )
  {
    if ( !sendBugReport() )
      KMessageBox::error(this, i18n("Couldn't send the bug report.\nHmmm, submit a bug report manually, sorry...\n"));
    else
      KMessageBox::information(this, i18n("Bug report sent, thanks for your input."));
  }
  else
  {
    debug("cancelled");
  }
  return result;
}

QString KBugReport::text()
{
  // Prepend the pseudo-headers to the contents of the mail
  QString severity = m_bgSeverity->selected()->name();
  if (severity == "i18n")
    // Case 1 : i18n bug
    return QString("Package: i18n")+"\n"+
      "Severity: Normal\n"+
      "\n"+
      "Application: "+kapp->name()+"\n"+
      "Version: "+m_strVersion+"\n"+ // not really i18n's version, so better here IMHO
      "\n"+
      m_lineedit->text();
  else
    // Case 2 : normal bug
    return QString("Package: ")+kapp->name()+"\n"+
      "Version: "+m_strVersion+"\n"+
      "Severity: "+severity+"\n"+
      "\n"+
      m_lineedit->text();
}

#define RECIPIENT "submit@bugs.kde.org"

bool KBugReport::sendBugReport()
{
  QString command = KStandardDirs::findExe( "sendmail", "/sbin:/usr/sbin:/usr/lib" )+" -oi -t";
  bool needHeaders = true;
  if ( command.isNull() )
  {
    command = "mail -s \x22";
    command.append(m_subject->text());
    command.append("\x22 ");
    command.append(RECIPIENT);
    needHeaders = false;
  }

  FILE * fd = popen(command.local8Bit(),"w");
  if (!fd)
  {
    kdebug(KDEBUG_ERROR,0,"Unable to open a pipe to %s",command.local8Bit().data());
    return false;
  }

  QString textComplete;
  if (needHeaders)
  {
    textComplete += "From: " + m_from->text() + "\n";
    textComplete += "To: " RECIPIENT "\n";
    textComplete += "Subject: " + m_subject->text() + "\n";
  }
  textComplete += "\n"; // end of headers
  textComplete += text();

  fwrite(textComplete.ascii(),textComplete.length(),1,fd);

  pclose(fd);

  return true;
}

#include "kbugreport.moc"
