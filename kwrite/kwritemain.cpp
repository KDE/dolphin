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

#include <ktexteditor/configinterface.h>
#include <ktexteditor/viewconfiginterface.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/printinterface.h>
#include <ktexteditor/encodinginterface.h>

#include <dcopclient.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kaboutdata.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kapplication.h>
#include <klocale.h>
#include <kurl.h>
#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kparts/event.h>
#include <kmenubar.h>

#include "kwritemain.h"
#include "kwritemain.moc"

#include "katefiledialog.h"
#include <klibloader.h>

#include <qtimer.h>

// StatusBar field IDs
#define ID_GEN 1

enum saveResult { SAVE_OK, SAVE_CANCEL, SAVE_RETRY, SAVE_ERROR };

int checkOverwrite( KURL u, KTextEditor::View *view )
{
  int query = KMessageBox::Yes;

  if( u.isLocalFile() )
  {
    QFileInfo info;
    QString name( u.path() );
    info.setFile( name );
    if( info.exists() )
      query = KMessageBox::warningYesNoCancel( view,
        i18n( "A Document with this Name already exists.\nDo you want to overwrite it?" ) );
  }
  return query;
}

saveResult saveAs(KTextEditor::View *view) {
  int query;
  KateFileDialog *dialog;
  KateFileDialogData data;

  do {
    query = KMessageBox::Yes;

    dialog = new KateFileDialog (view->document()->url().url(),static_cast<KTextEditor::EncodingInterface*>(view->document()->qt_cast("KTextEditor::EncodingInterface"))->encoding(), view, i18n ("Save File"), KateFileDialog::saveDialog);
    data = dialog->exec ();
    delete dialog;
    if (data.url.isEmpty())
      return SAVE_CANCEL;
    query = checkOverwrite( data.url, view );
  }
  while ((query != KMessageBox::Cancel) && (query != KMessageBox::Yes));

  if( query == KMessageBox::Cancel )
    return SAVE_CANCEL;

  static_cast<KTextEditor::EncodingInterface*>(view->document()->qt_cast("KTextEditor::EncodingInterface"))->setEncoding (data.encoding);
  if( !view->document()->saveAs(data.url) ) {
    KMessageBox::sorry(view,
      i18n("The file could not be saved. Please check if you have write permission."));
    return SAVE_ERROR;
  }

  return SAVE_OK;
}

saveResult save(KTextEditor::View *view) {
  int query = KMessageBox::Yes;
  if (view->document()->isModified()) {
    if (!view->document()->url().fileName().isEmpty() && view->document()->isReadWrite()) {
      // If document is new but has a name, check if saving it would
      // overwrite a file that has been created since the new doc
      // was created:
      if( query == KMessageBox::Yes )
      {
         if( !view->document()->saveAs(view->document()->url()) ) {
       KMessageBox::sorry(view,
        i18n("The file could not be saved. Please check if you have write permission."));
      return SAVE_ERROR;
  }
      }
      else  // Do not overwrite already existing document:
        return saveAs(view);
    } // New, unnamed document:
    else
      return saveAs(view);
  }
  return SAVE_OK;
}

bool canDiscard(KTextEditor::View *view) {
  int query;

  if (view->document()->isModified()) {
    query = KMessageBox::warningYesNoCancel(view,
      i18n("The current Document has been modified.\nWould you like to save it?"));
    switch (query) {
      case KMessageBox::Yes: //yes
        if (save(view) == SAVE_CANCEL) return false;
        if (view->document()->isModified()) {
            query = KMessageBox::warningContinueCancel(view,
               i18n("Could not save the document.\nDiscard it and continue?"),
           QString::null, i18n("&Discard"));
          if (query == KMessageBox::Cancel) return false;
        }
        break;
      case KMessageBox::Cancel: //cancel
        return false;
    }
  }
  return true;
}

QPtrList<KTextEditor::Document> docList; //documents

KWrite::KWrite (KTextEditor::Document *doc)
{
  setMinimumSize(200,200);
  
  if (!initialGeometrySet())
     resize(640,400); 

  factory = KLibLoader::self()->factory( "katepart" );

  if (!doc) {
    KTextEditor::Document *tmpDoc = (KTextEditor::Document *) factory->create (0L, "kate", "KTextEditor::Document");
    doc = (KTextEditor::Document *)tmpDoc; //new doc with default path
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


KWrite::~KWrite()
{
  if (kateView->document()->views().count() == 1) docList.remove((KTextEditor::Document*)kateView->document());
}


void KWrite::init()
{
  KToolBar *tb = toolBar("mainToolBar");
  if (tb) m_paShowToolBar->setChecked( !tb->isHidden() );
    else m_paShowToolBar->setEnabled(false);
  KStatusBar *sb = statusBar();
  if (sb) m_paShowStatusBar->setChecked( !sb->isHidden() );
    else m_paShowStatusBar->setEnabled(false);

  show();
}


void KWrite::loadURL(const KURL &url)
{
  m_recentFiles->addURL( url );
  kateView->document()->openURL(url);
}


bool KWrite::queryClose()
{
  if (!(kateView->document()->views().count() == 1)) return true;
  return canDiscard(kateView);
}


bool KWrite::queryExit()
{
  writeConfig();
  kapp->config()->sync();

  return true;
}


void KWrite::setupEditWidget(KTextEditor::Document *doc)
{
  kateView = (KTextEditor::View *)doc->createView (this, 0L);

  connect(kateView,SIGNAL(newStatus()),this,SLOT(newCaption()));
  connect(kateView,SIGNAL(viewStatusMsg(const QString &)),this,SLOT(newStatus(const QString &)));
  connect(kateView->document(),SIGNAL(fileNameChanged()),this,SLOT(newCaption()));
  connect(kateView,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(slotDropEvent(QDropEvent *)));

  setCentralWidget(kateView);
  
  KStdAction::close( this, SLOT(slotFlush()), actionCollection(), "file_close" );
}

void KWrite::slotFlush ()
{
   if (canDiscard (kateView))
     kateView->document()->closeURL();
}

void KWrite::setupActions()
{
  // setup File menu
  KStdAction::print(this, SLOT(printDlg()), actionCollection());
  KStdAction::openNew( this, SLOT(slotNew()), actionCollection(), "file_new" );
  KStdAction::open( this, SLOT( slotOpen() ), actionCollection(), "file_open" );
//  m_recentFiles = KStdAction::openRecent(this, SLOT(slotOpen(const KURL&)),
//                                        actionCollection());

  m_recentFiles = KStdAction::openRecent(this, SLOT(slotOpen(const KURL&)),
                                        actionCollection());


  new KAction(i18n("New &View"), 0, this, SLOT(newView()),
              actionCollection(), "file_newView");
  KStdAction::quit(this, SLOT(close()), actionCollection());


  // setup Settings menu
  m_paShowToolBar = KStdAction::showToolbar( this, SLOT( toggleToolBar() ), actionCollection(), "settings_show_toolbar" );
  m_paShowStatusBar = KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection(), "settings_show_statusbar");
  m_paShowPath = new KToggleAction(i18n("Sho&w Path"), 0, this, SLOT(newCaption()),
                    actionCollection(), "set_showPath");
  KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection(), "set_configure_toolbars");
}

void KWrite::setupStatusBar()
{
  KStatusBar *statusbar;
  statusbar = statusBar();
  statusbar->insertItem("", ID_GEN);
}

void KWrite::slotNew()
{
  if (kateView->document()->isModified() || !kateView->document()->url().isEmpty())
  {
   KWrite*t = new KWrite();
    t->readConfig();
    t->init();
  }
  else
    kateView->document()->openURL("");
}

void KWrite::slotOpen()
{
  KateFileDialog *dialog = new KateFileDialog (QString::null,static_cast<KTextEditor::EncodingInterface*>(kateView->document()->qt_cast("KTextEditor::EncodingInterface"))->encoding(), this, i18n ("Open File"));
	KateFileDialogData data = dialog->exec ();
	delete dialog;

	for (KURL::List::Iterator i=data.urls.begin(); i != data.urls.end(); ++i)
  {
	  encoding = data.encoding;
    slotOpen ( *i );
  }
}

void KWrite::slotOpen( const KURL& url )
{
  if (url.isEmpty()) return;

  if (kateView->document()->isModified() || !kateView->document()->url().isEmpty())
  {
    KWrite *t = new KWrite();
		static_cast<KTextEditor::EncodingInterface*>(kateView->document()->qt_cast("KTextEditor::EncodingInterface"))->setEncoding(encoding);
    t->readConfig();
    t->init();
    t->loadURL(url);
  }
  else
	{
	  static_cast<KTextEditor::EncodingInterface*>(kateView->document()->qt_cast("KTextEditor::EncodingInterface"))->setEncoding(encoding);
    loadURL(url);
  }
}

void KWrite::newView()
{
  KWrite *t = new KWrite((KTextEditor::Document *)kateView->document());
  t->readConfig();
  t->init();
}

void KWrite::toggleToolBar()
{
  if( m_paShowToolBar->isChecked() )
    toolBar("mainToolBar")->show();
  else
    toolBar("mainToolBar")->hide();
}

void KWrite::toggleStatusBar()
{
  if( m_paShowStatusBar->isChecked() )
    statusBar()->show();
  else
    statusBar()->hide();
}

void KWrite::editKeys()
{
  KKeyDialog dlg;
  dlg.insert(actionCollection());
  if( kateView )
    dlg.insert(kateView->actionCollection());
  dlg.configure();
}

void KWrite::editToolbars()
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

void KWrite::printNow()
{
  static_cast<KTextEditor::PrintInterface*>(kateView->document()->qt_cast("KTextEditor::PrintInterface"))->print ();
}

void KWrite::printDlg()
{
  static_cast<KTextEditor::PrintInterface*>(kateView->document()->qt_cast("KTextEditor::PrintInterface"))->printDialog ();
}

void KWrite::newStatus(const QString &msg)
{
  newCaption();

  statusBar()->changeItem(msg,ID_GEN);
}

void KWrite::newCaption()
{
  if (kateView->document()->url().isEmpty()) {
    setCaption(i18n("Untitled"),kateView->document()->isModified());
  } else {
    //set caption
    if ( m_paShowPath->isChecked() )
    {
       //File name shouldn't be too long - Maciek
       if (kateView->document()->url().filename().length() > 200)
         setCaption(kateView->document()->url().prettyURL().left(197) + "...",kateView->document()->isModified());
       else
         setCaption(kateView->document()->url().prettyURL(),kateView->document()->isModified());
     }
      else
     {
       //File name shouldn't be too long - Maciek
       if (kateView->document()->url().filename().length() > 200)
         setCaption("..." + kateView->document()->url().fileName().right(197),kateView->document()->isModified());
       else
         setCaption(kateView->document()->url().fileName(),kateView->document()->isModified());

    }

  }
}

void KWrite::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept(QUriDrag::canDecode(event));
}


void KWrite::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}


void KWrite::slotDropEvent( QDropEvent *event )
{
  QStrList  urls;

  if (QUriDrag::decode(event, urls)) {
    kdDebug(13000) << "KWrite:Handling QUriDrag..." << endl;
		QPtrListIterator<char> it(urls);
		for( ; it.current(); ++it ) {
      slotOpen( (*it) );
    }
  }
}

void KWrite::slotEnableActions( bool enable )
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
void KWrite::readConfig(KConfig *config)
{
  m_paShowPath->setChecked( config->readBoolEntry("ShowPath") );
  m_recentFiles->loadEntries(config, "Recent Files");
}


void KWrite::writeConfig(KConfig *config)
{
  config->writeEntry("ShowPath",m_paShowPath->isChecked());
  m_recentFiles->saveEntries(config, "Recent Files");
}


//config file
void KWrite::readConfig() {
  KConfig *config;

  config = kapp->config();

  config->setGroup("General Options");
  readConfig(config);

  static_cast<KTextEditor::ConfigInterface*>(kateView->document()->qt_cast("KTextEditor::ConfigInterface"))->readConfig();
}


void KWrite::writeConfig()
{
  KConfig *config;

  config = kapp->config();

  config->setGroup("General Options");
  writeConfig(config);

  static_cast<KTextEditor::ConfigInterface*>(kateView->document()->qt_cast("KTextEditor::ConfigInterface"))->writeConfig();
}

// session management
void KWrite::restore(KConfig *config, int n)
{
  if ((kateView->document()->views().count() == 1) && !kateView->document()->url().isEmpty()) { //in this case first view
    loadURL(kateView->document()->url());
  }
  readPropertiesInternal(config, n);
  init();
}

void KWrite::readProperties(KConfig *config)
{
  readConfig(config);
  static_cast<KTextEditor::ViewConfigInterface*>(kateView->qt_cast("KTextEditor::ViewConfigInterface"))->readSessionConfig(config);
}

void KWrite::saveProperties(KConfig *config)
{
  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.find((KTextEditor::Document *)kateView->document()) + 1);
  static_cast<KTextEditor::ViewConfigInterface*>(kateView->qt_cast("KTextEditor::ViewConfigInterface"))->writeSessionConfig(config);
}

void KWrite::saveGlobalProperties(KConfig *config) //save documents
{
  uint z;
  QString buf;
  KTextEditor::Document *doc;

  config->setGroup("Number");
  config->writeEntry("NumberOfDocuments",docList.count());

  for (z = 1; z <= docList.count(); z++) {
     buf = QString("Document%1").arg(z);
     config->setGroup(buf);
     doc = (KTextEditor::Document *) docList.at(z - 1);
     static_cast<KTextEditor::ConfigInterface*>(doc->qt_cast("KTextEditor::ConfigInterface"))->writeSessionConfig(config);
  }
}

//restore session
void restore()
{
  KConfig *config;
  int docs, windows, z;
  QString buf;
  KTextEditor::Document *doc;
  KWrite *t;
  KLibFactory *factory = KLibLoader::self()->factory( "libkatepart" );

  config = kapp->sessionConfig();
  if (!config) return;

  config->setGroup("Number");
  docs = config->readNumEntry("NumberOfDocuments");
  windows = config->readNumEntry("NumberOfWindows");

  for (z = 1; z <= docs; z++) {
     buf = QString("Document%1").arg(z);
     config->setGroup(buf);
     KTextEditor::Document *tmpDoc = (KTextEditor::Document *) factory->create (0L, "kate", "KTextEditor::Document");
     doc = (KTextEditor::Document *)tmpDoc; //new doc with default path
     static_cast<KTextEditor::ConfigInterface*>(doc->qt_cast("KTextEditor::ConfigInterface"))->readSessionConfig(config);
     docList.append(doc);
  }

  for (z = 1; z <= windows; z++) {
    buf = QString("%1").arg(z);
    config->setGroup(buf);
    t = new KWrite(docList.at(config->readNumEntry("DocumentNumber") - 1));
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
	I18N_NOOP( "KWrite - Lightweight Kate" ), KAboutData::License_GPL,
	I18N_NOOP( "(c) 2000-2001 The Kate Authors" ), 0, "http://kate.kde.org");

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
  aboutData.addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  aboutData.addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  aboutData.addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  aboutData.addCredit ("Daniel Naber","","");
  aboutData.addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  aboutData.addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  aboutData.addCredit ("Carsten Presser", I18N_NOOP("Betatest"), "mord-slime@gmx.de");
  aboutData.addCredit ("Jens Haupert", I18N_NOOP("Betatest"), "al_all@gmx.de");
  aboutData.addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  aboutData.addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

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
    KWrite *t;

    if ( args->count() == 0 )
    {
        t = new KWrite;
        t->readConfig();
        t->init();
    }
    else
    {
        for ( int i = 0; i < args->count(); ++i )
        {
            t = new KWrite();
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
