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
#include <kaboutdata.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kurllabel.h>

#include "kbugreport.h"

#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

KBugReport::KBugReport( QWidget * parentw )
  : KDialogBase( Plain,
                 i18n("Submit a KDE bug report"),
                 Ok | Cancel,
                 Ok,
                 parentw,
                 "KBugReport",
                 true, // modal
                 true // separator
                 )
{
  m_process = 0L;
  QWidget * parent = plainPage();
  QLabel * tmpLabel;
  QVBoxLayout * lay = new QVBoxLayout( parent, 0, spacingHint() );

  QGridLayout *glay = new QGridLayout( lay, 3, 3 );
  glay->setColStretch( 1, 10 );

  // From
  tmpLabel = new QLabel( i18n("From :"), parent );
  glay->addWidget( tmpLabel, 0,0 );
  m_from = new QLabel( parent );
  glay->addWidget( m_from, 0, 1 );
  slotSetFrom();

  // Program name
  tmpLabel = new QLabel( i18n("Application : "), parent );
  glay->addWidget( tmpLabel, 1, 0 );
  tmpLabel = new QLabel( kapp->name(), parent );
  glay->addWidget( tmpLabel, 1, 1 );

  // Version : TODO : take it from the future kdelibs class containing it
  tmpLabel = new QLabel( i18n("Version : "), parent );
  glay->addWidget( tmpLabel, 2, 0 );
  KAboutData * aboutData = KGlobal::instance()->aboutData(); // TODO : use the "active" instance
  if (aboutData) m_strVersion = aboutData->version();
   else m_strVersion = "no version set (programmer error!)";
  m_strVersion += " (KDE " KDE_VERSION_STRING ")";
  m_version = new QLabel( m_strVersion, parent );
  glay->addWidget( m_version, 2, 1 );

  // Configure email button
  QPushButton * configureEmail = new QPushButton( i18n("Configure E-Mail"),
						  parent );
  connect( configureEmail, SIGNAL( clicked() ), this,
	   SLOT( slotConfigureEmail() ) );
  glay->addMultiCellWidget( configureEmail, 0, 2, 2, 2, AlignTop );

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
  QHBoxLayout * hlay = new QHBoxLayout( lay );
  tmpLabel = new QLabel( i18n("Subject : "), parent );
  hlay->addWidget( tmpLabel );
  m_subject = new QLineEdit( parent );
  m_subject->setFocus();
  hlay->addWidget( m_subject );

  QString text = i18n(""
    "Enter the text (in English if possible) that you wish to submit for the "
    "bug report.\n"
    "If you press \"OK\", a mail message will be sent to the maintainer of "
    "this program \n"
    "and to the KDE buglist.");
  QLabel * label = new QLabel( parent, "label" );
  label->setText( text );
  lay->addWidget( label );

  // The multiline-edit
  m_lineedit = new QMultiLineEdit( parent, "QMultiLineEdit" );
  m_lineedit->setMinimumHeight( 180 ); // make it big
  lay->addWidget( m_lineedit, 10 /*stretch*/ );


  hlay = new QHBoxLayout( lay, 0 );

  text = i18n("You can access previously reported bugs and wishes at ");
  label = new QLabel( text, parent, "label");
  hlay->addWidget( label, 0, AlignBottom );

  text = "http://bugs.kde.org/";
  KURLLabel *url = new KURLLabel( parent );
  url->setFloat(true);
  url->setUnderline(true);
  url->setText(text);
  url->setURL(text);
  connect( url, SIGNAL(leftClickedURL(const QString &)),
	   this, SLOT(slotUrlClicked(const QString &)));
  hlay->addWidget( url, 0, AlignBottom );

  hlay->addStretch( 10 );

  // Necessary for vertical label and url alignment.
  label->setFixedHeight( fontMetrics().lineSpacing() );
  url->setFixedHeight( fontMetrics().lineSpacing()-1 );
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

void KBugReport::slotUrlClicked(const QString &urlText)
{
  kapp->invokeBrowser( urlText );
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
