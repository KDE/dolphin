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

#include <kmsgbox.h>

#include "kftabdlg.h"
#include "kfwin.h"
#include "kfind.h"

Kfind::Kfind( QWidget *parent, const char *name, const char *searchPath = 0 )
    : QWidget( parent, name )
  {
    //create tabdialog
    tabDialog = new KfindTabDialog(this,"dialog",searchPath);

    //prepare window for find results
    win = new KfindWindow(this,"window");
    win->hide();  //and hide it firstly    

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

    emit haveResults(false);
    resize(tabDialog->sizeHint()+QSize(0,5));
  };

void Kfind::resizeEvent( QResizeEvent *e)
  {
    QWidget::resizeEvent(e);

    tabDialog->setGeometry(5,5,
			   width()-10,(tabDialog->sizeHint()).height());
    //printf("------------------------------\n");
    //printf("Win height1 = %d\n",win->height());
    //printf("Kfind height1 = %d\n",height());
    //printf("tabDialog height1 = %d\n",(tabDialog->sizeHint()).height());
    win->setGeometry(0,5+(tabDialog->sizeHint()).height()+5,width(),
    		     height()-tabDialog->height()-10);
    //printf("Win height2 = %d\n",win->height());
    //printf("------------------------------\n");
  };

void Kfind::timerEvent( QTimerEvent * )
  {
    int status;

    if (childPID==waitpid(childPID,&status,WNOHANG))
      {
        killTimer( timerID );

	if (doProcess)
          {
            win->updateResults( outFile.data() );
            win->show();

            emit haveResults(true);
	    emit enableStatusBar(true);
 	  };

        unlink( outFile.data() );
    
	enableSearchButton(true);
      };
   };
    
void Kfind::startSearch()
  {
    QString buffer,pom;
    char **findargs;
    int args_number;
    int fromPos,toPos,i;

    //    printf("Starting Search\n");

    buffer = tabDialog->createQuery();

    emit haveResults(false);
    emit resultSelected(false);
    win->clearList();

    if (!buffer.isNull())
      {
	enableSearchButton(false);

        int t = time( 0L ); 
        outFile.sprintf( "/tmp/kfindout%i", t );

	buffer.append(pom.sprintf(" -fprint %s",outFile.data()));
        buffer=buffer.simplifyWhiteSpace();

        args_number=buffer.contains(" ")+3;
        findargs= (char **) malloc(args_number*sizeof(char *));

        findargs[0]=(char *) malloc(5*sizeof(char));
        findargs[0]="find";
        i=0;
        fromPos=0;
        while(i<buffer.contains(" ")+1)
          {
            toPos=buffer.find(" ",fromPos);
            toPos=(toPos!=-1)?toPos:buffer.length();	    
            pom=buffer.mid(fromPos,toPos-fromPos);
            i++;
            findargs[i]=(char *) malloc((pom.length()+1)*sizeof(char));
            strcpy(findargs[i],(char*)pom.data());
            fromPos=toPos+1;
          };
        findargs[buffer.contains(" ")+2]=0L;

        childPID=fork();
        if (childPID==0)
          {
	    //            printf("Hey I'm child process\n");
	    //            printf("CMD find %s\n",buffer.data());

	    execvp("find",findargs);
            printf("Error by creating child process!\n");
   	    exit(1); 
          }; 

        doProcess=true;
        timerID = startTimer( 1000 );
      };
  };

void Kfind::stopSearch()
  {
    //    printf("Stoping Search\n");
    
    enableSearchButton(true);

    kill(childPID,9);
  };

void Kfind::newSearch()
  {
    //    printf("Prepare for New Search\n");
    win->hide();
    win->clearList();

    tabDialog->setDefaults();

    emit enableStatusBar(false);

    doProcess=false;
    emit haveResults(false);
    emit resultSelected(false);
     
    stopSearch();
 };

QSize Kfind::sizeHint()
  {
    return (tabDialog->sizeHint());
  };

