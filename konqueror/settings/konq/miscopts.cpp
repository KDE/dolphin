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

#include "defaults.h"
#include <klocale.h> // include default values directly from kfm

//-----------------------------------------------------------------------------

KMiscOptions::KMiscOptions( QWidget *parent, const char *name )
    : KConfigWidget( parent, name )
{
    QVBoxLayout *lay = new QVBoxLayout(this, 40 /* big border */, 20);
        
    urlpropsbox = new QCheckBox(i18n("&Allow per-URL settings"), this);
    lay->addWidget(urlpropsbox);
 
    treefollowbox = new QCheckBox(i18n("Tree &view follows navigation"), this);
    lay->addWidget(treefollowbox);

    QHBoxLayout *hlay = new QHBoxLayout(10);
    lay->addLayout(hlay);
    QLabel * label = new QLabel(i18n("Terminal"),this);
    hlay->addWidget(label, 1);
        
    leTerminal = new QLineEdit(this);
    hlay->addWidget(leTerminal, 5);

    hlay = new QHBoxLayout(10);
    lay->addLayout(hlay);
    label = new QLabel(i18n("Editor used for viewing HTML source"),this);
    hlay->addWidget(label, 1);
        
    leEditor = new QLineEdit(this);
    hlay->addWidget(leEditor, 5);

    m_pAutoLoadImagesCheckBox = new QCheckBox( i18n( ""
     "Automatically load images\n""
     (Otherwise, click the Images button to load when needed)" ), this );
     
    lay->addWidget( m_pAutoLoadImagesCheckBox, 1 );

    m_pHaveBiiigToolBarCheckBox = new QCheckBox( i18n( "Display big toolbar" ),
                                                 this );
						 
    lay->addWidget( m_pHaveBiiigToolBarCheckBox );

    lay->addStretch(10);
    lay->activate();
    
    loadSettings();
}

void KMiscOptions::loadSettings()
{
    // *** load ***
    g_pConfig->setGroup( "Misc Defaults" );
    bool bUrlprops = g_pConfig->readBoolEntry( "EnablePerURLProps", false);
    bool bTreeFollow = g_pConfig->readBoolEntry( "TreeFollowsView", false);
    QString sTerminal = g_pConfig->readEntry( "Terminal", DEFAULT_TERMINAL );
    QString sEditor = g_pConfig->readEntry( "Editor", DEFAULT_EDITOR );
    bool bAutoLoadImages = g_pConfig->readBoolEntry( "AutoLoadImages", true );
    bool bHaveBigToolBar = g_pConfig->readBoolEntry( "HaveBigToolBar", false );

    // *** apply to GUI ***

    urlpropsbox->setChecked(bUrlprops);
    treefollowbox->setChecked(bTreeFollow);
    leTerminal->setText(sTerminal);
    leEditor->setText(sEditor);
    m_pAutoLoadImagesCheckBox->setChecked( bAutoLoadImages );
    m_pHaveBiiigToolBarCheckBox->setChecked( bHaveBigToolBar );
}

void KMiscOptions::defaultSettings()
{
    urlpropsbox->setChecked(false);
    treefollowbox->setChecked(false);
    leTerminal->setText(DEFAULT_TERMINAL);
    leEditor->setText(DEFAULT_EDITOR);
    m_pAutoLoadImagesCheckBox->setChecked( true );
    m_pHaveBiiigToolBarCheckBox->setChecked( false );
}

void KMiscOptions::saveSettings()
{
    g_pConfig->setGroup( "Misc Defaults" );
    g_pConfig->writeEntry( "EnablePerURLProps", urlpropsbox->isChecked());
    g_pConfig->writeEntry( "TreeFollowsView", treefollowbox->isChecked());
    g_pConfig->writeEntry( "Terminal", leTerminal->text());
    g_pConfig->writeEntry( "Editor", leEditor->text());
    g_pConfig->writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
    g_pConfig->writeEntry( "HaveBigToolBar", m_pHaveBiiigToolBarCheckBox->isChecked() );
    g_pConfig->sync();
}

void KMiscOptions::applySettings()
{
    saveSettings();
}

#include "miscopts.moc"
