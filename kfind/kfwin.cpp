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
#include <qmsgbox.h>

#include <kfm.h>
#include <kfmclient_ipc.h>
#include <kprocess.h>

#include "kfwin.h"
#include "kfarch.h"
#include "kfsave.h"

#include <klocale.h>
#include <kapp.h>

extern KfSaveOptions *saving; 
extern QList<KfArchiver> *archivers;

KfindWindow::KfindWindow( QWidget *parent, const char *name )
    : QWidget( parent, name )          
  {
    lbx = new QListBox(this,"list_box" );

    connect(lbx , SIGNAL(highlighted(int)),
            this, SLOT(highlighted()) );
    connect(lbx , SIGNAL(selected(int)),
            this, SLOT( openBinding()) );
   };

KfindWindow::~KfindWindow()
  {
    delete lbx;
  };

void KfindWindow::resizeEvent( QResizeEvent * )
  {
    lbx->setGeometry(0,0,width(),height());    
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
    sprintf(str,klocale->translate("%d file(s) found"),count);
    emit statusChanged(str);

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
      QMessageBox::warning(parentWidget(),klocale->translate("Error"),
		     klocale->translate("It wasn't possible to save results!"),
		     klocale->translate("OK"));
    else
      {
	if ( strcmp(saving->getSaveFormat(),"HTML")==0)
	  {
	    fprintf(results,"<HTML><HEAD>\n");
	    fprintf(results,"<!DOCTYPE %s>\n",
		    klocale->translate("KFind Results File"));
	    fprintf(results,"<TITLE>%sKFind Results File</TITLE></HEAD>\n",
		    klocale->translate("KFind Results File"));
	    fprintf(results,"<BODY><H1>%s</H1>\n",
		    klocale->translate("KFind Results File"));
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
 	QMessageBox::information(parentWidget(),
			 klocale->translate("Information"),
 			 klocale->translate("Results were saved to file\n")+
			 filename,
 			 klocale->translate("OK"));
      };
  };

void KfindWindow::highlighted()
  {
    emit resultSelected(true);
  };

void KfindWindow::deleteFiles()
  {
    QString tmp;

    tmp.sprintf(klocale->translate("Do you really want to delete file:\n%s"),
                lbx->text(lbx->currentItem()));
    if(QMessageBox::warning(parentWidget(),klocale->translate("Delete File"),
                      tmp,klocale->translate("OK"),
		      klocale->translate("Cancel")) == 0)
      {
        QFileInfo *file = new QFileInfo(lbx->text(lbx->currentItem()));
	if (file->isFile()||file->isSymLink())
            {
              if (remove(file->filePath())==-1)
                  switch(errno)
                    {
    	              case EACCES: 
			QMessageBox::warning(parentWidget(),
					   klocale->translate("Error"),
					   klocale->translate("You have no permission\n to delete this file"),
					   klocale->translate("OK"));
                                   break;
                      default: 
			QMessageBox::warning(parentWidget(),
					     klocale->translate("Error"),
					     klocale->translate("It isn't possible to delete\nselected file"),
					     klocale->translate("OK"));
                    }
                else
                  {
                   KFM *kfm= new KFM();
                   kfm->refreshDirectory(lbx->text(lbx->currentItem()));
                   lbx->removeItem(lbx->currentItem());
		   delete kfm;
 		  };
            }
          else
            {
              if (rmdir(file->filePath())==-1)
		  switch(errno)
                    {
		    case EACCES: QMessageBox::warning(parentWidget(),
				              klocale->translate("Error"),
		                              klocale->translate("You have no permission\n to delete this directory"),
					      klocale->translate("OK"));
                                  break;
      	             case ENOTEMPTY: QMessageBox::warning(parentWidget(),
					    klocale->translate("Error"),
					    klocale->translate("Specified directory\nis not empty!"),
					    klocale->translate("OK"));
                                     break;
     	             default: QMessageBox::warning(parentWidget(),
					 klocale->translate("Error"),
                                         klocale->translate("It isn't possible to delete\nselected directory"),
                                         klocale->translate("OK"));
                   }
                 else
                  {
		    KFM *kfm= new KFM();
		    kfm->refreshDirectory(lbx->text(lbx->currentItem()));
		    lbx->removeItem(lbx->currentItem());
		    delete kfm;
                  };
            };

	  delete file;
      };
  };

void KfindWindow::fileProperties()
  {
    QString tmp= "file:";
    KFM *kfm= new KFM();

    QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
    if (fileInfo->isDir())
      {
	tmp.append(fileInfo->filePath());
	kfm->openProperties(tmp.data());
      }
    else
      {
	tmp.append(lbx->text(lbx->currentItem()));
	kfm->openProperties(tmp.data());
      };
    delete kfm;
  };

void KfindWindow::openFolder()
  {
    QString tmp;
    KFM *kfm= new KFM();

    QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
    if (fileInfo->isDir())
        tmp.sprintf("file:%s",fileInfo->filePath());
      else
	tmp.sprintf("file:%s",(fileInfo->dirPath()).data());


    kfm->openURL(tmp.data());
    delete kfm;
  };

void KfindWindow::openBinding()
  {
    QString tmp= "file:";
    KFM *kfm= new KFM();

    QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
    if (fileInfo->isDir())
      {
	tmp.append(fileInfo->filePath());
	kfm->openURL(tmp.data());
      }
    else
      {
	tmp.append(lbx->text(lbx->currentItem()));
	kfm->exec(tmp.data(),0L);
      };
    delete kfm;
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

  if ( (arch = KfArchiver::findByPattern(pattern2))!=0L)
    execAddToArchive(arch,filename);
  else
    if ( (arch = KfArchiver::findByPattern("*"+pattern1))!=0L)
      execAddToArchive(arch,filename);
    else
      QMessageBox::warning(parentWidget(),klocale->translate("Error"),
		       klocale->translate("Couldn't recognize archive type!"),
		       klocale->translate("OK")); 

};

void KfindWindow::execAddToArchive(KfArchiver *arch,QString archname)
{
  QFileInfo archiv(archname);
  QString buffer,pom;
  KProcess archProcess;
  int pos;
      
  if ( archiv.exists() )
    buffer = arch->getOnUpdate();
  else
    buffer = arch->getOnCreate();

  buffer=buffer.simplifyWhiteSpace();

  pos = buffer.find(" ");
  pom = buffer.left(pos);
  if (pos==-1) 
    pos = buffer.length();
  buffer = buffer.remove(0,pos+1);

  archProcess.setExecutable(pom.data());
  archProcess.clearArguments ();

  while( !buffer.isEmpty() )
    {
      pos = buffer.find(" ");
      pom = buffer.left(pos);

      if ( pom=="%d" )
	{
	  QFileInfo *fileInfo = new QFileInfo(lbx->text(lbx->currentItem()));
	  pom.sprintf("%s/",(fileInfo->dirPath(TRUE)).data());
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

      archProcess << pom.data();

      if (pos==-1) 
	pos = buffer.length();
      buffer = buffer.remove(0,pos+1);
    };

  if ( !archProcess.start(KProcess::DontCare) )
    warning(klocale->translate("Error while creating child process!"));
};


