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

#include <klocale.h>
#include <kprocess.h>

#include "kftabdlg.h"
#include "kfwin.h"
#include "kfind.h"

// this should be enough to hold at least two fully 
// qualified pathnames. 
#define IBUFSIZE 16384

Kfind::Kfind( QWidget *parent, const char *name, const char *searchPath )
  : QWidget( parent, name )
{
  // init IO buffer
  iBuffer = new char[IBUFSIZE];
  isResultReported = false;
  
  // create tabwidget
  tabWidget = new KfindTabWidget(this,"dialog",searchPath);

  // prepare window for find results
  win = new KfindWindow(this,"window");
  
  QVBoxLayout *vBox = new QVBoxLayout(this);
  vBox->addWidget(tabWidget);
  vBox->addWidget(win);
  vBox->activate();

  setExpanded(false);

  connect(win ,SIGNAL(resultSelected(bool)),
	  this,SIGNAL(resultSelected(bool)));
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
  
  // We want to use /bin/sh as a shell. With tcsh and csh 
  // find process can not be stopped by kill() call.
  findProcess = new KShellProcess("/bin/sh");
  
  connect(findProcess, SIGNAL(processExited(KProcess *)),
	  this, SLOT(stopSearch()));
  connect(findProcess, SIGNAL(receivedStdout(KProcess *, char *, int)), 
	  this, SLOT(handleStdout(KProcess *, char *, int))) ;
}

Kfind::~Kfind() {
  delete [] iBuffer;
  delete findProcess;
}

void Kfind::startSearch() {

  // If this method returns NULL a error occured and 
  // the error message was presented to the user. We just exit.
  QString cmdline = tabWidget->createQuery();
  if(cmdline == NULL)
    return;
  
  iBuffer[0] = 0;
  isResultReported = false;
  
  // Reset count
  QString str = i18n("%1 file(s) found").arg(0);
  emit statusChanged(str.ascii());

  emit resultSelected(false);
  emit haveResults(false);
  emit enableSearchButton(false);
  
  win->beginSearch();
  tabWidget->beginSearch();

  setExpanded(true);
  
  findProcess->clearArguments();
  findProcess->setExecutable(cmdline);
  findProcess->start(KProcess::NotifyOnExit, KProcess::AllOutput);
}

void Kfind::stopSearch() {
  emit enableSearchButton(true);

  win->endSearch();
  tabWidget->endSearch();

  if(findProcess->isRunning())
    findProcess->kill();
  
  setFocus();
}

void Kfind::newSearch() {

  stopSearch();

  tabWidget->setDefaults();

  emit haveResults(false);
  emit resultSelected(false);

  setExpanded(false);
  setFocus();
}

void Kfind::handleStdout(KProcess *, char *buffer, int buflen) {
  
  // If find process has been stopped ignore rest of the input
  if(!findProcess->isRunning())
    return;
  
  // copy data to I/O buffer
  int len = strlen(iBuffer);
  memcpy(iBuffer + len, buffer, buflen);
  iBuffer[len + buflen] = 0;
  
  // split buffer: too expensive, improve
  char *p;
  while(( p = strchr(iBuffer, '\n')) != 0) {
    *p = 0;

    // found one file, append it to listbox
    win->insertItem(iBuffer);
    if(!isResultReported) {
      emit haveResults(true);
      isResultReported = true;
    }
    memmove(iBuffer, p+1, strlen(p + 1)+1);
  }
  
  // Update count
  QString str = i18n("%1 file(s) found").arg(win->childCount());
  emit statusChanged(str.ascii());
}

void Kfind::setExpanded(bool expand) {

  // Changing size "hides" the file table (win) when we do not need it.
  // If we really win->show()/hide() it, tabWidget is not stretched 
  // when win is hided. (Bug in QT?)
  
  if(expand) {
    setMinimumSize(tabWidget->sizeHint().width(), 
		   2*tabWidget->sizeHint().height());
    setMaximumHeight(5000);
  }
  else {
    setMinimumSize(tabWidget->sizeHint());
    setMaximumHeight(tabWidget->sizeHint().height());
  }
  
  emit enableStatusBar(expand);
}

void Kfind::copySelection() {
  win->copySelection();
}

