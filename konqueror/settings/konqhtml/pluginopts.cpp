// (c) 2001, Daniel Naber, based on javaopts.cpp

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <X11/Xlib.h>

#include "htmlopts.h"
#include "pluginopts.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <khtml_settings.h>
#include <khtmldefaults.h>

#include "pluginopts.moc"

KPluginOptions::KPluginOptions( KConfig* config, QString group, QWidget *parent,
                            const char *name )
    : KCModule( parent, name ),
      m_pConfig( config ),
      m_groupname( group )
{
    QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enablePluginsGloballyCB = new QCheckBox( i18n( "Enable &Plugins globally" ), globalGB );
    connect( enablePluginsGloballyCB, SIGNAL( clicked() ), this, SLOT( changed() ) );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    toplevel->addItem( spacer );

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    QWhatsThis::add( enablePluginsGloballyCB, i18n("Enables the execution of plugins "
          "that can be contained in HTML pages, e.g. Macromedia Flash. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );

    // Finally do the loading
    load();
}


void KPluginOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);
    bool bPluginGlobal = m_pConfig->readBoolEntry( "EnablePlugins", true );

    // *** apply to GUI ***
    enablePluginsGloballyCB->setChecked( bPluginGlobal );
}

void KPluginOptions::defaults()
{
    enablePluginsGloballyCB->setChecked( true );
}

void KPluginOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "EnablePlugins", enablePluginsGloballyCB->isChecked() );
}

void KPluginOptions::changed()
{
    emit KCModule::changed(true);
}
