/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/
/**
 * @file User Account Configuration Module (internal name userinfo)
 */
#include <unistd.h>
#include <pwd.h>

#include <qwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qfile.h>
#include <qgroupbox.h>
#include <qimage.h>

#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <kcmodule.h>
#include <kgenericfactory.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kimageio.h>
#include <kconfig.h>
#include <kurldrag.h>
#include <kuser.h>

#include "userinfo.h"
#include "chfnproc.h"
#include "userinfo_chfn.h"
#include "userinfo_chface.h"
#include "userinfo_constants.h"
#include "passwd.h"
#include "passwddlg.h"

/**
 * DLL interface.
 */
typedef KGenericFactory<KUserInfoConfig, QWidget > UserInfoFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_userinfo, UserInfoFactory("userinfo") )

KUserInfoConfig::KUserInfoConfig(QWidget *parent, const char *name, const QStringList &)
  : KCModule(UserInfoFactory::instance(), parent, name)
{
  KGlobal::locale()->insertCatalogue( "kdepasswd" );

  QVBoxLayout *pTop = new QVBoxLayout(this, 0, KDialog::spacingHint());

  // The header (face and name)
  QHBoxLayout *pHeaderLayout = new QHBoxLayout(0, 0, KDialog::spacingHint());

  m_pFaceButton = new QPushButton( this );
  m_pFaceButton->setAcceptDrops( true );
  m_pFaceButton->installEventFilter( this ); // for drag and drop

  QToolTip::add( m_pFaceButton, i18n("Click or drop an image here") );
  QWhatsThis::add( m_pFaceButton,
          i18n("Here you can see the image that represents you in KDM."
               " Click on the image button to select from a list"
               " of images or drag and drop your own image on to the"
               " button (e.g. from Konqueror).") );

  connect( m_pFaceButton, SIGNAL(clicked()), SLOT(slotFaceButtonClicked()) );

  m_pFaceButton->setFixedSize( FACE_BTN_SIZE, FACE_BTN_SIZE );

  m_pHeader = new QLabel( "", this );
  m_pHeader->setAlignment( Qt::AlignVCenter );
  m_pHeader->setMinimumWidth(400);

  pHeaderLayout->addWidget(m_pFaceButton);
  pHeaderLayout->addWidget(m_pHeader);
  pTop->addLayout( pHeaderLayout );

  QHBoxLayout *pMainArea = new QHBoxLayout( 0, 0, KDialog::spacingHint() );

  // The left-hand side information
  QGroupBox *pInfoGroup = new QGroupBox( 0, Qt::Vertical, i18n("Information"), this );

  QHBoxLayout *pInfoLayout = new QHBoxLayout( pInfoGroup->layout(), KDialog::spacingHint() );

  QLabel *pInfoLeft = new QLabel( i18n("Real name:\nUser name:\nUser ID:\nHome folder:\nShell:"), pInfoGroup );
  m_pInfoRight = new QLabel( m_InfoRightText , pInfoGroup );
  pInfoLayout->addWidget( pInfoLeft );
  pInfoLayout->addWidget( m_pInfoRight );

  pMainArea->addWidget( pInfoGroup );

  // The Right-hand settings buttons
  QGroupBox *pSettingsGroup = new QGroupBox( i18n("Settings"), this );

  QVBoxLayout *pSettingsLayout = new QVBoxLayout( pSettingsGroup, KDialog::marginHint(), KDialog::spacingHint() );

  QPushButton *pChangeNameBtn = new QPushButton( i18n("Change Real &Name..."), pSettingsGroup );
  connect( pChangeNameBtn, SIGNAL(clicked()), SLOT(slotChangeRealName()) );
  QPushButton *pChangePasswordBtn = new QPushButton( i18n("Change Pass&word..."), pSettingsGroup );
  connect( pChangePasswordBtn, SIGNAL(clicked()), SLOT(slotChangePassword()) );
  pSettingsLayout->addWidget( pChangeNameBtn );
  pSettingsLayout->addWidget( pChangePasswordBtn );
  QWhatsThis::add( pChangeNameBtn, i18n("Click on this button to change your real name. Your password will be required.") );
  QWhatsThis::add( pChangePasswordBtn, i18n("Click on this button to change your password.") );

  pMainArea->addWidget( pSettingsGroup );

  pTop->addLayout( pMainArea );
  pTop->addStretch();

  load();// Load our settings
}


// Convenience Function
inline void KUserInfoConfig::changeFace(const QString &path)
{
  changeFace( QPixmap( path ) );
}

void KUserInfoConfig::changeFace(const QPixmap &pix)
{
  if ( m_FaceSource < FACE_SRC_USERFIRST )
    return;// If the user isn't allowed to change their face, don't!

  if ( pix.isNull() ) {
    KMessageBox::sorry( this, i18n("There was an error loading the image.") );
    return;
  }

  m_FacePixmap = pix;
  m_pFaceButton->setPixmap( m_FacePixmap );
  emit changed( true );
}

void KUserInfoConfig::slotFaceButtonClicked()
{
  if ( m_FaceSource < FACE_SRC_USERFIRST )
  {
    KMessageBox::sorry( this, i18n("Your administrator has disallowed changing your face.") );
    return;
  }
  KUserInfoChFaceDlg* pDlg = new KUserInfoChFaceDlg( m_facesDir );

  if ( pDlg->exec() == QDialog::Accepted )
  {
    if ( !pDlg->getFaceImage().isNull() )
      changeFace( pDlg->getFaceImage() );
  }
  delete pDlg;
}

bool KUserInfoConfig::eventFilter(QObject *, QEvent *e)
{
  if (e->type() == QEvent::DragEnter)
  {
    QDragEnterEvent *ee = (QDragEnterEvent *) e;
    ee->accept( KURLDrag::canDecode(ee) );
    return true;
  }

  if (e->type() == QEvent::Drop)
  {
    faceButtonDropEvent((QDropEvent *) e);
    return true;
  }

  return false;
}

KURL *decodeImgDrop(QDropEvent *e, QWidget *wdg)
{
  KURL::List uris;

  if (KURLDrag::decode(e, uris) && (uris.count() > 0))
  {
    KURL *url = new KURL(uris.first());

    KImageIO::registerFormats();
    if( KImageIO::canRead(KImageIO::type(url->fileName())) )
      return url;

    QStringList qs = QStringList::split('\n', KImageIO::pattern());
    qs.remove(qs.begin());

    QString msg = i18n( "%1 does not appear to be an image file.\n"
			  "Please use files with these extensions:\n"
			  "%2").arg(url->fileName()).arg(qs.join("\n"));
    KMessageBox::sorry( wdg, msg);
    delete url;
  }
  return 0;
}

void KUserInfoConfig::faceButtonDropEvent(QDropEvent *e)
{
  if ( m_FaceSource < FACE_SRC_USERFIRST )
  {
    KMessageBox::sorry( this, i18n("Your administrator has disallowed changing your face.") );
    return;
  }

  KURL *url = decodeImgDrop(e, this);
  if (url)
  {
    QString pixpath;
    KIO::NetAccess::download(*url, pixpath);
    changeFace( pixpath );
    KIO::NetAccess::removeTempFile(pixpath);
    delete url;
  }
}

void KUserInfoConfig::slotChangeRealName()
{
  KUserInfoChFnDlg* pDlg = new KUserInfoChFnDlg( &m_UserName, &m_FullName, this );
  if ( pDlg->exec() ) {
    ChfnProcess* chfnproc = new ChfnProcess();
    int ret = chfnproc->exec( pDlg->getpass().latin1(), pDlg->getname().latin1() );
    if ( ret == 0 ) {
      m_FullName = pDlg->getname();
      m_HeaderText = m_FullName.isEmpty() ?
		"<font size=\"7\"><b>" + m_UserName + "</b></font>" :
		"<font size=\"5\"><b>" + m_FullName + "</b></font><br>\n(" + m_UserName + ")";
      m_pHeader->setText( m_HeaderText );

      m_InfoRightText = m_FullName.isEmpty() ?
	  m_UserName + "\n" + m_UserName + "\n" + m_UserID + "\n" + m_HomeDir + "\n" + m_Shell :
	  m_FullName + "\n" + m_UserName + "\n" + m_UserID + "\n" + m_HomeDir + "\n" + m_Shell;
      m_pInfoRight->setText( m_InfoRightText );

      //KMessageBox::information( this, i18n("Your name has been changed!") );
    }
    else if ( ret == ChfnProcess::PasswordError )
      KMessageBox::sorry( this, i18n("Your password was wrong.").arg( chfnproc->error() ) );
    else if ( ret == ChfnProcess::MiscError )
      KMessageBox::sorry( this, i18n("An error occurred: %1").arg( chfnproc->error() ) );
    else
      KMessageBox::sorry( this, i18n("An unknown error occurred.") );
    delete chfnproc;
  }
  delete pDlg;
}

void KUserInfoConfig::slotChangePassword()
{
  QCString user(m_UserName.latin1());
  QCString oldpass;
  int result = KDEpasswd1Dialog::getPassword(oldpass);
  if (result != KDEpasswd1Dialog::Accepted)
    return;

  KDEpasswd2Dialog *dlg = new KDEpasswd2Dialog(oldpass, user);
  dlg->exec();
  delete dlg;
}

void KUserInfoConfig::defaults()
{
  changeFace( m_userPicsDir + SYS_DEFAULT_FILE );
  emit changed(true);
}

void KUserInfoConfig::save()
{
  QString userpix = m_HomeDir + USER_FACE_FILE;
  if (!m_FacePixmap.save( userpix, "PNG" ))
      KMessageBox::error(this, i18n("There was an error saving the image:\n%1").arg( userpix ) );
  emit changed(false);
}

void KUserInfoConfig::load()
{
  // Get face dirs
  m_facesDir =  KGlobal::dirs()->resourceDirs("data").last() + "kdm/" + KDM_FACES_DIR;

  KConfig *kdmconfig = new KConfig("kdm/kdmrc", true);
  kdmconfig->setGroup("X-*-Greeter");
  m_userPicsDir = kdmconfig->readEntry( "FaceDir", KGlobal::dirs()->resourceDirs("data").last() + "kdm/faces" ) + '/';
  QString fs = kdmconfig->readEntry( "FaceSource" );
  if (fs == QString::fromLatin1("UserOnly"))
    m_FaceSource = FACE_SRC_USERONLY;
  else if (fs == QString::fromLatin1("PreferUser"))
    m_FaceSource = FACE_SRC_USERFIRST;
  else if (fs == QString::fromLatin1("PreferAdmin"))
    m_FaceSource = FACE_SRC_ADMINFIRST;
  else
    m_FaceSource = FACE_SRC_ADMINONLY; // Admin Only
  delete kdmconfig;

  // Read user information
  KUser user;
  m_UserName = user.loginName();

  m_FullName = user.fullName();
  m_HomeDir = user.homeDir();
  m_Shell = user.shell();
  m_UserID = m_UserID.setNum( user.uid() );

  m_HeaderText = m_FullName.isEmpty() ?
			"<font size=\"7\"><b>" + m_UserName + "</b></font>" :
			"<font size=\"5\"><b>" + m_FullName + "</b></font><br>\n(" + m_UserName + ")";
  m_pHeader->setText( m_HeaderText );
  m_InfoRightText = m_FullName.isEmpty() ?
			m_UserName + "\n" + m_UserName + "\n" + m_UserID + "\n" + m_HomeDir + "\n" + m_Shell:
			m_FullName + "\n" + m_UserName + "\n" + m_UserID + "\n" + m_HomeDir + "\n" + m_Shell;
  m_pInfoRight->setText( m_InfoRightText );

  if ( m_FaceSource == FACE_SRC_ADMINFIRST )
  {
    // If the administrator's choice takes preference
    m_FacePixmap = QPixmap( m_userPicsDir + m_UserName + ".face.icon" );
    if ( m_FacePixmap.isNull() )
      m_FaceSource = FACE_SRC_USERONLY;
    else
      m_pFaceButton->setPixmap( m_FacePixmap );
  }
  if ( m_FaceSource >= FACE_SRC_USERFIRST)
  {
    // If the user's choice takes preference
    m_FacePixmap = QPixmap( m_HomeDir + USER_FACE_FILE );
    if ( m_FacePixmap.isNull() && m_FaceSource == FACE_SRC_USERFIRST ) // The user has no face, should we check for the admin's setting?
       m_FacePixmap = QPixmap( m_userPicsDir + m_UserName + ".face.icon" );
    if ( m_FacePixmap.isNull() )
      m_FacePixmap = QPixmap( m_userPicsDir + SYS_DEFAULT_FILE );

    m_pFaceButton->setPixmap( m_FacePixmap );
  }
  else if ( m_FaceSource <= FACE_SRC_ADMINONLY )
  {
    // Admin only
    m_FacePixmap = QPixmap( m_userPicsDir + m_UserName + ".face.icon" );
    if ( m_FacePixmap.isNull() )
      m_FacePixmap = QPixmap( m_userPicsDir + SYS_DEFAULT_FILE );
    m_pFaceButton->setPixmap( m_FacePixmap );
  }
}

int KUserInfoConfig::buttons()
{
  return KCModule::Help | KCModule::Apply |  KCModule::Default;
}

QString KUserInfoConfig::quickHelp() const
{
  return i18n("<h1>User Account</h1> This module gives you options for"
     " changing basic user information such as your full name, your"
     " password, and the icon which represents your account when"
     " you login.");
}

const KAboutData* KUserInfoConfig::aboutData() const
{
  KAboutData *about =
  new KAboutData(I18N_NOOP("kcmuserinfo"), I18N_NOOP("UserInfo Control Module"),
                0, 0, KAboutData::License_GPL,
                I18N_NOOP("(c) 2002, Braden MacDonald"));

  about->addAuthor("Braden MacDonald", I18N_NOOP("Face editor"), "bradenm_k@shaw.ca");
  about->addAuthor("Geert Jansen", I18N_NOOP("Password changer"), "jansen@kde.org", "http://www.stack.nl/~geertj/");
  about->addAuthor("Ravikiran Rajagopal", I18N_NOOP("Maintainer"), "ravi@ee.eng.ohio-state.edu");

  return about;
}

#include "userinfo.moc"
