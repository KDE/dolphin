/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include <qdropsite.h>
#include <qdragobject.h>
#include <qvbox.h>

#include <ktexteditor/configinterface.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/printinterface.h>
#include <ktexteditor/encodinginterface.h>
#include <ktexteditor/editorchooser.h>

#include <kate/document.h>

#include <dcopclient.h>
#include <kurldrag.h>
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

#include <kio/netaccess.h>

#include "kwritemain.h"
#include "kwritemain.moc"

#include "../utils/filedialog.h"
#include "kwritedialogs.h"
#include <qtimer.h>

#include <stdlib.h> // exit

// StatusBar field IDs
#define ID_GEN 1

QPtrList<KTextEditor::Document> KWrite::docList;

KWrite::KWrite (KTextEditor::Document *doc)
    : m_kateView(0),
      m_recentFiles(0),
      m_paShowPath(0),
      m_paShowStatusBar(0)
{
  setMinimumSize(200,200);

  if (!initialGeometrySet())
     resize(640,400);

  if (!doc)
  {
    doc=KTextEditor::EditorChooser::createDocument(this,"KTextEditor::Document");
    docList.append(doc);
  }

  setupEditWidget(doc);
  setupActions();
  setupStatusBar();

  setAcceptDrops(true);

  setXMLFile( "kwriteui.rc" );
  createShellGUI( true );
  guiFactory()->addClient( m_kateView );
  KParts::GUIActivateEvent ev( true );
  QApplication::sendEvent( m_kateView, &ev );

  // Read basic main-view settings, and set to autosave
  setAutoSaveSettings( "General Options" );
}


KWrite::~KWrite()
{
  if (m_kateView->document()->views().count() == 1) docList.remove(m_kateView->document());
}


void KWrite::init()
{
/*  KToolBar *tb = toolBar("mainToolBar");
  if (tb) m_paShowToolBar->setChecked( !tb->isHidden() );
    else m_paShowToolBar->setEnabled(false);*/
  KStatusBar *sb = statusBar();
  if (sb) m_paShowStatusBar->setChecked( !sb->isHidden() );
    else m_paShowStatusBar->setEnabled(false);

  show();
}


void KWrite::loadURL(const KURL &url)
{
  m_recentFiles->addURL( url );
  m_kateView->document()->openURL(url);
}


bool KWrite::queryClose()
{
  return m_kateView->document()->views().count() != 1
      || m_kateView->document()->queryClose();
}


bool KWrite::queryExit()
{
  writeConfig();
  kapp->config()->sync();

  return true;
}


void KWrite::setupEditWidget(KTextEditor::Document *doc)
{
  if (!doc)
  {
    KMessageBox::error(this, i18n("A KDE text editor component could not be found!\n"
                                  "Please check your KDE installation."));
    exit(1);
  }

  m_kateView = doc->createView (this, 0L);

  connect(m_kateView,SIGNAL(newStatus()),this,SLOT(newCaption()));
  connect(m_kateView,SIGNAL(viewStatusMsg(const QString &)),this,SLOT(newStatus(const QString &)));
  connect(m_kateView->document(),SIGNAL(fileNameChanged()),this,SLOT(newCaption()));
  connect(m_kateView,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(slotDropEvent(QDropEvent *)));

  setCentralWidget(m_kateView);

  KStdAction::close( this, SLOT(slotFlush()), actionCollection(), "file_close" )->setWhatsThis(i18n("Use this to close the current document"));
}

void KWrite::changeEditor()
{
	KWriteEditorChooser choose(this);
	choose.exec();
}

void KWrite::slotFlush ()
{
   m_kateView->document()->closeURL();
}

void KWrite::setupActions()
{
  KAction *a;

  // setup File menu
  KStdAction::print(this, SLOT(printDlg()), actionCollection())->setWhatsThis(i18n("Use this command to print the current document"));
  KStdAction::openNew( this, SLOT(slotNew()), actionCollection(), "file_new" )->setWhatsThis(i18n("Use this command to create a new document"));
  KStdAction::open( this, SLOT( slotOpen() ), actionCollection(), "file_open" )->setWhatsThis(i18n("Use this command to open an existing document for editing"));
//  m_recentFiles = KStdAction::openRecent(this, SLOT(slotOpen(const KURL&)),
//                                        actionCollection());

  m_recentFiles = KStdAction::openRecent(this, SLOT(slotOpen(const KURL&)),
                                         actionCollection());
  m_recentFiles->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a=new KAction(i18n("New &View"), 0, this, SLOT(newView()),
              actionCollection(), "file_newView");
  a->setWhatsThis(i18n("Create another view containing the current document"));

  a=new KAction(i18n("Choose Editor..."),0,this,SLOT(changeEditor()),
		actionCollection(),"settings_choose_editor");
  a->setWhatsThis(i18n("Override the system wide setting for the default editing component"));

  KStdAction::quit(this, SLOT(close()), actionCollection())->setWhatsThis(i18n("Close the current document view"));


  // setup Settings menu
  //m_paShowToolBar = KStdAction::showToolbar( this, SLOT( toggleToolBar() ), actionCollection(), "settings_show_toolbar" );
  setStandardToolBarMenuEnabled(true);

  m_paShowStatusBar = KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection(), "settings_show_statusbar");
  m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));

  m_paShowPath = new KToggleAction(i18n("Sho&w Path"), 0, this, SLOT(newCaption()),
                    actionCollection(), "set_showPath");
  m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));
  a=KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection(), "set_configure_toolbars");
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));
}

void KWrite::setupStatusBar()
{
  KStatusBar *statusbar;
  statusbar = statusBar();
  statusbar->insertItem("", ID_GEN);
}

void KWrite::slotNew()
{
  if (m_kateView->document()->isModified() || !m_kateView->document()->url().isEmpty())
  {
   KWrite*t = new KWrite();
    t->readConfig();
    t->init();
  }
  else
    m_kateView->document()->openURL("");
}

void KWrite::slotOpen()
{
  if (KTextEditor::encodingInterface(m_kateView->document()))
  {
    Kate::FileDialog *dialog = new Kate::FileDialog (QString::null,KTextEditor::encodingInterface(m_kateView->document())->encoding(), this, i18n ("Open File"));
    Kate::FileDialogData data = dialog->exec ();
    delete dialog;

    for (KURL::List::Iterator i=data.urls.begin(); i != data.urls.end(); ++i)
    {
      encoding = data.encoding;
      slotOpen ( *i );
    }
  }
  else
  {
    KURL::List l=KFileDialog::getOpenURLs(QString::null,QString::null,this,QString::null);
    for (KURL::List::Iterator i=l.begin(); i != l.end(); ++i)
    {
      slotOpen ( *i );
    }
  }
}

void KWrite::slotOpen( const KURL& url )
{
  if (url.isEmpty()) return;

  if (!KIO::NetAccess::exists(url, true, this))
  {
    KMessageBox::error (this, i18n("The given file could not be read, check if it exists or if it is readable for the current user."));
    return;
  }

  if (m_kateView->document()->isModified() || !m_kateView->document()->url().isEmpty())
  {
    KWrite *t = new KWrite();
    if (KTextEditor::encodingInterface(m_kateView->document())) KTextEditor::encodingInterface(m_kateView->document())->setEncoding(encoding);
    t->readConfig();
    t->init();
    t->loadURL(url);
  }
  else
  {
    if (KTextEditor::encodingInterface(m_kateView->document())) KTextEditor::encodingInterface(m_kateView->document())->setEncoding(encoding);
    loadURL(url);
  }
}

void KWrite::newView()
{
  KWrite *t = new KWrite(m_kateView->document());
  t->readConfig();
  t->init();
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
  if( m_kateView )
    dlg.insert(m_kateView->actionCollection());
  dlg.configure();
}

void KWrite::editToolbars()
{
  KEditToolbar *dlg = new KEditToolbar(guiFactory());

  if (dlg->exec())
  {
      KParts::GUIActivateEvent ev1( false );
      QApplication::sendEvent( m_kateView, &ev1 );
      guiFactory()->removeClient( m_kateView );
      createShellGUI( false );
      createShellGUI( true );
      guiFactory()->addClient( m_kateView );
      KParts::GUIActivateEvent ev2( true );
      QApplication::sendEvent( m_kateView, &ev2 );
  }
  delete dlg;
}

void KWrite::printNow()
{
  KTextEditor::printInterface(m_kateView->document())->print ();
}

void KWrite::printDlg()
{
  KTextEditor::printInterface(m_kateView->document())->printDialog ();
}

void KWrite::newStatus(const QString &msg)
{
  newCaption();

  statusBar()->changeItem(msg,ID_GEN);
}

void KWrite::newCaption()
{
  if (m_kateView->document()->url().isEmpty()) {
    setCaption(i18n("Untitled"),m_kateView->document()->isModified());
  }
  else
  {
    QString c;
    if (!m_paShowPath->isChecked())
    {
      c = m_kateView->document()->url().filename();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = c.left(64) + "...";
    }
    else
    {
      c = m_kateView->document()->url().prettyURL();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = "..." + c.right(64);
    }

    setCaption (c, m_kateView->document()->isModified());
  }
}

void KWrite::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept(KURLDrag::canDecode(event));
}


void KWrite::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}


void KWrite::slotDropEvent( QDropEvent *event )
{
  KURL::List textlist;
  if (!KURLDrag::decode(event, textlist)) return;

  for (KURL::List::Iterator i=textlist.begin(); i != textlist.end(); ++i)
  {
    slotOpen (*i);
  }
}

void KWrite::slotEnableActions( bool enable )
{
    QValueList<KAction *> actions = actionCollection()->actions();
    QValueList<KAction *>::ConstIterator it = actions.begin();
    QValueList<KAction *>::ConstIterator end = actions.end();
    for (; it != end; ++it )
        (*it)->setEnabled( enable );

    actions = m_kateView->actionCollection()->actions();
    it = actions.begin();
    end = actions.end();
    for (; it != end; ++it )
        (*it)->setEnabled( enable );
}

//common config
void KWrite::readConfig(KConfig *config)
{
  config->setGroup("General Options");

  m_paShowPath->setChecked( config->readBoolEntry("ShowPath") );
  m_recentFiles->loadEntries(config, "Recent Files");

  if (m_kateView && KTextEditor::configInterface(m_kateView->document()))
    KTextEditor::configInterface(m_kateView->document())->readConfig(config);
}


void KWrite::writeConfig(KConfig *config)
{
  config->setGroup("General Options");

  if (m_paShowPath)
    config->writeEntry("ShowPath",m_paShowPath->isChecked());

  if (m_recentFiles)
    m_recentFiles->saveEntries(config, "Recent Files");

  if (m_kateView && KTextEditor::configInterface(m_kateView->document()))
    KTextEditor::configInterface(m_kateView->document())->writeConfig(config);
}


//config file
void KWrite::readConfig()
{
  KConfig *config = kapp->config();
  readConfig(config);
}


void KWrite::writeConfig()
{
  KConfig *config = kapp->config();
  writeConfig(config);
}

// session management
void KWrite::restore(KConfig *config, int n)
{
  if ((m_kateView->document()->views().count() == 1) && !m_kateView->document()->url().isEmpty()) { //in this case first view
    loadURL(m_kateView->document()->url());
  }
 readPropertiesInternal(config, n);
  init();
}

void KWrite::readProperties(KConfig *config)
{
  readConfig(config);

  if (KTextEditor::sessionConfigInterface(m_kateView))
    KTextEditor::sessionConfigInterface(m_kateView)->readSessionConfig(config);
}

void KWrite::saveProperties(KConfig *config)
{
  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.find(m_kateView->document()) + 1);

  if (KTextEditor::sessionConfigInterface(m_kateView))
    KTextEditor::sessionConfigInterface(m_kateView)->writeSessionConfig(config);
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
     doc = docList.at(z - 1);

     if (KTextEditor::configInterface(doc))
       KTextEditor::configInterface(doc)->writeSessionConfig(config);
  }
}

//restore session
void KWrite::restore()
{
  KConfig *config;
  int docs, windows, z;
  QString buf;
  KTextEditor::Document *doc;
  KWrite *t;

  config = kapp->sessionConfig();
  if (!config) return;

  config->setGroup("Number");
  docs = config->readNumEntry("NumberOfDocuments");
  windows = config->readNumEntry("NumberOfWindows");

  for (z = 1; z <= docs; z++) {
     buf = QString("Document%1").arg(z);
     config->setGroup(buf);
     doc = KTextEditor::createDocument ("libkatepart");

     if (KTextEditor::configInterface(doc))
       KTextEditor::configInterface(doc)->readSessionConfig(config);
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
  { "stdin",    I18N_NOOP("Read the contents of stdin."), 0},
  { "+[URL]",   I18N_NOOP("Document to open."), 0 },
  KCmdLineLastOption
};

extern "C" int kdemain(int argc, char **argv)
{
  KLocale::setMainCatalogue("kate");         //lukas: set this to have the kwritepart translated using kate message catalog

  KAboutData *s_about = new KAboutData ("kwrite", I18N_NOOP("KWrite"), "4.2",
	I18N_NOOP( "KWrite - Simple Text Editor" ), KAboutData::License_LGPL_V2,
	I18N_NOOP( "(c) 2000-2003 The Kate Authors" ), 0, "http://kate.kde.org");

  s_about->addAuthor ("Christoph Cullmann", I18N_NOOP("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
    s_about->addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
    s_about->addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
    s_about->addAuthor ("Hamish Rodda",I18N_NOOP("Core Developer"), "meddie@yoyo.its.monash.edu.au");
    s_about->addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
    s_about->addAuthor ("Charles Samuels", I18N_NOOP("The Editing Commands"), "charles@kde.org");
    s_about->addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
    s_about->addAuthor ("Michael Bartl", I18N_NOOP("Former Core Developer"), "michael.bartl1@chello.at");
    s_about->addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
    s_about->addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
    s_about->addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
    s_about->addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
    s_about->addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
    s_about->addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
    s_about->addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
    s_about->addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");

    s_about->addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
    s_about->addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
    s_about->addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
    s_about->addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
    s_about->addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
    s_about->addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
    s_about->addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
    s_about->addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
    s_about->addCredit ("Daniel Naber","","");
    s_about->addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
    s_about->addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
    s_about->addCredit ("Carsten Presser", I18N_NOOP("Betatest"), "mord-slime@gmx.de");
    s_about->addCredit ("Jens Haupert", I18N_NOOP("Betatest"), "al_all@gmx.de");
    s_about->addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
    s_about->addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

    s_about->setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  KCmdLineArgs::init( argc, argv, s_about );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication a;

  KGlobal::locale()->insertCatalogue("katepart");

  DCOPClient *client = kapp->dcopClient();
  if (!client->isRegistered())
  {
    client->attach();
    client->registerAs("kwrite");
  }

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (kapp->isRestored())
  {
    KWrite::restore();
  }
  else
  {
    KWrite *t;

    if ( args->count() == 0 )
    {
        t = new KWrite;
        t->readConfig();

        if( args->isSet( "stdin" ) ) {
            QTextIStream input(stdin);
            QString line;
            QString text;
            do {
                line = input.readLine();
                text.append( line + "\n" );
            } while( !line.isNull() );


            KTextEditor::EditInterface *doc = KTextEditor::editInterface (t->kateView()->document());
            if( doc ) {
                doc->setText( text );
            }
        }

        t->init();
    }
    else
    {
        for ( int i = 0; i < args->count(); ++i )
        {
            t = new KWrite();

	    if (Kate::document (t->kateView()->document()))
	      Kate::Document::setOpenErrorDialogsActivated (false);

            t->readConfig();
            t->loadURL( args->url( i ) );
            t->init();

	    if (Kate::document (t->kateView()->document()))
	      Kate::Document::setOpenErrorDialogsActivated (true);
	}
    }
  }

  int r = a.exec();

  args->clear();
  return r;
}
