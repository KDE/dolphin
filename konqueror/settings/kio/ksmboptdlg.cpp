// ksmboptdlg.h - SMB configuration by Nicolas Brodu <brodu@kde.org>
// code adapted from other option dialogs

#include <qlayout.h>
#include <kbuttonbox.h>

#include <kapp.h>
#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>

#include "ksmboptdlg.h"


KSMBOptions::KSMBOptions(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this,
					     KDialog::marginHint(),
					     KDialog::spacingHint(),
					     "smbOptTopLayout");
    labelInfo1 = new QLabel(i18n("Note: The Konqueror browser is only a windows SMB client."), this);
    topLayout->addWidget(labelInfo1);
    labelInfo2 = new QLabel(i18n("If you want to export Windows shares, you must use the SAMBA configuration screen."), this);
    topLayout->addWidget(labelInfo2);
    topLayout->addSpacing(KDialog::spacingHint());
	
    QGridLayout *subLayout;
	
    // =======================
    //   net groupbox
    // =======================

    groupNet = new QGroupBox(i18n("Network settings:"), this);
    subLayout = new QGridLayout(groupNet,1,1,
				KDialog::marginHint(),
				KDialog::spacingHint(),
				"smbOptNetSubLayout");
    
    subLayout->addRowSpacing(0, fontMetrics().lineSpacing());

    topLayout->addWidget(groupNet);
    topLayout->addSpacing(KDialog::spacingHint());
	
    labelBrowseServer = new QLabel(i18n("Browse server:"), groupNet);
    subLayout->addWidget(labelBrowseServer,1,0);

    editBrowseServer = new QLineEdit(groupNet);
    subLayout->addWidget(editBrowseServer,1,1);
    connect(editBrowseServer, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

    labelBroadcast = new QLabel(i18n("Broadcast address:"), groupNet);
    subLayout->addWidget(labelBroadcast,2,0);
    
    editBroadcast = new QLineEdit(groupNet);
    subLayout->addWidget(editBroadcast,2,1);
    connect(editBroadcast, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

    // =======================
    //   user groupbox
    // =======================
    
    groupUser = new QGroupBox(i18n("User settings:"), this);
    subLayout = new QGridLayout(groupUser,1,1,
				KDialog::marginHint(),
				KDialog::spacingHint(),
				"smbOptUserSubLayout");
    subLayout->addRowSpacing(0,fontMetrics().lineSpacing());

    topLayout->addWidget(groupUser);

    labelDefaultUser = new QLabel(i18n("Default user name:"), groupUser);
    subLayout->addWidget(labelDefaultUser,1,0);
    
    editDefaultUser = new QLineEdit(groupUser);
    subLayout->addWidget(editDefaultUser,1,2);
    connect(editDefaultUser, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
    
    labelPassword = new QLabel(i18n("Remember password:"), groupUser );
    subLayout->addWidget(labelPassword, 2, 0);
    
    groupPassword = new QButtonGroup("",groupUser);
    groupPassword->setFrameStyle(QFrame::Box | QFrame::Plain);
    groupPassword->setLineWidth(0);
    groupPassword->setExclusive( TRUE );
    subLayout->addWidget(groupPassword, 2, 2);
	
    QGridLayout *bgLay = new QGridLayout(groupPassword,1,1,
					 KDialog::marginHint(),
					 KDialog::spacingHint());
    radioPolicyAsk = new QRadioButton( i18n("Ask"), groupPassword );
    bgLay->addWidget(radioPolicyAsk, 0, 0);
    connect(radioPolicyAsk, SIGNAL(clicked()), this, SLOT(changed()));
    
    radioPolicyNever = new QRadioButton( i18n("Never"), groupPassword );
    bgLay->addWidget(radioPolicyNever, 0, 1);
    connect(radioPolicyNever, SIGNAL(clicked()), this, SLOT(changed()));
    
    radioPolicyAlways = new QRadioButton( i18n("Always"), groupPassword );
    bgLay->addWidget(radioPolicyAlways, 0, 2);
    connect(radioPolicyAlways, SIGNAL(clicked()), this, SLOT(changed()));
    
    bgLay->setColStretch(0,1);
    bgLay->setColStretch(1,1);
    bgLay->setColStretch(2,1);
    
    topLayout->addStretch(1);

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

QString KSMBOptions::quickHelp()
{
    return i18n("<h1>Windows Shares</h1>Konqueror is able to access shared "
		"windows filesystems if properly configured.  If there is a "
		"specific computer from which you want to browse, fill in "
		"the <em>Browse server</em> field.  Otherwise, to retrieve "
		"available shares from your entire LAN, use the "
		"<em>Broadcast</em> field.<p>"
		"If you must access windows shares with a different username "
		"than your login name, fill in the <em>Default user "
		"name</em> field.  You may also choose whether you want "
		"your password to be remembered, or if you want to be "
		"prompted each time you connect to a new shared drive. "
		"This may be preferable for security reasons.");
}

#include "ksmboptdlg.moc"
