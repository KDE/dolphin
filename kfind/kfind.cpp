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

#include "kfmenu.h"
#include "kftabdlg.h"
#include "kfwin.h"
#include "kfoptions.h"
#include "kfind.h"

#include "version.h"

Kfind::Kfind( QWidget *parent, const char *name, const char *searchPath = 0 )
    : QWidget( parent, name )
  {
    setMinimumSize(440,220);             //the window never should be smaller
    setMaximumSize(9999,220);

    setCaption(QString("KFind ")+KFIND_VERSION);

    //initialize menu
    menu = new KfindMenu(this,"menu");
    menu->setGeometry(0,0,width(),menu->height());

    connect(this,SIGNAL(haveResults(bool)),
            menu,SLOT(enableSaveResults(bool)));
    connect(menu,SIGNAL(about()),
            this,SLOT(aboutFind()));

    //create tree bottons      
    bt[0] = new QPushButton("F&ind Now"  ,this,"find");
    bt[1] = new QPushButton("Sto&p"      ,this,"stop");
    bt[2] = new QPushButton("Ne&w Search",this,"new_search");

    bt[0]->setFixedSize(bt[2]->sizeHint());
    bt[1]->setFixedSize(bt[2]->sizeHint());
    bt[1]->setEnabled(false);
    bt[2]->setFixedSize(bt[2]->sizeHint());

    //connect button to slots
    connect( bt[0],  SIGNAL(clicked()),
             this, SLOT(startSearch()) );
    connect( bt[1],  SIGNAL(clicked()),
             this, SLOT(stopSearch()) );
    connect( bt[2],  SIGNAL(clicked()),
             this, SLOT(newSearch()) );

    //create tabdialog
    tabDialog = new KfindTabDialog(this,"dialog",searchPath);

    //prepare window for find results
    win = new KfindWindow(this,"window");
    win->hide();  //and hide it firstly    

    connect(win ,SIGNAL(resultSelected(const char *)),
            this,SLOT(resultSelected(const char *)));
    connect(this,SIGNAL(resultSelected(bool)),
            menu,SLOT(enableMenuItems(bool)));
    connect(menu,SIGNAL(deleteFile()),
            win,SLOT(deleteFiles()));
    connect(menu,SIGNAL(properties()),
            win,SLOT(fileProperties()));
    connect(menu,SIGNAL(openFolder()),
            win,SLOT(openFolder()));
    connect(menu,SIGNAL(saveResults()),
            win,SLOT(saveResults()));
    connect(menu,SIGNAL(addToArchive()),
            win,SLOT(addToArchive()));
    connect(menu,SIGNAL(open()),
            win,SLOT(openBinding()));

    connect(menu,SIGNAL(prefs()),
            this,SLOT(prefs()));

    //    emit haveResults(FALSE);
    resize(440,220);
  };

void Kfind::resizeEvent( QResizeEvent * )
  {
    menu->setGeometry(0,0,width(),menu->height());
    tabDialog->setGeometry(10,menu->height()+5,
                           width()-25-bt[0]->width(),210-menu->height());
    bt[0]->move(20+tabDialog->width(),tabDialog->y()+20);
    bt[1]->move(20+tabDialog->width(),bt[0]->y()+5+bt[0]->height());
    bt[2]->move(20+tabDialog->width(),bt[1]->y()+5+bt[1]->height());
    win->setGeometry(0,220,width(),height()-win->y());
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
            if (height()==220)
	      resize(width(),height()+150);
	  };

        unlink( outFile.data() );
    
        bt[1]->setEnabled(false);
        bt[0]->setEnabled(true);
      }
   };
    
void Kfind::startSearch()
  {
    QString buffer,pom;
    char **findargs;
    int args_number;
    int fromPos,toPos,i;

    //    printf("Starting Search\n");
    setMaximumSize(9999,9999);

    buffer = tabDialog->createQuery();

    emit haveResults(false);
    emit resultSelected(false);

    if (!buffer.isNull())
      {
        bt[0]->setEnabled(false);
        bt[1]->setEnabled(true);

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
    
    bt[1]->setEnabled(false);
    bt[0]->setEnabled(true);

    kill(childPID,9);
  };

void Kfind::newSearch()
  {
    //    printf("Prepare for New Search\n");
    win->hide();
    win->clearList();
    resize(width(),220);
    setMaximumSize(9999,220);

    doProcess=false;
    emit haveResults(false);
    emit resultSelected(false);
     
    stopSearch();
 };

void Kfind::resultSelected(const char * str)
  {
    emit resultSelected(true);
  };

void Kfind::aboutFind()
  {
    QString tmp;

    tmp.sprintf("KFind %s\nFrontend to find utility
                     \nMiroslav Flídr <flidr@kky.zcu.cz>",KFIND_VERSION);

    KMsgBox::message(this,"about box",tmp,KMsgBox::INFORMATION, "OK"); 
  };

void Kfind::message(const char *str)
  {
    KMsgBox::message(this, "about box",str,KMsgBox::INFORMATION, "Oops"); 
  };   

void Kfind::prefs()
  {
    //    KfOptions *prefs = new KfOptions(0L,0L);
    KfOptions *prefs = new KfOptions(0L,0L);

    prefs->show();
  };
