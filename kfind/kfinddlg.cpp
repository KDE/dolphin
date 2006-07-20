/***********************************************************************
 *
 *  Kfinddlg.cpp
 *
 **********************************************************************/

#include <QLayout>
#include <QPushButton>

#include <klocale.h>
#include <kglobal.h>
#include <kguiitem.h>
#include <kstatusbar.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kaboutapplication.h>
#include <kstandarddirs.h>

#include "kftabdlg.h"
#include "kquery.h"
#include "kfwin.h"

#include "kfinddlg.h"
#include "kfinddlg.moc"

KfindDlg::KfindDlg(const KUrl & url, QWidget *parent, const char *name)
  : KDialog( parent )
{
  setButtons( User1 | User2 | Apply | Close | Help );
  setDefaultButton( Apply );
  setObjectName( name );
  setModal( true );
  showButtonSeparator( true );

  setButtonGuiItem( User1, KStdGuiItem::stop() );
  setButtonGuiItem( User2, KStdGuiItem::saveAs() );
  setButtonGuiItem( Apply, KStdGuiItem::find());

  QWidget::setWindowTitle( i18n("Find Files/Folders" ) );
  setButtonsOrientation(Qt::Horizontal);

  enableButton(Apply, true); // Enable "Find"
  enableButton(User1, false); // Disable "Stop"
  enableButton(User2, false); // Disable "Save As..."

  isResultReported = false;

  QFrame *frame = new QFrame;
  setMainWidget( frame );

  // create tabwidget
  tabWidget = new KfindTabWidget( frame, "dialog");
  tabWidget->setURL( url );

  // prepare window for find results
  win = new KfindWindow(frame );
  win->setObjectName( "window" );

  mStatusBar = new KStatusBar(frame);
  mStatusBar->insertFixedItem(i18n("AMiddleLengthText..."), 0);
  setStatusMsg(i18n("Ready."));
  mStatusBar->setItemAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
  mStatusBar->insertPermanentItem(QString(), 1, 1);
  mStatusBar->setItemAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);

  QVBoxLayout *vBox = new QVBoxLayout(frame);
  vBox->addWidget(tabWidget, 0);
  vBox->addWidget(win, 1);
  vBox->addWidget(mStatusBar, 0);

  connect(this, SIGNAL(applyClicked()),
	  this, SLOT(startSearch()));
  connect(this, SIGNAL(user1Clicked()),
	  this, SLOT(stopSearch()));
  connect(this, SIGNAL(user2Clicked()),
	  win, SLOT(saveResults()));

  connect(win ,SIGNAL(resultSelected(bool)),
	  this,SIGNAL(resultSelected(bool)));

  query = new KQuery(frame);
  connect(query, SIGNAL(addFile(const KFileItem*,const QString&)),
	  SLOT(addFile(const KFileItem*,const QString&)));
  connect(query, SIGNAL(result(int)), SLOT(slotResult(int)));

  dirwatch=NULL;
}

KfindDlg::~KfindDlg()
{
   stopSearch();
}

void KfindDlg::closeEvent(QCloseEvent *)
{
   slotButtonClicked(KDialog::Close);
}

void KfindDlg::setProgressMsg(const QString &msg)
{
   mStatusBar->changeItem(msg, 1);
}

void KfindDlg::setStatusMsg(const QString &msg)
{
   mStatusBar->changeItem(msg, 0);
}


void KfindDlg::startSearch()
{
  tabWidget->setQuery(query);

  isResultReported = false;

  // Reset count - use the same i18n as below
  setProgressMsg(i18np("one file found", "%n files found", 0));

  emit resultSelected(false);
  emit haveResults(false);

  enableButton(Apply, false); // Disable "Find"
  enableButton(User1, true); // Enable "Stop"
  enableButton(User2, false); // Disable "Save As..."

  if(dirwatch!=NULL)
    delete dirwatch;
  dirwatch=new KDirWatch();
  connect(dirwatch, SIGNAL(created(const QString&)), this, SLOT(slotNewItems(const QString&)));
  connect(dirwatch, SIGNAL(deleted(const QString&)), this, SLOT(slotDeleteItem(const QString&)));
  dirwatch->addDir(query->url().path(),true);

#if 0
  // waba: Watching for updates is disabled for now because even with FAM it causes too
  // much problems. See BR68220, BR77854, BR77846, BR79512 and BR85802
  // There are 3 problems:
  // 1) addDir() keeps looping on recursive symlinks
  // 2) addDir() scans all subdirectories, so it basically does the same as the process that
  // is started by KQuery but in-process, undoing the advantages of using a separate find process
  // A solution could be to let KQuery emit all the directories it has searched in.
  // Either way, putting dirwatchers on a whole file system is probably just too much.
  // 3) FAM has a tendency to deadlock with so many files (See BR77854) This has hopefully
  // been fixed in KDirWatch, but that has not yet been confirmed.

  //Getting a list of all subdirs
  if(tabWidget->isSearchRecursive() && (dirwatch->internalMethod() == KDirWatch::FAM))
  {
    QStringList subdirs=getAllSubdirs(query->url().path());
    for(QStringList::Iterator it = subdirs.begin(); it != subdirs.end(); ++it)
      dirwatch->addDir(*it,true);
  }
#endif

  win->beginSearch(query->url());
  tabWidget->beginSearch();

  setStatusMsg(i18n("Searching..."));
  query->start();
}

void KfindDlg::stopSearch()
{
  query->kill();
}

void KfindDlg::newSearch()
{
  // WABA: Not used any longer?
  stopSearch();

  tabWidget->setDefaults();

  emit haveResults(false);
  emit resultSelected(false);

  setFocus();
}

void KfindDlg::slotResult(int errorCode)
{
  if (errorCode == 0)
    setStatusMsg(i18n("Ready."));
  else if (errorCode == KIO::ERR_USER_CANCELED)
    setStatusMsg(i18n("Aborted."));
  else if (errorCode == KIO::ERR_MALFORMED_URL)
  {
     setStatusMsg(i18n("Error."));
     KMessageBox::sorry( this, i18n("Please specify an absolute path in the \"Look in\" box."));
  }
  else if (errorCode == KIO::ERR_DOES_NOT_EXIST)
  {
     setStatusMsg(i18n("Error."));
     KMessageBox::sorry( this, i18n("Could not find the specified folder."));
  }
  else
  {
     kDebug()<<"KIO error code: "<<errorCode<<endl;
     setStatusMsg(i18n("Error."));
  };

  enableButton(Apply, true); // Enable "Find"
  enableButton(User1, false); // Disable "Stop"
  enableButton(User2, true); // Enable "Save As..."

  win->endSearch();
  tabWidget->endSearch();
  setFocus();

}

void KfindDlg::addFile(const KFileItem* item, const QString& matchingLine)
{
  win->insertItem(*item,matchingLine);

  if (!isResultReported)
  {
    emit haveResults(true);
    isResultReported = true;
  }

  int count = win->childCount();
  QString str = i18np("one file found", "%n files found", count);
  setProgressMsg(str);
}

void KfindDlg::setFocus()
{
  tabWidget->setFocus();
}

void KfindDlg::copySelection()
{
  win->copySelection();
}

void  KfindDlg::about ()
{
  KAboutApplication dlg(0, this, true);
  dlg.exec ();
}

void KfindDlg::slotDeleteItem(const QString& file)
{
  kDebug()<<QString("Will remove one item: %1").arg(file)<<endl;
  Q3ListViewItem *iter;
  QString iterwithpath;

  iter=win->firstChild();
  while( iter ) {
    iterwithpath=query->url().path(KUrl::AddTrailingSlash)+iter->text(1)+iter->text(0);

    if(iterwithpath==file)
    {
      win->takeItem(iter);
      break;
    }
    iter = iter->nextSibling();
  }
}

void KfindDlg::slotNewItems( const QString& file )
{
  kDebug()<<QString("Will add this item")<<endl;
  QStringList newfiles;
  Q3ListViewItem *checkiter;
  QString checkiterwithpath;

  if(file.indexOf(query->url().path(KUrl::AddTrailingSlash))==0)
  {
    kDebug()<<QString("Can be added, path OK")<<endl;
    checkiter=win->firstChild();
    while( checkiter ) {
      checkiterwithpath=query->url().path(KUrl::AddTrailingSlash)+checkiter->text(1)+checkiter->text(0);
      if(file==checkiterwithpath)
        return;
      checkiter = checkiter->nextSibling();
    }
    query->slotListEntries(QStringList(file));
  }
}

QStringList KfindDlg::getAllSubdirs(QDir d)
{
  QStringList dirs;
  QStringList subdirs;

  d.setFilter( QDir::Dirs );
  dirs = d.entryList();

  for(QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it)
  {
    if((*it==".")||(*it==".."))
      continue;
    subdirs.append(d.path()+'/'+*it);
    subdirs+=getAllSubdirs(d.path()+'/'+*it);
  }
  return subdirs;
}
