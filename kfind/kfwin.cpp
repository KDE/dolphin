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

#include <qapplication.h>
#include <qwidget.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qstrlist.h>
#include <qfileinfo.h> 
#include <qpushbutton.h> 
#include <qlineedit.h> 
#include <qgroupbox.h> 
#include <qcheckbox.h>
#include <qfiledefs.h>
#include <qfiledialog.h> 
#include <qlist.h>
#include <qfileinfo.h> 
#include <qmessagebox.h>
#include <qdir.h>
#include <qclipboard.h>
#include <qevent.h>
#include <kfiledialog.h>

#include <kdebug.h>
#include <krun.h>
#include <kprocess.h>
#include <kpropsdlg.h>
#include <krun.h>

#include "kfwin.h"
#include "kfarch.h"
#include "kfsave.h"

#include <klocale.h>
#include <kapp.h>

#include <qkeycode.h>

extern KfSaveOptions *saving; 
extern QList<KfArchiver> *archivers;

KfindWindow::KfindWindow( QWidget *parent, const char *name )
    : QListBox( parent, name )          
  {
    //    topLevelWidget()->installEventFilter(lbx);
    setMultiSelection(FALSE);

    connect(this , SIGNAL(highlighted(int)),
            this, SLOT(highlighted()) );
    connect(this, SIGNAL(selected(int)),
            this, SLOT( openBinding()) );
   };

KfindWindow::~KfindWindow()
  {
  };

void KfindWindow::appendResult(const char *file) {
  insertItem(file);
}

void KfindWindow::beginSearch() {
  killTimers();
  startTimer(100);
}

void KfindWindow::doneSearch() {
  killTimers();
  
  QString str = i18n("%1 file(s) found").arg(count());
  emit statusChanged(str.ascii());
}

void KfindWindow::timerEvent(QTimerEvent *) {
  if(count() > 0) {
    QString str = i18n("%1 file(s) found").arg(count());
    emit statusChanged(str.ascii());
  }
}

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

void KfindWindow::updateResults(const char *file )
  {
    kdebug(KDEBUG_INFO, 1902, "UPDATERESULTs\n");
    
    char str[PATH_MAX];
    int count;
    
    QStrList *strl= new QStrList (TRUE);
    FILE *f = fopen(file,"rb");
    if (f==0)
      {
	QString statusmsg =i18n("%1 file(s) found").arg(0);
	emit statusChanged(statusmsg.ascii());
	return;
      };
    
    clear();
    
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

    insertStrList(strl,-1);
    QString statusmsg = i18n("%1 file(s) found").arg(count);
    emit statusChanged(statusmsg.ascii());

	unlink( file );
    fclose(f);    
    delete filename;
    delete strl;
  };

// copy to clipboard aka X11 selection
void KfindWindow::copySelection() {
  QString s;
  for(int i = 0; i < (int)count(); i++)
    if(isSelected(i)) {
      s.append(text(i));
      s.append(" ");
    }

  if(s.length() > 0) {
    QClipboard *cb = kapp->clipboard();
    cb->clear();
    cb->setText(s);
  }
}

void KfindWindow::clearList()
  { 
    clear();
  };

void KfindWindow::changeItem(const char *itemName)
  {
    debug("CXHANGE ITEM CALLED\n");
    //    changeItem(itemName,currentItem());    
  };

void KfindWindow::selectAll() {
  setAutoUpdate(FALSE);
  if(currentItem() == -1)
    setCurrentItem(0);
  for(int i = 0; i < (int)count(); i++)
    setSelected(i, TRUE);
  setAutoUpdate(TRUE);
  repaint();
}

void KfindWindow::invertSelection() {
  setAutoUpdate(FALSE);
  if(currentItem() == -1)
    setCurrentItem(0);
  for(int i = 0; i < (int)count(); i++)
    setSelected(i, !isSelected(i));
  setAutoUpdate(TRUE);
  repaint();
}

void KfindWindow::unselectAll() {
  setAutoUpdate(FALSE);
  if(currentItem() == -1)
    setCurrentItem(0);
  for(int i = 0; i < (int)count(); i++)
    setSelected(i, FALSE);
  setAutoUpdate(TRUE);
  repaint();
}

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

    results=fopen(filename.ascii(),"w");

    items=count();
    if (results==0L)
      QMessageBox::warning(parentWidget(),i18n("Error"),
		     i18n("It wasn't possible to save results!"),
		     i18n("OK"));
    else
      {
	if ( saving->getSaveFormat() == "HTML" )
	  {
	    fprintf(results,"<HTML><HEAD>\n");
	    fprintf(results,"<!DOCTYPE %s>\n",
		    i18n("KFind Results File").ascii());
	    fprintf(results,"<TITLE>%sKFind Results File</TITLE></HEAD>\n",
		    i18n("KFind Results File").ascii());
	    fprintf(results,"<BODY><H1>%s</H1>\n",
		    i18n("KFind Results File").ascii());
	    fprintf(results,"<DL><p>\n"); 
	
	    item=0;  
	    while(item!=items)
	      {
		fprintf(results,"<DT><A HREF=\"file:%s\">file:%s</A>\n",
			text(item).ascii(),text(item).ascii());
		item++;
	      };
	    fprintf(results,"</DL><P></BODY></HTML>\n");
	  }
	else
	  {
	    item=0;  
	    while(item!=items)
	      {
		fprintf(results,"%s\n", text(item).ascii());
		item++;
	      };
	    
	  };
	fclose(results); 
 	QMessageBox::information(parentWidget(),
			 i18n("Information"),
 			 i18n("Results were saved to file\n")+
			 filename,
 			 i18n("OK"));
      };
  };

void KfindWindow::highlighted()
  {
    emit resultSelected(true);
  };

void KfindWindow::deleteFiles()
  {
    QString tmp = i18n("Do you really want to delete file:\n%1")
                .arg(text(currentItem()));
    if (!QMessageBox::information(parentWidget(),
				  i18n("Delete File - Find Files"),
				  tmp, i18n("&Yes"), i18n("&No"), 0, 
				  1))
      {
        QFileInfo *file = new QFileInfo(text(currentItem()));
	if (file->isFile()||file->isSymLink())
            {
              if (remove(file->filePath().ascii())==-1)
                  switch(errno)
                    {
    	              case EACCES: 
			QMessageBox::warning(parentWidget(),
					   i18n("Error - Find Files"),
					   i18n("You have no permission\n to delete this file"),
					   i18n("&Ok"));
                                   break;
                      default: 
			QMessageBox::warning(parentWidget(),
					     i18n("Error - Find Files"),
					     i18n("It isn't possible to delete\nselected file"),
					     i18n("&Ok"));
                    }
                else
                  {
                   // KFM *kfm= new KFM();
#warning "TODO : implement a replacement for kfm->refreshDirectory(url)"
                   // and replace the one below as well
                   // kfm->refreshDirectory(text(currentItem()));
                   removeItem(currentItem());
		   // delete kfm;
 		  };
            }
          else
            {
              if (rmdir(file->filePath().ascii())==-1)
		  switch(errno)
                    {
		    case EACCES: QMessageBox::warning(parentWidget(),
				              i18n("Error"),
		                              i18n("You have no permission\n to delete this directory"),
					      i18n("OK"));
                                  break;
      	             case ENOTEMPTY: QMessageBox::warning(parentWidget(),
					    i18n("Error"),
					    i18n("Specified directory\nis not empty!"),
					    i18n("OK"));
                                     break;
     	             default: QMessageBox::warning(parentWidget(),
					 i18n("Error"),
                                         i18n("It isn't possible to delete\nselected directory"),
                                         i18n("OK"));
                   }
                 else
                  {
		    // KFM *kfm= new KFM();
		    // kfm->refreshDirectory(text(currentItem()));
		    removeItem(currentItem());
		    // delete kfm;
                  };
            };

	  delete file;
      };
  };

void KfindWindow::fileProperties()
  {
    QString tmp= "file:";

    QFileInfo *fileInfo = new QFileInfo(text(currentItem()));
    if (fileInfo->isDir())
	tmp.append(fileInfo->filePath());
    else
	tmp.append(text(currentItem()));
    (void) new PropertiesDialog(tmp);
  };

void KfindWindow::openFolder()
  {
    QString tmp;

    QFileInfo *fileInfo = new QFileInfo(text(currentItem()));
    if (fileInfo->isDir())
        tmp = "file:" + fileInfo->filePath();
      else
        tmp = "file:" + fileInfo->dirPath();

    (void) new KRun(tmp);
  };

void KfindWindow::openBinding()
  {
    QString tmp= "file:";

    QFileInfo *fileInfo = new QFileInfo(text(currentItem()));
    if (fileInfo->isDir())
	tmp.append(fileInfo->filePath());
    else
	tmp.append(text(currentItem()));
    (void) new KRun( tmp );
  };

void KfindWindow::addToArchive()
{
  QString path = QDir::home().absPath();
  KfArchiver *arch;

  QString filename( KFileDialog::getOpenFileName(path) );
  if ( filename.isNull() )
    return;

  int pos1 = filename.findRev(".");
  int pos2 = filename.findRev(".",pos1-1);
  
  QString pattern1 = filename.right(filename.length()-pos1);
  QString pattern2 = "*"+filename.mid(pos2,pos1-pos2)+pattern1;

  if ( (arch = KfArchiver::findByPattern(pattern2.ascii()))!=0L)
    execAddToArchive(arch,filename);
  else
    if ( (arch = KfArchiver::findByPattern(("*"+pattern1).ascii()))!=0L)
      execAddToArchive(arch,filename);
    else
      QMessageBox::warning(parentWidget(),i18n("Error"),
		       i18n("Couldn't recognize archive type!"),
		       i18n("OK")); 

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

  archProcess.clearArguments ();
  archProcess.setExecutable(pom);

  while( !buffer.isEmpty() )
    {
      pos = buffer.find(" ");
      pom = buffer.left(pos);

      if ( pom=="%d" )
	{
	  QFileInfo *fileInfo = new QFileInfo(text(currentItem()));
	  pom = fileInfo->dirPath(TRUE)+"/";
	};

      if ( pom=="%a" )
	pom = archname;
      
      if ( pom=="%f" )
	pom = text(currentItem());

      if ( pom=="%n" )
	{
	  QFileInfo *fileInfo = new QFileInfo(text(currentItem()));
	  pom = fileInfo->fileName();
	};

      archProcess << pom;

      if (pos==-1) 
	pos = buffer.length();
      buffer = buffer.remove(0,pos+1);
    };

  if ( !archProcess.start(KProcess::DontCare) )
    warning(i18n("Error while creating child process!").ascii());
};

int KfindWindow::numItems() { 
  return count(); 
}
