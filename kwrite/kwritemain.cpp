/***************************************************************************
                          kwritemain.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qdropsite.h>
#include <qdragobject.h>
#include <qvbox.h>

#include <dcopclient.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kaboutdata.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kapp.h>
#include <klocale.h>
#include <kurl.h>
#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kparts/event.h>
#include <kmenubar.h>

#include "kwritemain.h"
#include "kwritemain.moc"

#include "../part/katefiledialog.h"

// StatusBar field IDs
#define ID_LINE_COLUMN 1
#define ID_INS_OVR 2
#define ID_SEL_NORM_BLOCK 3
#define ID_MODIFIED 4
#define ID_GENERAL 5

QPtrList<KateDocument> docList; //documents

TopLevel::TopLevel (KateDocument *doc)
{
  setMinimumSize(200,200);

  statusbarTimer = new QTimer(this);
  connect(statusbarTimer,SIGNAL(timeout()),this,SLOT(timeout()));

  if (!doc) {
    doc = new KateDocument (); //new doc with default path
    docList.append(doc);
  }
  setupEditWidget(doc);
  setupActions();
  setupStatusBar();

  setAcceptDrops(true);

  setXMLFile( "kwriteui.rc" );
  createShellGUI( true );
  guiFactory()->addClient( kateView );
  KParts::GUIActivateEvent ev( true );
  QApplication::sendEvent( kateView, &ev );

  // Read basic main-view settings, and set to autosave
  setAutoSaveSettings( "General Options" );

}


TopLevel::~TopLevel()
{
  if (kateView->isLastView()) docList.remove((KateDocument*)kateView->doc());
}


void TopLevel::init()
{
  KToolBar *tb = toolBar("mainToolBar");
  if (tb) m_paShowToolBar->setChecked( !tb->isHidden() );
    else m_paShowToolBar->setEnabled(false);
  KStatusBar *sb = statusBar();
  if (sb) m_paShowStatusBar->setChecked( !sb->isHidden() );
    else m_paShowStatusBar->setEnabled(false);

  newCurPos();
  newStatus();

  show();
}


void TopLevel::loadURL(const KURL &url)
{
  m_recentFiles->addURL( url );
  kateView->doc()->openURL(url);
}


bool TopLevel::queryClose()
{
  if (!kateView->isLastView()) return true;
  return kateView->canDiscard();
}


bool TopLevel::queryExit()
{
  writeConfig();
  kapp->config()->sync();
  KateFactory::instance()->config()->sync();

  return true;
}


void TopLevel::setupEditWidget(KateDocument *doc)
{
  kateView = new KateView(doc, this, 0);

  connect(kateView,SIGNAL(cursorPositionChanged()),this,SLOT(newCurPos()));
  connect(kateView,SIGNAL(newStatus()),this,SLOT(newStatus()));
  connect(kateView->doc(),SIGNAL(fileNameChanged()),this,SLOT(newCaption()));
  connect(kateView,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(slotDropEvent(QDropEvent *)));

  setCentralWidget(kateView);
}


void TopLevel::setupActions()
{
  // setup File menu
  KStdAction::print(this, SLOT(printDlg()), actionCollection());
  KStdAction::openNew( this, SLOT(slotNew()), actionCollection(), "file_new" );
  KStdAction::open( this, SLOT( slotOpen() ), actionCollection(), "file_open" );
  m_recentFiles = KStdAction::openRecent(this, SLOT(slotOpen(const KURL&)),
                                        actionCollection());

  new KAction(i18n("New &View"), 0, this, SLOT(newView()),
              actionCollection(), "file_newView");
  KStdAction::quit(this, SLOT(close()), actionCollection());


  // setup Settings menu
  m_paShowToolBar = KStdAction::showToolbar( this, SLOT( toggleToolBar() ), actionCollection() );
  m_paShowStatusBar = KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection());
  m_paShowPath = new KToggleAction(i18n("Sho&w Path"), 0, this, SLOT(newCaption()),
                    actionCollection(), "set_showPath");
  KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
}

void TopLevel::setupStatusBar()
{
  KStatusBar *statusbar;
  statusbar = statusBar();
  statusbar->insertItem(" Line:000000 Col: 000 ", ID_LINE_COLUMN);
  statusbar->insertItem(" XXX ", ID_INS_OVR);
  statusbar->insertItem(" XXX ", ID_SEL_NORM_BLOCK);
  statusbar->insertFixedItem(" * ", ID_MODIFIED);
  statusbar->insertItem("", ID_GENERAL, 1);
  statusbar->setItemAlignment( ID_GENERAL, AlignLeft );
}

void TopLevel::slotNew()
{
  if (kateView->doc()->isModified() || !kateView->doc()->url().isEmpty())
  {
   TopLevel*t = new TopLevel();
    t->readConfig();
    t->init();
  }
  else
    kateView->flush();
}

void TopLevel::slotOpen()
{
  KateFileDialog *dialog = new KateFileDialog (QString::null,kateView->doc()->encoding(), this, i18n ("Open File"));
	KateFileDialogData data = dialog->exec ();
	delete dialog;

	for (KURL::List::Iterator i=data.urls.begin(); i != data.urls.end(); ++i)
  {
	  encoding = data.encoding;
    slotOpen ( *i );
  }
}

void TopLevel::slotOpen( const KURL& url )
{
  if (url.isEmpty()) return;

  if (kateView->doc()->isModified() || !kateView->doc()->url().isEmpty())
  {
    TopLevel *t = new TopLevel();
		t->kateView->doc()->setEncoding(encoding);
    t->readConfig();
    t->init();
    t->loadURL(url);
  }
  else
	{
	  kateView->doc()->setEncoding(encoding);
    loadURL(url);
  }
}

void TopLevel::newView()
{
  TopLevel *t = new TopLevel((KateDocument *)kateView->doc());
  t->readConfig();
  t->init();
}

void TopLevel::toggleToolBar()
{
  if( m_paShowToolBar->isChecked() )
    toolBar("mainToolBar")->show();
  else
    toolBar("mainToolBar")->hide();
}

void TopLevel::toggleStatusBar()
{
  if( m_paShowStatusBar->isChecked() )
    statusBar()->show();
  else
    statusBar()->hide();
}

void TopLevel::editKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void TopLevel::editToolbars()
{
  KEditToolbar *dlg = new KEditToolbar(guiFactory());

  if (dlg->exec())
  {
      KParts::GUIActivateEvent ev1( false );
      QApplication::sendEvent( kateView, &ev1 );
      guiFactory()->removeClient( kateView );
      createShellGUI( false );
      createShellGUI( true );
      guiFactory()->addClient( kateView );
      KParts::GUIActivateEvent ev2( true );
      QApplication::sendEvent( kateView, &ev2 );
  }
  delete dlg;
}

void TopLevel::printNow()
{
  kateView->doc()->print ();
}

void TopLevel::printDlg()
{
  kateView->doc()->printDialog ();
}

void TopLevel::newCurPos()
{
  statusBar()->changeItem(i18n(" Line: %1 Col: %2 ")
    .arg(KGlobal::locale()->formatNumber(kateView->cursorLine()+1, 0))
    .arg(KGlobal::locale()->formatNumber(kateView->cursorColumn(), 0)),
    ID_LINE_COLUMN);
}

void TopLevel::newStatus()
{
  newCaption();

  bool readOnly = !kateView->doc()->isReadWrite();
  uint config = kateView->doc()->configFlags();
  bool block=kateView->doc()->blockSelectionMode();

  if (readOnly)
    statusBar()->changeItem(i18n(" R/O "),ID_INS_OVR);
  else
    statusBar()->changeItem(config & KateDocument::cfOvr ? i18n(" OVR ") : i18n(" INS "),ID_INS_OVR);

  statusBar()->changeItem(kateView->doc()->isModified() ? " * " : "",ID_MODIFIED);
  statusBar()->changeItem(block ? i18n("BLK") : i18n(" NORM "),ID_SEL_NORM_BLOCK);
}

void TopLevel::timeout() {
  statusBar()->changeItem("", ID_GENERAL);
}

void TopLevel::newCaption()
{
  if (kateView->doc()->url().isEmpty()) {
    setCaption(i18n("Untitled"),kateView->doc()->isModified());
  } else {
    //set caption
    if ( m_paShowPath->isChecked() )
    {
       //File name shouldn't be too long - Maciek
       if (kateView->doc()->url().filename().length() > 200)
         setCaption(kateView->doc()->url().prettyURL().left(197) + "...",kateView->doc()->isModified());
       else
         setCaption(kateView->doc()->url().prettyURL(),kateView->doc()->isModified());
     }
      else
     {
       //File name shouldn't be too long - Maciek
       if (kateView->doc()->url().filename().length() > 200)
         setCaption("..." + kateView->doc()->url().fileName().right(197),kateView->doc()->isModified());
       else
         setCaption(kateView->doc()->url().fileName(),kateView->doc()->isModified());

    }

  }
}

void TopLevel::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept(QUriDrag::canDecode(event));
}


void TopLevel::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}


void TopLevel::slotDropEvent( QDropEvent *event )
{
  QStrList  urls;

  if (QUriDrag::decode(event, urls)) {
    kdDebug(13000) << "TopLevel:Handling QUriDrag..." << endl;
		QPtrListIterator<char> it(urls);
		for( ; it.current(); ++it ) {
      slotOpen( (*it) );
    }
  }
}

void TopLevel::slotEnableActions( bool enable )
{
    QValueList<KAction *> actions = actionCollection()->actions();
    QValueList<KAction *>::ConstIterator it = actions.begin();
    QValueList<KAction *>::ConstIterator end = actions.end();
    for (; it != end; ++it )
        (*it)->setEnabled( enable );

    actions = kateView->actionCollection()->actions();
    it = actions.begin();
    end = actions.end();
    for (; it != end; ++it )
        (*it)->setEnabled( enable );
}

//common config
void TopLevel::readConfig(KConfig *config)
{
  m_paShowPath->setChecked( config->readBoolEntry("ShowPath") );
  m_recentFiles->loadEntries(config, "Recent Files");
}


void TopLevel::writeConfig(KConfig *config)
{
  config->writeEntry("ShowPath",m_paShowPath->isChecked());
  m_recentFiles->saveEntries(config, "Recent Files");
}


//config file
void TopLevel::readConfig() {
  KConfig *config;

  config = kapp->config();

  config->setGroup("General Options");
  readConfig(config);

  kateView->doc()->readConfig();
}


void TopLevel::writeConfig()
{
  KConfig *config;

  config = kapp->config();

  config->setGroup("General Options");
  writeConfig(config);

  kateView->doc()->writeConfig();
}

// session management
void TopLevel::restore(KConfig *config, int n)
{
  if (kateView->isLastView() && !kateView->doc()->url().isEmpty()) { //in this case first view
    loadURL(kateView->doc()->url());
  }
  readPropertiesInternal(config, n);
  init();
}

void TopLevel::readProperties(KConfig *config)
{
  readConfig(config);
  kateView->readSessionConfig(config);
}

void TopLevel::saveProperties(KConfig *config)
{
  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.find((KateDocument *)kateView->doc()) + 1);
  kateView->writeSessionConfig(config);
}

void TopLevel::saveGlobalProperties(KConfig *config) //save documents
{
  uint z;
  QString buf;
  KateDocument *doc;

  config->setGroup("Number");
  config->writeEntry("NumberOfDocuments",docList.count());

  for (z = 1; z <= docList.count(); z++) {
     buf = QString("Document%1").arg(z);
     config->setGroup(buf);
     doc = (KateDocument *) docList.at(z - 1);
     doc->writeSessionConfig(config);
  }
}

//restore session
void restore()
{
  KConfig *config;
  int docs, windows, z;
  QString buf;
  KateDocument *doc;
  TopLevel *t;

  config = kapp->sessionConfig();
  if (!config) return;

  config->setGroup("Number");
  docs = config->readNumEntry("NumberOfDocuments");
  windows = config->readNumEntry("NumberOfWindows");

  for (z = 1; z <= docs; z++) {
     buf = QString("Document%1").arg(z);
     config->setGroup(buf);
     doc = new KateDocument ();
     doc->readSessionConfig(config);
     docList.append(doc);
  }

  for (z = 1; z <= windows; z++) {
    buf = QString("%1").arg(z);
    config->setGroup(buf);
    t = new TopLevel(docList.at(config->readNumEntry("DocumentNumber") - 1));
    t->restore(config,z);
  }
}

static KCmdLineOptions options[] =
{
  { "+[URL]",   I18N_NOOP("Document to open."), 0 },
  { 0, 0, 0}
};

int main(int argc, char **argv)
{
  KLocale::setMainCatalogue("kate");         //lukas: set this to have the kwritepart translated using kate message catalog

  KAboutData aboutData ("kwrite", I18N_NOOP("KWrite"), "4.0",
	I18N_NOOP( "KWrite - Leightweight Kate" ), KAboutData::License_GPL,
	I18N_NOOP( "(c) 2000-2001 The Kate Authors" ), 0, "http://kate.sourceforge.net");

  aboutData.addAuthor("Christoph Cullmann", I18N_NOOP("Project Manager and Core Developer"), "cullmann@kde.org", "http://www.babylon2k.de");
  aboutData.addAuthor("Michael Bartl", I18N_NOOP("Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor("Phlip", I18N_NOOP("The Project Compiler"), "phlip_cpp@my-deja.com");
  aboutData.addAuthor("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  aboutData.addAuthor("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor( "Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor( "Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor( "Christian Gebauer", 0, "gebauer@kde.org" );
  aboutData.addAuthor( "Simon Hausmann", 0, "hausmann@kde.org" );
  aboutData.addAuthor("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  aboutData.addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");
  
  aboutData.addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  aboutData.addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  aboutData.addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  aboutData.addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  aboutData.addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  aboutData.addCredit ("John Firebaugh",I18N_NOOP("Highlighting for Java, and much more"),"");
  aboutData.addCredit ("Carsten Niehaus", I18N_NOOP("Highlighing for LaTeX"),"");
  aboutData.addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  aboutData.addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  aboutData.addCredit ("Daniel Naber","","");
  aboutData.addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  aboutData.addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  aboutData.addCredit ("Carsten Presser", I18N_NOOP("Betatest"), "mord-slime@gmx.de");
  aboutData.addCredit ("Jens Haupert", I18N_NOOP("Betatest"), "al_all@gmx.de");
  aboutData.addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  aboutData.addCredit ("Mr. Doerr", I18N_NOOP("For absolutely nothing"), "");
  aboutData.addCredit (I18N_NOOP("All people, who have contributed and I have forgotten to mention"),"","");

  aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication *a = new KApplication();

  DCOPClient *client = kapp->dcopClient();
  if (!client->isRegistered())
  {
    client->attach();
    client->registerAs("kwrite");
  }

  //list that contains all documents
  docList.setAutoDelete(false);

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (kapp->isRestored()) {
    restore();
  } else {
    TopLevel *t;

    if ( args->count() == 0 )
    {
        t = new TopLevel;
        t->readConfig();
        t->init();
    }
    else
    {
        for ( int i = 0; i < args->count(); ++i )
        {
            t = new TopLevel();
            t->readConfig();
            t->loadURL( args->url( i ) );
            t->init();
        }
    }
  }

  int r = a->exec();

  args->clear();
  return r;
}
