/***********************************************************************
 *
 *  Kfinddlg.cpp
 *
 **********************************************************************/

#include <qlayout.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kglobal.h>
#include <kguiitem.h>
#include <kstatusbar.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kaboutapplication.h>

#include "kftabdlg.h"
#include "kquery.h"
#include "kfwin.h"

#include "kfinddlg.h"
#include "kfinddlg.moc"

KfindDlg::KfindDlg(const KURL & url, QWidget *parent, const char *name)
  : KDialogBase( Plain, QString::null,
	User1 | User2 | User3 | Apply | Close | Help, User1,
        parent, name, true, true,
	KGuiItem(i18n("&Find"), "find"),
	KGuiItem(i18n("Stop"), "stop"),
	KStdGuiItem::saveAs())
{
  QWidget::setCaption( i18n("Find Files" ) );
  setButtonBoxOrientation(Vertical);

  enableButton(User1, true); // Enable "Search"
  enableButton(User2, false); // Disable "Stop"
  enableButton(User3, false); // Disable "Save..."

  setEscapeButton(User2);

  setButtonApplyText(i18n("About"));

  // remove misleading default help
  setButtonWhatsThis(Apply,QString::null);
  setButtonTip(Apply,QString::null);
  actionButton(Apply)->setIconSet(QIconSet());

  isResultReported = false;

  QFrame *frame = plainPage();

  // create tabwidget
  tabWidget = new KfindTabWidget( frame, "dialog");
  tabWidget->setURL( url );

  // prepare window for find results
  win = new KfindWindow(frame,"window");

  mStatusBar = new KStatusBar(frame);
  mStatusBar->insertFixedItem(i18n("AMiddleLengthText..."), 0, true);
  setStatusMsg(i18n("Ready."));
  mStatusBar->setItemAlignment(0, AlignLeft | AlignVCenter);
  mStatusBar->insertItem(QString::null, 1, 1, true);
  mStatusBar->setItemAlignment(1, AlignLeft | AlignVCenter);

  QVBoxLayout *vBox = new QVBoxLayout(frame);
  vBox->addWidget(tabWidget, 0);
  vBox->addWidget(win, 1);
  vBox->addWidget(mStatusBar, 0);

  connect(this, SIGNAL(user1Clicked()),
	  this, SLOT(startSearch()));
  connect(this, SIGNAL(user2Clicked()),
	  this, SLOT(stopSearch()));
  connect(this, SIGNAL(user3Clicked()),
	  win, SLOT(saveResults()));

  connect(win ,SIGNAL(resultSelected(bool)),
	  this,SIGNAL(resultSelected(bool)));

  connect(this, SIGNAL( applyClicked() ), this, SLOT(about()));

  query = new KQuery(frame);
  connect(query, SIGNAL(addFile(const KFileItem*,const QString&)),
	  SLOT(addFile(const KFileItem*,const QString&)));
  connect(query, SIGNAL(result(int)), SLOT(slotResult(int)));
  aboutWin = new KAboutApplication(this, "about", true);

  dirlister=new KDirLister();
  connect(dirlister, SIGNAL( deleteItem ( KFileItem* )), this, SLOT(slotDeleteItem(KFileItem*)));
  connect(dirlister, SIGNAL( newItems ( const KFileItemList& )), this, SLOT(slotNewItems( const KFileItemList& )));
  searching=false;
}

KfindDlg::~KfindDlg()
{
   stopSearch();
}

void KfindDlg::closeEvent(QCloseEvent *)
{
   slotClose();
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
  setProgressMsg(i18n("one file found", "%n files found", 0));

  emit resultSelected(false);
  emit haveResults(false);

  enableButton(User1, false); // Disable "Search"
  enableButton(User2, true); // Enable "Stop"
  enableButton(User3, false); // Disable "Save..."

  dirlister->openURL(query->url());

  //Getting a list of all subdirs
  if(tabWidget->isSearchRecursive())
  {
    depth=0;
    QStringList subdirs=getAllSubdirs(query->url().path());
    for(QStringList::Iterator it = subdirs.begin(); it != subdirs.end(); ++it)
      dirlister->openURL(KURL(*it),true);
  }
  
  searching=true;
  win->beginSearch(query->url());
  tabWidget->beginSearch();

  setStatusMsg(i18n("Searching..."));
  query->start();
}

void KfindDlg::stopSearch()
{
  query->kill();
  searching=false;
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
     KMessageBox::sorry( this, i18n("Could not find the specified directory."));
  }
  else
  {
     kdDebug()<<"KIO error code: "<<errorCode<<endl;
     setStatusMsg(i18n("Error."));
  };

  enableButton(User1, true); // Enable "Search"
  enableButton(User2, false); // Disable "Stop"
  enableButton(User3, true); // Enable "Save..."

  win->endSearch();
  tabWidget->endSearch();
  setFocus();

  searching=false;
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
  QString str = i18n("one file found", "%n files found", count);
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
  aboutWin->show(this);
  //delete aboutWin;
}

void KfindDlg::slotDeleteItem(KFileItem* item)
{
  kdDebug()<<QString("Will remove one item: %1").arg(item->url().path())<<endl;
  QListViewItem *iter;
  QString iterwithpath;

  iter=win->firstChild();
  while( iter ) {
    iterwithpath=query->url().path(+1)+iter->text(1)+iter->text(0);

    if(iterwithpath==item->url().path())
    {
      win->takeItem(iter);
      break;
    }
    iter = iter->nextSibling();
  }
}

void KfindDlg::slotNewItems( const KFileItemList& items )
{
  if(searching)
    return;
    
  kdDebug()<<QString("Will add some items")<<endl;
  KFileItemListIterator iter(items);
  QString iterwithpath;
  QStringList newfiles;
  QListViewItem *checkiter;
  QString checkiterwithpath;
  bool redundant=false;
  
  while(iter.current()!=0)
  {
    iterwithpath=iter.current()->url().path(+1);
    if(iterwithpath.find(query->url().path(+1))==0)
    {
      kdDebug()<<QString("Can be added, path OK")<<endl;
      //New items appears twice ???

      checkiter=win->firstChild();
      while( checkiter ) {
        checkiterwithpath=query->url().path(+1)+checkiter->text(1)+checkiter->text(0);
        if(iterwithpath==checkiterwithpath)
        {
          redundant=true;
          break;
        }
        checkiter = checkiter->nextSibling();
      }
      //
      if(!redundant)
        newfiles.append(iterwithpath);
    }
    ++iter;
  }
  
  if(!newfiles.isEmpty())
  {
    query->slotListEntries(newfiles);
  }
}

QStringList KfindDlg::getAllSubdirs(QDir d)
{
  QStringList dirs;
  QStringList subdirs;

  if(depth>50)//otherwise, it's too slow
    return NULL;
    
  d.setFilter( QDir::Dirs );
  dirs = d.entryList();

  for(QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it)
  {
    if((*it==".")||(*it==".."))
      continue;
    subdirs.append(d.path()+"/"+*it);
    depth++;
    subdirs+=getAllSubdirs(d.path()+"/"+*it);
  }

  return subdirs;
}
