/***********************************************************************
 *
 *  Kfind.cpp
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <qapp.h>
#include <qwidget.h>
#include <qpushbt.h>
#include <qcolor.h>
#include <stdio.h>
#include <qevent.h>
#include <qstring.h>
#include <qdir.h>
#include <qkeycode.h>

#include <kmsgbox.h>
#include <kprocess.h>

#include "kftabdlg.h"
#include "kfwin.h"
#include "kfind.h"
#include <config.h>

// this should be enough to hold at least two fully 
// qualified pathnames. 
#define IBUFSIZE 16384

Kfind::Kfind( QWidget *parent, const char *name, const char *searchPath )
    : QWidget( parent, name )
  {
    // init IO buffer
    iBuffer = 0;

    //create tabdialog
    tabDialog = new KfindTabDialog(this,"dialog",searchPath);

    //prepare window for find results
    win = new KfindWindow(this,"window");
    win->hide();  //and hide it firstly    
    winsize=1;

    connect(win ,SIGNAL(resultSelected(bool)),
	    this,SIGNAL(resultSelected(bool)));
    connect(win ,SIGNAL(statusChanged(const char *)),
	    this,SIGNAL(statusChanged(const char *)));
    connect(this,SIGNAL(deleteFile()),
	    win,SLOT(deleteFiles()));
    connect(this,SIGNAL(properties()),
	    win,SLOT(fileProperties()));
    connect(this,SIGNAL(openFolder()),
	    win,SLOT(openFolder()));
    connect(this,SIGNAL(saveResults()),
	    win,SLOT(saveResults()));
    connect(this,SIGNAL(addToArchive()),
	    win,SLOT(addToArchive()));
    connect(this,SIGNAL(open()),
	    win,SLOT(openBinding()));
    connect(parentWidget(),SIGNAL(selectAll()),
	    win,SLOT(selectAll()));
    connect(parentWidget(),SIGNAL(unselectAll()),
	    win,SLOT(unselectAll()));
    connect(parentWidget(),SIGNAL(invertSelection()),
	    win,SLOT(invertSelection()));
    connect(&findProcess,SIGNAL(processExited(KProcess *)),
	    this,SLOT(processResults()));
    connect(&findProcess,SIGNAL(receivedStdout(KProcess *, char *, int)), 
	    this, SLOT(handleStdout(KProcess *, char *, int))) ;
    
    resize(sizeHint());
    //emit haveResults(false); // we're not connectd to anything yet!?
  };

Kfind::~Kfind() {
  if(iBuffer)
    delete iBuffer;
}

void Kfind::copySelection() {
  win->copySelection();
}

void Kfind::resizeEvent( QResizeEvent *e)
  {
    QWidget::resizeEvent(e);

    tabDialog->setGeometry(0,0,
			   width(),(tabDialog->sizeHint()).height());
    win->setGeometry(0,(tabDialog->sizeHint()).height(),width(),
    		     height()-(tabDialog->sizeHint()).height());
  };
    
void Kfind::startSearch()
  {
    // init buffer
    if(iBuffer)
      free(iBuffer);
    
    iBuffer = new char[IBUFSIZE];
    iBuffer[0] = 0;

    QString buffer,pom;
    //int pos;
    buffer = tabDialog->createQuery();

    if ( winsize==1)
      winsize=200;

    emit resultSelected(false);
    win->clearList();
    win->show();
    emit haveResults(false);
    
    win->beginSearch();
    tabDialog->beginSearch();

    if (!buffer.isNull())
      {
	enableSearchButton(false);

	findProcess.clearArguments ();
	QString cmdline = buffer;
	findProcess.setExecutable(cmdline);
	
	findProcess.start(KProcess::NotifyOnExit, KProcess::AllOutput);
      };
  };

void Kfind::stopSearch()
  {
    //    printf("Stoping Search\n");
    tabDialog->endSearch();
    win->doneSearch();
    
    enableSearchButton(true);

    findProcess.kill();
  };

void Kfind::newSearch()
  {
    // re-init buffer
    if(iBuffer)
      iBuffer[0] = 0;

    //    printf("Prepare for New Search\n");
    win->hide(); // !!!!!
    win->clearList();
    winsize=1;

    tabDialog->setDefaults();

    emit enableStatusBar(false);
    emit haveResults(false);
    emit resultSelected(false);
     
    stopSearch();
    tabDialog->endSearch();
 };

void Kfind::handleStdout(KProcess *, char *buffer, int buflen)
{
  // copy data to I/O buffer
  int len = strlen(iBuffer);
  memcpy(iBuffer + len, buffer, buflen);
  iBuffer[len + buflen] = 0;
  
  // split buffer: too expensive, improve
  char *p;
  while(( p = strchr(iBuffer, '\n')) != 0) {
    *p = 0;

    // found one file, append it to listbox
    win->appendResult(iBuffer);
    if(win->numItems() == 1)
      emit enableStatusBar(true);
    memmove(iBuffer, p+1, strlen(p + 1)+1);
  }
}

void Kfind::processResults()
  {
    win->show();
    win->doneSearch();
    tabDialog->endSearch();
    
    emit haveResults(true);
    emit enableStatusBar(true);

    enableSearchButton(true);
  };

QSize Kfind::sizeHint()
{
  QSize s;
  s = tabDialog->sizeHint() + QSize(0,winsize-1); // this just doesn't work
  s.setWidth(520);
  return s;
  };





