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
	KGuiItem(i18n("Save As..."), "filesaveas"))
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
  setStatusMsg(i18n("Searching..."));

  emit resultSelected(false);
  emit haveResults(false);

  enableButton(User1, false); // Disable "Search"
  enableButton(User2, true); // Enable "Stop"
  enableButton(User3, false); // Disable "Save..."

  win->beginSearch(query->url());
  tabWidget->beginSearch();

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
