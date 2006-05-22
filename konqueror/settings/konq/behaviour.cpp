/**
 *  Copyright (c) 2001 David Faure <david@mandrakesoft.com>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Behaviour options for konqueror

#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <kvbox.h>
#include <khbox.h>

//Added by qt3to4:
#include <QPixmap>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <Q3ButtonGroup>

#include <dcopclient.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kio/uiserver_stub.h>
#include <klocale.h>
#include <konq_defaults.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

#include "behaviour.h"

KBehaviourOptions::KBehaviourOptions(KConfig *config, QString group, KInstance *inst, QWidget *parent )
    : KCModule(inst, parent), g_pConfig(config), groupname(group)
{
    QLabel * label;

    setQuickHelp( i18n("<h1>Konqueror Behavior</h1> You can configure how Konqueror behaves as a file manager here."));

    QVBoxLayout *lay = new QVBoxLayout( this );
    lay->setMargin( 0 );
    lay->setSpacing( KDialog::spacingHint() );

	QGroupBox * miscGb = new QGroupBox(i18n("Misc Options"), this);
	KHBox *hbox = new KHBox(miscGb);
	KVBox *vbox = new KVBox(hbox);

	// ----

	winPixmap = new QLabel(hbox);
    winPixmap->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    winPixmap->setPixmap(QPixmap(locate("data",
                                        "kcontrol/pics/onlyone.png")));
    winPixmap->setFixedSize( winPixmap->sizeHint() );


   // ----

    cbNewWin = new QCheckBox(i18n("Open folders in separate &windows"), vbox);
    cbNewWin->setWhatsThis( i18n("If this option is checked, Konqueror will open a new window when "
                                    "you open a folder, rather than showing that folders's contents in the current window."));
    connect(cbNewWin, SIGNAL(clicked()), this, SLOT(changed()));
    connect(cbNewWin, SIGNAL(toggled(bool)), SLOT(updateWinPixmap(bool)));

    // ----

    cbListProgress = new QCheckBox( i18n( "&Show network operations in a single window" ), vbox );
    connect(cbListProgress, SIGNAL(clicked()), this, SLOT(changed()));

    cbListProgress->setWhatsThis( i18n("Checking this option will group the"
                                          " progress information for all network file transfers into a single window"
                                          " with a list. When the option is not checked, all transfers appear in a"
                                          " separate window.") );


    // --

    cbShowTips = new QCheckBox( i18n( "Show file &tips" ), vbox );
    connect(cbShowTips, SIGNAL(clicked()), this, SLOT(changed()));

    cbShowTips->setWhatsThis( i18n("Here you can control if, when moving the mouse over a file, you want to see a "
                                    "small popup window with additional information about that file"));

    connect(cbShowTips, SIGNAL(toggled(bool)), SLOT(slotShowTips(bool)));
/*
    //connect(cbShowTips, SIGNAL(toggled(bool)), sbToolTip, SLOT(setEnabled(bool)));
    //connect(cbShowTips, SIGNAL(toggled(bool)), fileTips, SLOT(setEnabled(bool)));
    fileTips->setBuddy(sbToolTip);
    QString tipstr = i18n("If you move the mouse over a file, you usually see a small popup window that shows some "
                          "additional information about that file. Here, you can set how many items of information "
                          "are displayed");
    QWhatsThis::add( fileTips, tipstr );
    QWhatsThis::add( sbToolTip, tipstr );
*/

    KHBox *hboxpreview = new KHBox(vbox);
    QWidget* spacer = new QWidget( hboxpreview );
    spacer->setMinimumSize( 20, 0 );
    spacer->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

    cbShowPreviewsInTips = new QCheckBox( i18n( "Show &previews in file tips" ), hboxpreview );
    connect(cbShowPreviewsInTips, SIGNAL(clicked()), this, SLOT(changed()));

    cbShowPreviewsInTips->setWhatsThis( i18n("Here you can control if you want the "
                          "popup window to contain a larger preview for the file, when moving the mouse over it."));

    cbRenameDirectlyIcon = new QCheckBox(i18n("Rename icons in&line"), vbox);
    cbRenameDirectlyIcon->setWhatsThis( i18n("Checking this option will allow files to be "
                                               "renamed by clicking directly on the icon name. "));
    connect(cbRenameDirectlyIcon, SIGNAL(clicked()), this, SLOT(changed()));

	QHBoxLayout *hlay = new QHBoxLayout();
	lay->addItem( hlay );

    label = new QLabel(i18n("Home &URL:"), this);
	hlay->addWidget( label );

	homeURL = new KUrlRequester(this);
	homeURL->setMode(KFile::Directory);
	homeURL->setWindowTitle(i18n("Select Home Folder"));
	hlay->addWidget( homeURL );
    connect(homeURL, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
    label->setBuddy(homeURL);

    QString homestr = i18n("This is the URL (e.g. a folder or a web page) where "
                           "Konqueror will jump to when the \"Home\" button is pressed. "
						   "This is usually your home folder, symbolized by a 'tilde' (~).");
    label->setWhatsThis( homestr );
    homeURL->setWhatsThis( homestr );

    	lay->addWidget( miscGb );

    lay->addItem(new QSpacerItem(0,20,QSizePolicy::Fixed,QSizePolicy::Fixed));

    cbShowDeleteCommand = new QCheckBox( i18n( "Show 'Delete' context me&nu entries which bypass the trashcan" ), this );
    lay->addWidget( cbShowDeleteCommand );
    connect(cbShowDeleteCommand, SIGNAL(clicked()), this, SLOT(changed()));

    cbShowDeleteCommand->setWhatsThis( i18n("Check this if you want 'Delete' menu commands to be displayed "
                                                "on the desktop and in the file manager's context menus. "
						"You can always delete files by holding the Shift key "
						"while calling 'Move to Trash'."));

    Q3ButtonGroup *bg = new Q3VButtonGroup( i18n("Ask Confirmation For"), this );
    bg->layout()->setSpacing( KDialog::spacingHint() );
    bg->setWhatsThis( i18n("This option tells Konqueror whether to ask"
       " for a confirmation when you \"delete\" a file."
       " <ul><li><em>Move To Trash:</em> moves the file to your trash folder,"
       " from where it can be recovered very easily.</li>"
       " <li><em>Delete:</em> simply deletes the file.</li>"
       " </li></ul>") );

    connect(bg, SIGNAL( clicked( int ) ), SLOT( changed() ));

    cbMoveToTrash = new QCheckBox( i18n("&Move to trash"), bg );

    cbDelete = new QCheckBox( i18n("D&elete"), bg );

    lay->addWidget(bg);

    lay->addStretch();

    load();
}

KBehaviourOptions::~KBehaviourOptions()
{
}

void KBehaviourOptions::slotShowTips(bool b)
{
//    sbToolTip->setEnabled( b );
    cbShowPreviewsInTips->setEnabled( b );
//    fileTips->setEnabled( b );

}

void KBehaviourOptions::load()
{
    g_pConfig->setGroup( groupname );
    cbNewWin->setChecked( g_pConfig->readEntry("AlwaysNewWin", QVariant(false)).toBool() );
    updateWinPixmap(cbNewWin->isChecked());

    homeURL->setUrl(g_pConfig->readPathEntry("HomeURL", "~"));

    bool stips = g_pConfig->readEntry( "ShowFileTips", QVariant(true )).toBool();
    cbShowTips->setChecked( stips );
    slotShowTips( stips );

    bool showPreviewsIntips = g_pConfig->readEntry( "ShowPreviewsInFileTips", QVariant(true )).toBool();
    cbShowPreviewsInTips->setChecked( showPreviewsIntips );

    cbRenameDirectlyIcon->setChecked( g_pConfig->readEntry("RenameIconDirectly", QVariant(DEFAULT_RENAMEICONDIRECTLY )).toBool() );

    KConfig globalconfig("kdeglobals", true, false);
    globalconfig.setGroup( "KDE" );
    cbShowDeleteCommand->setChecked( globalconfig.readEntry("ShowDeleteCommand", QVariant(false)).toBool() );

//    if (!stips) sbToolTip->setEnabled( false );
    if (!stips) cbShowPreviewsInTips->setEnabled( false );

//    sbToolTip->setValue( g_pConfig->readEntry( "FileTipItems", 6 ) );

    KConfig config("uiserverrc");
    config.setGroup( "UIServer" );

    cbListProgress->setChecked( config.readEntry( "ShowList", QVariant(false )).toBool() );

    g_pConfig->setGroup( "Trash" );
    cbMoveToTrash->setChecked( g_pConfig->readEntry("ConfirmTrash", QVariant(DEFAULT_CONFIRMTRASH)).toBool() );
    cbDelete->setChecked( g_pConfig->readEntry("ConfirmDelete", QVariant(DEFAULT_CONFIRMDELETE)).toBool() );
}

void KBehaviourOptions::defaults()
{
    cbNewWin->setChecked(false);

    homeURL->setUrl(KUrl("~"));

    cbListProgress->setChecked( false );

    cbShowTips->setChecked( true );
    //sbToolTip->setEnabled( true );
    //sbToolTip->setValue( 6 );

    cbShowPreviewsInTips->setChecked( true );
    cbShowPreviewsInTips->setEnabled( true );

    cbRenameDirectlyIcon->setChecked( DEFAULT_RENAMEICONDIRECTLY );

    cbMoveToTrash->setChecked( DEFAULT_CONFIRMTRASH );
    cbDelete->setChecked( DEFAULT_CONFIRMDELETE );
    cbShowDeleteCommand->setChecked( false );
}

void KBehaviourOptions::save()
{
    g_pConfig->setGroup( groupname );

    g_pConfig->writeEntry( "AlwaysNewWin", cbNewWin->isChecked() );
    g_pConfig->writePathEntry( "HomeURL", homeURL->url().isEmpty()? QString("~") : homeURL->url().url() );

    g_pConfig->writeEntry( "ShowFileTips", cbShowTips->isChecked() );
    g_pConfig->writeEntry( "ShowPreviewsInFileTips", cbShowPreviewsInTips->isChecked() );
//    g_pConfig->writeEntry( "FileTipsItems", sbToolTip->value() );

    g_pConfig->writeEntry( "RenameIconDirectly", cbRenameDirectlyIcon->isChecked());

    KConfig globalconfig("kdeglobals", false, false);
    globalconfig.setGroup( "KDE" );
    globalconfig.writeEntry( "ShowDeleteCommand", cbShowDeleteCommand->isChecked());
    globalconfig.sync();

    g_pConfig->setGroup( "Trash" );
    g_pConfig->writeEntry( "ConfirmTrash", cbMoveToTrash->isChecked());
    g_pConfig->writeEntry( "ConfirmDelete", cbDelete->isChecked());
    g_pConfig->sync();

    // UIServer setting
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

    // Send signal to konqueror
    QByteArray data;
    if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
    kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );
    kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "configure()", data );
}

void KBehaviourOptions::updateWinPixmap(bool b)
{
  if (b)
    winPixmap->setPixmap(QPixmap(locate("data",
                                        "kcontrol/pics/overlapping.png")));
  else
    winPixmap->setPixmap(QPixmap(locate("data",
                                        "kcontrol/pics/onlyone.png")));
}

#include "behaviour.moc"
