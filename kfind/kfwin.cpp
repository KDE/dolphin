/***********************************************************************
 *
 *  Kfwin.cpp
 *
 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>  
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>     
#include <time.h>    

#include <qapp.h>
#include <qwidget.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qstrlist.h>
#include <qfileinf.h> 
#include <qpushbt.h> 
#include <qlined.h> 
#include <qgrpbox.h> 
#include <qchkbox.h>
#include <qfiledef.h>
#include <qfiledlg.h> 
#include <qlist.h>
#include <qfileinf.h> 

#include <kfm.h>
#include <kmsgbox.h>
#include "kfwin.h"
//#include "kftypes.h"
#include "kfarch.h"
#include "kfsave.h"

extern KfSaveOptions *saving; 
extern QList<KfArchiver> *archivers;
//extern QList<KfFileType> *types;

KfindWindow::KfindWindow( QWidget *parent, const char *name )
    : QWidget( parent, name )          
  {
    lbx = new QListBox(this,"list_box" );
    lbx->setGeometry(0,0,width(),height()-27);
    
    label = new QLabel(this,"label");
    label->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    label->setLineWidth( 2 );
    label->setGeometry(0,lbx->height()+2,width(),25);

    connect(lbx , SIGNAL(highlighted(int)),
            this, SLOT(highlighted(int)) );
    connect(lbx , SIGNAL(selected(int)),
            this, SLOT( openBinding()) );
   
   };

void KfindWindow::resizeEvent( QResizeEvent * )
  {
    lbx  ->setGeometry(0,0,width(),height()-27);
    label->setGeometry(0,lbx->height()+2,width(),25);
  };

void KfindWindow::updateResults(const char *file )
  {
    char str[PATH_MAX];
    int count;
    
    QStrList *strl= new QStrList (TRUE);
    FILE *f = fopen(file,"rb");
    if (f==0)
      return;
    
    lbx->clear();
    
    count=0;
    while ( !feof( f ) )
      {
        str[0] = 0;
        fgets( str, 1023, f );
        if ( str[0] != 0 )
          {
            // Delete trailing '\n'
            str[ strlen( str ) - 1 ] = 0;
 	    strl->append (str);
            //insertItem( str );
            count++;
          }
      };

    QFileInfo *filename = new QFileInfo(strl->last());
    if (!(filename->exists()))
      strl->removeLast();

    lbx->insertStrList(strl,-1);
    sprintf(str,"%d file(s) found",count);
    label->setText(str);
    fclose(f);    
    delete filename;
    delete strl;
  };

void KfindWindow::clearList()
  { 
    lbx->clear();
  };

void KfindWindow::changeItem(const char *itemName)
  {
    lbx->changeItem(itemName,lbx->currentItem());    
  };

void KfindWindow::saveResults()
  { 
    uint items,item;
    FILE *results;
    QString filename;

    if ( saving->getSaveStandard() )
      {
	filename = getenv("HOME");
	filename += "/.kfind-results.html";
      }
    else
      filename = saving->getSaveFile();

    results=fopen(filename,"w");

    items=lbx->count();
    if (results==0L)
      KMsgBox::yesNo(parentWidget(),"Error",
		     "It wasn't possible to save results!",
		     KMsgBox::EXCLAMATION,
		     "OK","Cancel");
    else
      {
	if ( strcmp(saving->getSaveFormat(),"HTML")==0)
	  {
	    fprintf(results,"<HTML><HEAD>\n");
	    fprintf(results,"<!DOCTYPE KFIND-Result-file>\n");
	    fprintf(results,"<TITLE>KFind Results File</TITLE></HEAD>\n");
	    fprintf(results,"<BODY><H1>KFind Results File</H1>\n");
	    fprintf(results,"<DL><p>\n"); 
	
	    item=0;  
	    while(item!=items)
	      {
		fprintf(results,"<DT><A HREF=\"file:%s\">file:%s</A>\n",
			lbx->text(item),lbx->text(item));
		item++;
	      };
	    fprintf(results,"</DL><P></BODY></HTML>\n");
	  }
	else
	  {
	    item=0;  
	    while(item!=items)
	      {
		fprintf(results,"%s\n", lbx->text(item));
		item++;
	      };
	    
	  };
	fclose(results); 
	KMsgBox::message(parentWidget(),"Error",
			 "Results were saved to file\n"+filename,
			 KMsgBox::INFORMATION,"OK");
      };
  };

void KfindWindow::highlighted(int index)
  {
    emit resultSelected(lbx->text(index));
  };

void KfindWindow::deleteFiles()
  {
    QString tmp;

    tmp.sprintf("Do you really want to delete file:\n%s",
                lbx->text(lbx->currentItem()));
    if(KMsgBox::yesNo(parentWidget(),"Delete File",
                      tmp,KMsgBox::QUESTION, "OK","Cancel") == 1)
      {
	//        printf("So we'll delete it!\n");
        
        QFileInfo *file = new QFileInfo(lbx->text(lbx->currentItem()));
	if (file->isFile()||file->isSymLink())
            {
              if (remove(file->filePath())==-1)
                  switch(errno)
                    {
    	              case EACCES: KMsgBox::yesNo(parentWidget(),"Error",
                                               "You have no permission
                                                \n to delete this file",
                                                KMsgBox::EXCLAMATION,
                                                "OK","Cancel");
                                   break;
                      default: KMsgBox::yesNo(parentWidget(),"Error",
                                            "It isn't possible to delete
                                             \nselected file",
                                             KMsgBox::EXCLAMATION,
                                             "OK","Cancel");
                    }
                else
                  {
                   KFM *PropertiesD= new KFM();
                  /* QFileInfo *fileInfo = new QFileInfo(lbx->text(
                                                    lbx->currentItem()));
		  */ 
                   PropertiesD->refreshDirectory(lbx->text(lbx->currentItem()));
                   lbx->removeItem(lbx->currentItem());
 		  };
            }
          else
            {
              if (rmdir(file->filePath())==-1)
		  switch(errno)
                    {
	             case EACCES: KMsgBox::yesNo(parentWidget(),"Error",
                                           "You have no permission
                                            \n to delete this directory",
                                            KMsgBox::EXCLAMATION,
                                            "OK","Cancel");
                                  break;
      	             case ENOTEMPTY: KMsgBox::yesNo(parentWidget(),"Error",
                                           "Specified directory 
                                            \nis not empty!",
                                            KMsgBox::EXCLAMATION,
                                            "OK","Cancel");
                                     break;
     	             default: KMsgBox::yesNo(parentWidget(),"Error",
                                        "It isn't possible to delete
                                         \nselected directory",
                                         KMsgBox::EXCLAMATION,
                                         "OK","Cancel");
                   }
                 else
                  {
		    KFM *PropertiesD= new KFM();
		    /* QFileInfo *fileInfo = new QFileInfo(lbx->text(
					      lbx->currentItem()));
		  */ 
		    PropertiesD->refreshDirectory(lbx->text(lbx->currentItem()));
		    lbx->removeItem(lbx->currentItem());
                  };
            };

	  delete file;
      };
  };

void KfindWindow::fileProperties()
  {
    KFM *PropertiesD= new KFM();

    // QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));

    PropertiesD->openProperties(lbx->text(lbx->currentItem()));
  };

void KfindWindow::openFolder()
  {
    QString tmp;
    KFM *PropertiesD= new KFM();

    QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
    if (fileInfo->isDir())
        tmp.sprintf("file:%s",fileInfo->filePath());
      else
        tmp.sprintf("file:%s",(fileInfo->dirPath()).data());


    PropertiesD->openURL(tmp.data());
    
//     int childPID=fork();
//     if (childPID==0)
//       {
// 	execlp("kfmclient","kfmclient","openURL",tmp.data(),0L);
//         printf("Error by creating child process!\n");
//         exit(1); 
//       };
    
  };

void KfindWindow::openBinding()
  {
    QString tmp;
    KFM *PropertiesD= new KFM();

    QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
    if (fileInfo->isDir())
      {
	tmp.sprintf("file:%s",fileInfo->filePath());
	PropertiesD->openURL(tmp);
      }
    else
      PropertiesD->exec(lbx->text(lbx->currentItem()),0L);
  };

void KfindWindow::addToArchive()
{
  QString path(getenv("HOME"));
  KfArchiver *arch;

  QString filename( QFileDialog::getOpenFileName(path) );
  if ( filename.isNull() )
    return;

  int pos1 = filename.findRev(".");
  int pos2 = filename.findRev(".",pos1-1);
  
  QString pattern1 = filename.right(filename.length()-pos1);
  QString pattern2 = "*"+filename.mid(pos2,pos1-pos2)+pattern1;

  //printf("filename1 = %s\n",pattern1.data());
  //printf("filename2 = %s\n",pattern2.data());

  if ( (arch = KfArchiver::findByPattern(pattern2))!=0L)
    execAddToArchive(arch,filename);
  else
    if ( (arch = KfArchiver::findByPattern("*"+pattern1))!=0L)
      execAddToArchive(arch,filename);
    else
      KMsgBox::message(parentWidget(),"Error","Could not recognize archive type!"
		       ,KMsgBox::STOP, "OK"); 

};

void KfindWindow::execAddToArchive(KfArchiver *arch,QString archname)
{
  QFileInfo archiv(archname);
  QString buffer,pom;
  char **arch_args;
  int args_number;
  int pos,i;
      
  if ( archiv.exists() )
    buffer = arch->getOnUpdate();
  else
    buffer = arch->getOnCreate();

  buffer=buffer.simplifyWhiteSpace();

  args_number=buffer.contains(" ")+1;
  //printf("args number = %d\n",args_number);

  arch_args= (char **) malloc(args_number*sizeof(char *));

  i=0;
  //  i<=args_number
  while( !buffer.isEmpty() )
    {
      pos = buffer.find(" ");
      pom = buffer.left(pos);

      if ( pom=="%d" )
	{
	  QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
	  pom.sprintf("%s/",(fileInfo->dirPath(TRUE)).data());

	  printf("Dir = %s\n",pom.data());
	};

      if ( pom=="%a" )
	pom = archname;
      
      if ( pom=="%f" )
	pom = lbx->text(lbx->currentItem());

      if ( pom=="%n" )
	{
	  QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
	  pom = fileInfo->fileName();
	};

      arch_args[i]=(char *) malloc(pom.length()*sizeof(char));
      strcpy(arch_args[i],pom.data());	  

      if (pos==-1) 
	pos = buffer.length();
      buffer = buffer.remove(0,pos+1);
      i++;
    };

  //  for(i=0;i<args_number;i++)
  //    printf("Arg[%d] = %s\n",i,arch_args[i]);
  if (fork()==0)
    {
      execvp(arch_args[0],arch_args);
      printf("Error by creating child process!\n");
      exit(1); 
    }; 

  for(i=0;i<args_number;i++)
    free(arch_args[i]);
  free(arch_args);
};


