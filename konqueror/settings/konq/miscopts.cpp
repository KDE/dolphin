//
//
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>//CT - 12Nov1998
#include <kapp.h>

#include "miscopts.h"

#include <konqdefaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <kconfig.h>


//-----------------------------------------------------------------------------

KMiscOptions::KMiscOptions( QWidget *parent, const char *name )
    : KCModule( parent, name )
{
    QVBoxLayout *lay = new QVBoxLayout(this, 40 /* big border */, 20);

    urlpropsbox = new QCheckBox(i18n("&Allow per-URL settings"), this);
    lay->addWidget(urlpropsbox);
    connect(urlpropsbox, SIGNAL(clicked()), this, SLOT(changed()));

    treefollowbox = new QCheckBox(i18n("Tree &view follows navigation"), this);
    lay->addWidget(treefollowbox);
    connect(treefollowbox, SIGNAL(clicked()), this, SLOT(changed()));

    QHBoxLayout *hlay = new QHBoxLayout(10);
    lay->addLayout(hlay);
    QLabel * label = new QLabel(i18n("Terminal"),this);
    hlay->addWidget(label, 1);

    leTerminal = new QLineEdit(this);
    hlay->addWidget(leTerminal, 5);
    connect(leTerminal, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

    hlay = new QHBoxLayout(10);
    lay->addLayout(hlay);
    label = new QLabel(i18n("Editor used for viewing HTML source"),this);
    hlay->addWidget(label, 1);

    leEditor = new QLineEdit(this);
    hlay->addWidget(leEditor, 5);
    connect(leEditor, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

    m_pHaveBiiigToolBarCheckBox = new QCheckBox( i18n( "Display big toolbar" ),
                                                 this );
    connect(m_pHaveBiiigToolBarCheckBox, SIGNAL(clicked()), this, SLOT(changed()));
						
    lay->addWidget( m_pHaveBiiigToolBarCheckBox );

    lay->addStretch(10);
    lay->activate();

    load();
}

void KMiscOptions::load()
{
  KConfig *config = new KConfig("konquerrorrc");

    // *** load ***
    config->setGroup( "Misc Defaults" );
    bool bUrlprops = config->readBoolEntry( "EnablePerURLProps", false);
    bool bTreeFollow = config->readBoolEntry( "TreeFollowsView", false);
    QString sTerminal = config->readEntry( "Terminal", DEFAULT_TERMINAL );
    QString sEditor = config->readEntry( "Editor", DEFAULT_EDITOR );
    bool bHaveBigToolBar = config->readBoolEntry( "HaveBigToolBar", false );

    // *** apply to GUI ***

    urlpropsbox->setChecked(bUrlprops);
    treefollowbox->setChecked(bTreeFollow);
    leTerminal->setText(sTerminal);
    leEditor->setText(sEditor);
    m_pHaveBiiigToolBarCheckBox->setChecked( bHaveBigToolBar );

    delete config;
}

void KMiscOptions::defaults()
{
    urlpropsbox->setChecked(false);
    treefollowbox->setChecked(false);
    leTerminal->setText(DEFAULT_TERMINAL);
    leEditor->setText(DEFAULT_EDITOR);
    m_pHaveBiiigToolBarCheckBox->setChecked( false );
}

void KMiscOptions::save()
{
  KConfig *config = new KConfig("konquerrorrc");

    config->setGroup( "Misc Defaults" );
    config->writeEntry( "EnablePerURLProps", urlpropsbox->isChecked());
    config->writeEntry( "TreeFollowsView", treefollowbox->isChecked());
    config->writeEntry( "Terminal", leTerminal->text());
    config->writeEntry( "Editor", leEditor->text());
    config->writeEntry( "HaveBigToolBar", m_pHaveBiiigToolBarCheckBox->isChecked() );
    config->sync();

    delete config;
}

void KMiscOptions::changed()
{
    emit KCModule::changed(true);
}

#include "miscopts.moc"
