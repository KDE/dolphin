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

#include <qapplication.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qcolor.h>
#include <stdio.h>
#include <qstring.h>
#include <qdir.h>
#include <qkeycode.h>
#include <qlayout.h>

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

  QVBoxLayout *vBox = new QVBoxLayout(this);

  //create tabwidget
  tabWidget = new KfindTabWidget(this,"dialog",searchPath);

  //prepare window for find results
  win = new KfindWindow(this,"window");
  win->hide();  //and hide it firstly    

  vBox->addWidget(tabWidget);
  vBox->addWidget(win);
  vBox->activate();

  setExpanded(false);

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
  
}


Kfind::~Kfind() {
  if(iBuffer)
    delete [] iBuffer;
}


void Kfind::copySelection() {
  win->copySelection();
}


void Kfind::startSearch() {
  // init buffer
  if(iBuffer)
    delete [] iBuffer;
    
  iBuffer = new char[IBUFSIZE];
  iBuffer[0] = 0;

  QString buffer,pom;
  // If this method returns NULL a error occured and 
  // the error message was presented to the user. We just exit.
  buffer = tabWidget->createQuery();
  if(buffer == NULL)
    return;

  emit resultSelected(false);
  win->clearList();
  win->show();
  emit haveResults(false);
    
  win->beginSearch();
  tabWidget->beginSearch();

  if (!buffer.isNull())
    {
      enableSearchButton(false);

      findProcess.clearArguments ();
      QString cmdline = buffer;
      findProcess.setExecutable(cmdline);
	
      findProcess.start(KProcess::NotifyOnExit, KProcess::AllOutput);
    };
};


void Kfind::stopSearch() {
  //    printf("Stoping Search\n");
  tabWidget->endSearch();
  win->doneSearch();
    
  enableSearchButton(true);

  findProcess.kill();
}


void Kfind::newSearch() {
  // re-init buffer
  if(iBuffer)
    iBuffer[0] = 0;

    //    printf("Prepare for New Search\n");
  win->hide(); // !!!!!
  win->clearList();

  tabWidget->setDefaults();

  emit haveResults(false);
  emit resultSelected(false);
  setExpanded(false);

  stopSearch();
  tabWidget->endSearch();
}


void Kfind::handleStdout(KProcess *, char *buffer, int buflen) {
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
      setExpanded(false);
    memmove(iBuffer, p+1, strlen(p + 1)+1);
  }
}


void Kfind::processResults() {
  win->show();
  win->doneSearch();
  tabWidget->endSearch();
    
  emit haveResults(true);
  setExpanded(true);

  enableSearchButton(true);
}
void Kfind::setExpanded(bool expand) {
  printf("Do it%d \n", expand);

  if(expand) {
    setMinimumSize(tabWidget->sizeHint().width(), 
		   2*tabWidget->sizeHint().height());
    setMaximumHeight(5000);
    //    win->clearList();
    win->show();
  }
  else {
    win->hide();
    setMinimumSize(tabWidget->sizeHint());
    setMaximumHeight(tabWidget->sizeHint().height());
  }

  emit enableStatusBar(expand);
}


