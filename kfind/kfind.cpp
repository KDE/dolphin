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

Kfind::Kfind( QWidget *parent, const char *name, const char *searchPath )
    : QWidget( parent, name )
  {
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
    

    emit haveResults(false);
    resize(tabDialog->sizeHint());
  };

void Kfind::copySelection() {
  win->copySelection();
}

void Kfind::keyPressEvent(QKeyEvent *e) {
  if(e->key() == Key_Return || e->key() == Key_Enter) {
    e->accept();
    startSearch();
  }

  QWidget::keyPressEvent(e);
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
    QString buffer,pom;
    //int pos;
    buffer = tabDialog->createQuery();

    if ( winsize==1)
      winsize=200;
    emit haveResults(false);
    emit resultSelected(false);
    win->clearList();

    if (!buffer.isNull())
      {
	enableSearchButton(false);

	findProcess.clearArguments ();
	QString cmdline = buffer;
	findProcess.setExecutable(cmdline);

        int t = time( 0L ); 
        outFile.sprintf( "/tmp/kfindout%i", t );
	
	findProcess.start(KProcess::NotifyOnExit, KProcess::AllOutput);
      };
  };

void Kfind::stopSearch()
  {
    //    printf("Stoping Search\n");
    
    enableSearchButton(true);

    findProcess.kill();
  };

void Kfind::newSearch()
  {
    //    printf("Prepare for New Search\n");
    win->hide();
    win->clearList();
    winsize=1;

    tabDialog->setDefaults();

    emit enableStatusBar(false);
    emit haveResults(false);
    emit resultSelected(false);
     
    stopSearch();
 };

void Kfind::handleStdout(KProcess *proc, char *buffer, int buflen)
{
  (void)proc;
	char tmp = buffer[buflen] ;
	buffer[buflen] = '\0' ;
	
	FILE *f = fopen(outFile.data(),"a");
	fwrite(buffer, strlen(buffer), 1, f);
	fclose(f);
	buffer[buflen] = tmp ;
}

void Kfind::processResults()
  {
    win->updateResults( outFile.data() );
    win->show();
    
    emit haveResults(true);
    emit enableStatusBar(true);

    unlink( outFile.data() );
    
    enableSearchButton(true);
  };

QSize Kfind::sizeHint()
  {
    return (tabDialog->sizeHint());//+QSize(0,winsize-1));
  };
