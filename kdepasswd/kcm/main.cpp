
/**
 *  Copyright (C) 2004 Frans Englich <frans.englich@telia.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *
 *
 *  Please see the README
 *
 */

#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QEvent>
#include <QPixmap>
#include <QStringList>
#include <QLayout>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <QDropEvent>

#include <kpushbutton.h>
#include <kguiitem.h>
#include <kemailsettings.h>
#include <kpassworddialog.h>
#include <kuser.h>
#include <kdialog.h>
#include <kimageio.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kgenericfactory.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kio/netaccess.h>
#include <kurl.h>
#include <k3urldrag.h>

#include "settings.h"
#include "pass.h"
#include "chfnprocess.h"
#include "chfacedlg.h"
#include "main.h"

typedef KGenericFactory<KCMUserAccount, QWidget> Factory;
K_EXPORT_COMPONENT_FACTORY( useraccount, Factory("useraccount") )

KCMUserAccount::KCMUserAccount( QWidget *parent, const QStringList &)
	: KCModule( Factory::instance(), parent)
{
	QVBoxLayout *topLayout = new QVBoxLayout(this);
	_mw = new MainWidget(this);
	topLayout->addWidget( _mw );

	connect( _mw->btnChangeFace, SIGNAL(clicked()), SLOT(slotFaceButtonClicked()));
	connect( _mw->btnChangePassword, SIGNAL(clicked()), SLOT(slotChangePassword()));
	_mw->btnChangePassword->setGuiItem( KGuiItem( i18n("Change &Password..."), "password" ));

	connect( _mw->leRealname, SIGNAL(textChanged(const QString&)), SLOT(changed()));
	connect( _mw->leOrganization, SIGNAL(textChanged(const QString&)), SLOT(changed()));
	connect( _mw->leEmail, SIGNAL(textChanged(const QString&)), SLOT(changed()));
	connect( _mw->leSMTP, SIGNAL(textChanged(const QString&)), SLOT(changed()));

	_ku = new KUser();
	_kes = new KEMailSettings();

	_mw->lblUsername->setText( _ku->loginName() );
	_mw->lblUID->setText( QString().number(_ku->uid()) );

	KAboutData *about = new KAboutData(I18N_NOOP("kcm_useraccount"),
		I18N_NOOP("Password & User Information"), 0, 0,
		KAboutData::License_GPL,
		I18N_NOOP("(C) 2002, Braden MacDonald, "
			"(C) 2004 Ravikiran Rajagopal"));

	about->addAuthor("Frans Englich", I18N_NOOP("Maintainer"), "frans.englich@telia.com");
	about->addAuthor("Ravikiran Rajagopal", 0, "ravi@kde.org");
	about->addAuthor("Michael H\303\244ckel", "haeckel@kde.org" );

	about->addAuthor("Braden MacDonald", I18N_NOOP("Face editor"), "bradenm_k@shaw.ca");
	about->addAuthor("Geert Jansen", I18N_NOOP("Password changer"), "jansen@kde.org",
			"http://www.stack.nl/~geertj/");
	about->addAuthor("Daniel Molkentin");
	about->addAuthor("Alex Zepeda");
	about->addAuthor("Hans Karlsson", I18N_NOOP("Icons"), "karlsson.h@home.se");
	about->addAuthor("Hermann Thomas", I18N_NOOP("Icons"), "h.thomas@gmx.de");
	setAboutData(about);

	setQuickHelp( i18n("<qt>Here you can change your personal information, which "
			"will be used in mail programs and word processors, for example. You can "
			"change your login password by clicking <em>Change Password</em>.</qt>") );

	addConfig( KCFGPassword::self(), this );
	load();
}

void KCMUserAccount::slotChangePassword()
{
	KProcess *proc = new KProcess;
	QString bin = KGlobal::dirs()->findExe("kdepasswd");
	if ( bin.isNull() )
	{
		kDebug() << "kcm_useraccount: kdepasswd was not found." << endl;
		KMessageBox::sorry ( this, i18n( "A program error occurred: the internal "
			"program 'kdepasswd' could not be found. You will "
			"not be able to change your password."));

		_mw->btnChangePassword->setEnabled(false);
		return;
	}

	*proc << bin << _ku->loginName() ;
	proc->start(KProcess::DontCare);

	delete proc;

}


KCMUserAccount::~KCMUserAccount()
{
	delete _ku;
	delete _kes;
}

void KCMUserAccount::load()
{
	_mw->lblUsername->setText(_ku->loginName());

	_kes->setProfile(_kes->defaultProfileName());

	_mw->leRealname->setText( _kes->getSetting( KEMailSettings::RealName ));
	_mw->leEmail->setText( _kes->getSetting( KEMailSettings::EmailAddress ));
	_mw->leOrganization->setText( _kes->getSetting( KEMailSettings::Organization ));
	_mw->leSMTP->setText( _kes->getSetting( KEMailSettings::OutServer ));

	QString _userPicsDir = KCFGUserAccount::faceDir() +
		KGlobal::dirs()->resourceDirs("data").last() + "kdm/faces/";

	QString fs = KCFGUserAccount::faceSource();
	if (fs == QLatin1String("UserOnly"))
		_facePerm = userOnly;
	else if (fs == QLatin1String("PreferUser"))
		_facePerm = userFirst;
	else if (fs == QLatin1String("PreferAdmin"))
		_facePerm = adminFirst;
	else
		_facePerm = adminOnly; // Admin Only

	if ( _facePerm == adminFirst )
	{ 	// If the administrator's choice takes preference
		_facePixmap = QPixmap( _userPicsDir + _ku->loginName() + ".face.icon" );

		if ( _facePixmap.isNull() )
			_facePerm = userFirst;
		else
			_mw->btnChangeFace->setIcon( _facePixmap );
	}

	if ( _facePerm >= userFirst )
	{
		// If the user's choice takes preference
		_facePixmap = QPixmap( KCFGUserAccount::faceFile() );

		// The user has no face, should we check for the admin's setting?
		if ( _facePixmap.isNull() && _facePerm == userFirst )
			_facePixmap = QPixmap( _userPicsDir + _ku->loginName() + ".face.icon" );

		if ( _facePixmap.isNull() )
			_facePixmap = QPixmap( _userPicsDir + KCFGUserAccount::defaultFile() );

		_mw->btnChangeFace->setIcon( _facePixmap );
	}
	else if ( _facePerm <= adminOnly )
	{
		// Admin only
		_facePixmap = QPixmap( _userPicsDir + _ku->loginName() + ".face.icon" );
		if ( _facePixmap.isNull() )
			_facePixmap = QPixmap( _userPicsDir + KCFGUserAccount::defaultFile() );
		_mw->btnChangeFace->setIcon( _facePixmap );
	}

	KCModule::load(); /* KConfigXT */

}

void KCMUserAccount::save()
{
	KCModule::save(); /* KConfigXT */

	/* Save KDE's homebrewn settings */
	_kes->setSetting( KEMailSettings::RealName, _mw->leRealname->text() );
	_kes->setSetting( KEMailSettings::EmailAddress, _mw->leEmail->text() );
	_kes->setSetting( KEMailSettings::Organization, _mw->leOrganization->text() );
	_kes->setSetting( KEMailSettings::OutServer, _mw->leSMTP->text() );

	/* Save realname to /etc/passwd */
	if ( _mw->leRealname->isModified() )
	{
		QByteArray password;
		int ret = KPasswordDialog::getPassword( _mw, password, i18n("Please enter "
			"your password in order to save your settings:"));

		if ( !ret )
		{
			KMessageBox::sorry( this, i18n("You must enter "
				"your password in order to change your information."));
			return;
		}

		ChfnProcess *proc = new ChfnProcess();
		ret = proc->exec(password, _mw->leRealname->text().toAscii() );
		if ( ret )
			{
			if ( ret == ChfnProcess::PasswordError )
				KMessageBox::sorry( this, i18n("You must enter a correct password."));

			else
			{
				KMessageBox::sorry( this, i18n("An error occurred and your password has "
							"probably not been changed. The error "
							"message was:\n%1", QString::fromLocal8Bit(proc->error())));
				kDebug() << "ChfnProcess->exec() failed. Error code: " << ret
					<< "\nOutput:" << proc->error() << endl;
			}
		}

		delete proc;
	}

	/* Save the image */
	if( !_facePixmap.save( KCFGUserAccount::faceFile(), "PNG" ))
		KMessageBox::error( this, i18n("There was an error saving the image: %1" ,
			KCFGUserAccount::faceFile()) );

}

void KCMUserAccount::changeFace(const QPixmap &pix)
{
  if ( _facePerm < userFirst )
    return; // If the user isn't allowed to change their face, don't!

  if ( pix.isNull() ) {
    KMessageBox::sorry( this, i18n("There was an error loading the image.") );
    return;
  }

  _facePixmap = pix;
  _mw->btnChangeFace->setIcon( _facePixmap );
  emit changed( true );
}

void KCMUserAccount::slotFaceButtonClicked()
{
  if ( _facePerm < userFirst )
  {
    KMessageBox::sorry( this, i18n("Your administrator has disallowed changing your image.") );
    return;
  }

  ChFaceDlg* pDlg = new ChFaceDlg( KGlobal::dirs()->resourceDirs("data").last() +
	"/kdm/pics/users/" );

  if ( pDlg->exec() == QDialog::Accepted && !pDlg->getFaceImage().isNull() )
      changeFace( pDlg->getFaceImage() );

  delete pDlg;
}

/**
 * I merged faceButtonDropEvent into this /Frans
 * The function was called after checking event type and
 * the code is now below that if statement
 */
bool KCMUserAccount::eventFilter(QObject *, QEvent *e)
{
	if (e->type() == QEvent::DragEnter)
		{
		QDragEnterEvent *ee = (QDragEnterEvent *) e;
    if ( K3URLDrag::canDecode( ee ) )
      ee->accept();
    else
      ee->ignore();
		return true;
	}

	if (e->type() == QEvent::Drop)
	{
		if ( _facePerm < userFirst )
		{
			KMessageBox::sorry( this, i18n("Your administrator "
				"has disallowed changing your image.") );
			return true;
		}

		KUrl *url = decodeImgDrop( (QDropEvent *) e, this);
		if (url)
		{
			QString pixPath;
			KIO::NetAccess::download(*url, pixPath, this);
			changeFace( QPixmap( pixPath ) );
			KIO::NetAccess::removeTempFile(pixPath);
			delete url;
		}
		return true;
	}
	return false;
}

inline KUrl *KCMUserAccount::decodeImgDrop(QDropEvent *e, QWidget *wdg)
{
  KUrl::List uris;

  if (K3URLDrag::decode(e, uris) && (uris.count() > 0))
  {
    KUrl *url = new KUrl(uris.first());

    KMimeType::Ptr mime = KMimeType::findByUrl( *url );
    if ( mime && KImageIO::isSupported( mime->name(), KImageIO::Reading ) )
      return url;

    QStringList qs = KImageIO::pattern().split( '\n');
    qs.erase(qs.begin());

    QString msg = i18n( "%1 does not appear to be an image file.\n"
			  "Please use files with these extensions:\n"
			  "%2", url->fileName(), qs.join("\n"));
    KMessageBox::sorry( wdg, msg);
    delete url;
  }
  return 0;
}

#include "main.moc"

