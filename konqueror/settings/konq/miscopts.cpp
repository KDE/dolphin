//
//
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>//CT - 12Nov1998
#include <qwhatsthis.h>
#include <kapp.h>

#include "miscopts.h"

#include <kdialog.h>
#include <konqdefaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <kconfig.h>


//-----------------------------------------------------------------------------

KMiscOptions::KMiscOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule( parent, name ), g_pConfig(config), groupname(group)
{
    QVBoxLayout *lay = new QVBoxLayout(this, KDialog::marginHint(),
				       KDialog::spacingHint());

    urlpropsbox = new QCheckBox(i18n("&Allow per-URL settings"), this);
    lay->addWidget(urlpropsbox);
    connect(urlpropsbox, SIGNAL(clicked()), this, SLOT(changed()));

    QWhatsThis::add( urlpropsbox, i18n("This allows you to save different settings "
       "depending on the URL. This way, you can configure konqueror to display certain "
       "URLs differently.") );

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

    QString wtstr = i18n("Enter here your favourite terminal program, e.g. konsole or xterm. This "
       "program will be used when you select \"Open Terminal ...\".");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( leTerminal, wtstr );

    lay->addStretch(10);
    lay->activate();

    load();
}

void KMiscOptions::load()
{
    // *** load ***
    g_pConfig->setGroup( groupname );
    bool bUrlprops = g_pConfig->readBoolEntry( "EnablePerURLProps", false);
    QString sTerminal = g_pConfig->readEntry( "Terminal", DEFAULT_TERMINAL );

    // *** apply to GUI ***

    urlpropsbox->setChecked(bUrlprops);
    leTerminal->setText(sTerminal);

}

void KMiscOptions::defaults()
{
    urlpropsbox->setChecked(false);
    leTerminal->setText(DEFAULT_TERMINAL);
}

void KMiscOptions::save()
{
    g_pConfig->setGroup( groupname );
    g_pConfig->writeEntry( "EnablePerURLProps", urlpropsbox->isChecked());
    g_pConfig->writeEntry( "Terminal", leTerminal->text());
    g_pConfig->sync();
}

void KMiscOptions::changed()
{
    emit KCModule::changed(true);
}

#include "miscopts.moc"
