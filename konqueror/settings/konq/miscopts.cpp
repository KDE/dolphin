//
//
// "Misc Options" Tab for Konqueror configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998-2000

#include <qlabel.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qgroupbox.h>
#include <qlayout.h>//CT - 12Nov1998
#include <qwhatsthis.h>
#include <kapp.h>

#include "miscopts.h"

#include <kdialog.h>
#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <dcopclient.h>
#include <kconfig.h>
#include <uiserver_stub.h>


//-----------------------------------------------------------------------------

KMiscOptions::KMiscOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule( parent, name ), g_pConfig(config), groupname(group)
{
    QVBoxLayout *lay = new QVBoxLayout(this, KDialog::marginHint(),
				       KDialog::spacingHint());

/*
    urlpropsbox = new QCheckBox(i18n("&Allow per-URL settings"), this);
    lay->addWidget(urlpropsbox);
    connect(urlpropsbox, SIGNAL(clicked()), this, SLOT(changed()));

    QWhatsThis::add( urlpropsbox, i18n("Checking this option allows you to customize"
       " Konqueror settings separately for each URL. For example, you can configure"
       " different views for different directories. (Note that you need to have"
       " write access in those directories in order to save settings.)") );
*/

    QGroupBox *gbox = new QGroupBox(i18n("Preferred Programs"), this);
    lay->addWidget(gbox);

    QGridLayout *grid = new QGridLayout(gbox, 0, 2,
					 KDialog::marginHint(),
					 KDialog::spacingHint());
    grid->addRowSpacing(0, gbox->fontMetrics().height());
    grid->setColStretch(1, 1);

    QLabel * label = new QLabel(i18n("Terminal program:"), gbox);
    grid->addWidget(label, 1, 0);

    leTerminal = new QLineEdit(gbox);
    grid->addWidget(leTerminal, 1, 1);
    connect(leTerminal, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

    QString wtstr = i18n("Enter the name of your favourite terminal program here,"
       " for example, konsole or xterm. This program will be used when you open"
       " a terminal window from Konqueror by selecting \"Open Terminal ...\","
       " or by using the Ctrl+T shortcut.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( leTerminal, wtstr );

    cbListProgress = new QCheckBox( i18n( "&Show network operations in a single window" ), this );
    lay->addWidget( cbListProgress );
    connect(cbListProgress, SIGNAL(clicked()), this, SLOT(changed()));

    QWhatsThis::add( cbListProgress, i18n("Checking this option will group the"
       " progress information for all network file transfers into a single window"
       " with a list. When the option is not checked, all transfers appear in a"
       " separate window.") );

    lay->addStretch(10);
    lay->activate();

    load();
}

void KMiscOptions::load()
{
    // *** load ***
    g_pConfig->setGroup( groupname );
    QString sTerminal = g_pConfig->readEntry( "Terminal", DEFAULT_TERMINAL );

    KConfig config("uiserverrc");
    config.setGroup( "UIServer" );

    bool bShowList = config.readBoolEntry( "ShowList", false );

    // *** apply to GUI ***

    leTerminal->setText(sTerminal);
    cbListProgress->setChecked( bShowList );
}

void KMiscOptions::defaults()
{
    leTerminal->setText(DEFAULT_TERMINAL);
    cbListProgress->setChecked( false );
}

void KMiscOptions::save()
{
    g_pConfig->setGroup( groupname );
    g_pConfig->writeEntry( "Terminal", leTerminal->text());
    g_pConfig->sync();

    KConfig config("uiserverrc");
    config.setGroup( "UIServer" );
    config.writeEntry( "ShowList", cbListProgress->isChecked() );
    config.sync();
    // Tell the running server
    if ( kapp->dcopClient()->isApplicationRegistered( "kio_uiserver" ) )
    {
      UIServer_stub uiserver( "kio_uiserver", "UIServer" );
      uiserver.setListMode( cbListProgress->isChecked() );
    }
}

void KMiscOptions::changed()
{
    emit KCModule::changed(true);
}

#include "miscopts.moc"
