// ksmboptdlg.h - SMB configuration by Nicolas Brodu <brodu@kde.org>
// code adapted from other option dialogs

#include <qlayout.h>
#include <kbuttonbox.h>

#include <kapp.h>

#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>

#include "ksmboptdlg.h"


KSMBOptions::KSMBOptions(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
	QGridLayout *topLayout = new QGridLayout(this,4,1,10,0,"smbOptTopLayout");
	labelInfo1 = new QLabel(i18n("Note: konqueror is a SMB client only."), this);
	topLayout->addWidget(labelInfo1,0,0);
	labelInfo2 = new QLabel(i18n("The server, if any, cannot be configured from here."), this);
	topLayout->addWidget(labelInfo2,1,0);
	topLayout->addRowSpacing(2,10);
	
	QGridLayout *subLayout = new QGridLayout(9,5,10,"smbOptSubLayout");
	
	subLayout->addRowSpacing(0,10);
	subLayout->addRowSpacing(3,10);
	subLayout->addRowSpacing(8,10);
	subLayout->addColSpacing(0,10);
	subLayout->addColSpacing(4,10);

	subLayout->setRowStretch(0,0);
	subLayout->setRowStretch(1,1);
	subLayout->setRowStretch(2,1);
	subLayout->setRowStretch(3,0);
	subLayout->setRowStretch(4,1);
	subLayout->setRowStretch(5,1);
	subLayout->setRowStretch(6,1);
	subLayout->setRowStretch(7,1);
	subLayout->setRowStretch(8,0);
	
	subLayout->setColStretch(0,0);
	subLayout->setColStretch(1,1);
	subLayout->setColStretch(2,1);
	subLayout->setColStretch(3,1);
	subLayout->setColStretch(4,0);

	labelNet = new QLabel(i18n("Network settings:"), this);
	subLayout->addWidget(labelNet,1,1);

	labelBrowseServer = new QLabel(i18n("Browse server:"), this);
	subLayout->addWidget(labelBrowseServer,1,2);

	editBrowseServer = new QLineEdit(this);
	subLayout->addWidget(editBrowseServer,1,3);
	connect(editBrowseServer, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

	labelBroadcast = new QLabel(i18n("Broadcast address:"), this);
	subLayout->addWidget(labelBroadcast,2,2);

	editBroadcast = new QLineEdit(this);
	subLayout->addWidget(editBroadcast,2,3);
	connect(editBroadcast, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

	labelUser = new QLabel(i18n("User settings:"), this);
	subLayout->addWidget(labelUser,4,1);

	labelDefaultUser = new QLabel(i18n("Default user name:"), this);
	subLayout->addWidget(labelDefaultUser,4,2);

	editDefaultUser = new QLineEdit(this);
	subLayout->addWidget(editDefaultUser,4,3);
	connect(editDefaultUser, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

	labelPassword = new QLabel(i18n("Password policy:"), this);
	subLayout->addWidget(labelPassword,5,2);

	groupPassword = new QButtonGroup(i18n("Remember password"), this );
	groupPassword->setExclusive( TRUE );
	
	QGridLayout *bgLay = new QGridLayout(groupPassword,3,1,15,5);
	radioPolicyAsk = new QRadioButton( i18n("Ask"), groupPassword );
	bgLay->addWidget(radioPolicyAsk, 0, 0);
	connect(radioPolicyAsk, SIGNAL(clicked()), this, SLOT(changed()));

	radioPolicyNever = new QRadioButton( i18n("Never"), groupPassword );
	bgLay->addWidget(radioPolicyNever, 1, 0);
	connect(radioPolicyNever, SIGNAL(clicked()), this, SLOT(changed()));

	radioPolicyAlways = new QRadioButton( i18n("Always"), groupPassword );
	bgLay->addWidget(radioPolicyAlways, 2, 0);
	connect(radioPolicyAlways, SIGNAL(clicked()), this, SLOT(changed()));

	bgLay->activate();
	
	subLayout->addWidget(groupPassword,5,3);
	topLayout->addLayout(subLayout,3,0);
//	subLayout->activate();
	
	topLayout->activate();

	// finaly read the options
	load();
}

KSMBOptions::~KSMBOptions()
{
}

void KSMBOptions::load()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

	QString tmp;
	g_pConfig->setGroup( "Browser Settings/SMB" );
	tmp = g_pConfig->readEntry( "Browse server" );
	editBrowseServer->setText( tmp );
	tmp = g_pConfig->readEntry( "Broadcast address" );
	editBroadcast->setText( tmp );
	tmp = g_pConfig->readEntry( "Default user" );
	editDefaultUser->setText( tmp );
	tmp = g_pConfig->readEntry( "Remember password" );
	// more ifs, but less QString comparisons
	// Default is to ask
	if ( tmp == "Never" ) {
		radioPolicyNever->setChecked(true);
		radioPolicyAlways->setChecked(false);
		radioPolicyAsk->setChecked(false);
	} else {
		radioPolicyNever->setChecked(false);
		if ( tmp == "Always" ) {
			radioPolicyAlways->setChecked(true);
			radioPolicyAsk->setChecked(false);
		} else {
			radioPolicyAlways->setChecked(false);
			radioPolicyAsk->setChecked(true);
		}
	}

  delete g_pConfig;
}

void KSMBOptions::save()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

	g_pConfig->setGroup( "Browser Settings/SMB" );	
	g_pConfig->writeEntry( "Browse server", editBrowseServer->text());
	g_pConfig->writeEntry( "Broadcast address", editBroadcast->text());
	g_pConfig->writeEntry( "Default user", editDefaultUser->text());
	if (radioPolicyAlways->isChecked())
		g_pConfig->writeEntry( "Remember password", "Always");
	else if (radioPolicyNever->isChecked())
		g_pConfig->writeEntry( "Remember password", "Never");
	else g_pConfig->writeEntry( "Remember password", "Ask");
	g_pConfig->sync();

  delete g_pConfig;
}

void KSMBOptions::defaults()
{
	radioPolicyAsk->setChecked(true);
	radioPolicyNever->setChecked(false);
	radioPolicyAlways->setChecked(false);
	editBrowseServer->setText( "" );
	editBroadcast->setText( "" );
	editDefaultUser->setText( "" );
}


void KSMBOptions::changed()
{
  emit KCModule::changed(true);
}


#include "ksmboptdlg.moc"
