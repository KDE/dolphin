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
#include <qstrlist.h>
#include <qfiledefs.h>
#include <qfiledialog.h> 
#include <qfileinfo.h> 
#include <qmessagebox.h>
#include <qdir.h>
#include <qclipboard.h>
#include <qevent.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kapp.h>
#include <kdebug.h>
#include <krun.h>
#include <kprocess.h>
#include <kpropsdlg.h>

//#include <knewmenu.h>
//#include <kpopupmenu.h>

#include "kfwin.h"
#include "kfarch.h"
#include "kfsave.h"

extern KfSaveOptions *saving; 
extern QList<KfArchiver> *archivers;

// see comment near use of this variable
static const char * egcsWorkaround = "%x  %X";

// Permission strings
static QString perm[4] = {
  QString(i18n( "Read-write" )),
  QString(i18n( "Read-only" )),
  QString(i18n( "Write-only" )),
  QString(i18n( "Inaccessible" )) };
#define RW 0
#define RO 1
#define WO 2
#define NA 3

KfFileLVI::KfFileLVI(QListView* lv, QString file) 
  : QListViewItem(lv) 
{
  fileInfo = new QFileInfo(file);
  
  QString size = QString("%1").arg(fileInfo->size());
  
  // This code is copyed from QT qfiledialog.cpp
  QDateTime epoch;
  epoch.setTime_t( 0 );
  char date[256];
  time_t t1 = epoch.secsTo( fileInfo->lastModified() );
  struct tm * t2 = ::localtime( &t1 );
  // use a static const char here, so that egcs will not see
  // the formatting string and give an incorrect warning.
  if ( t2 && strftime( date, 255, egcsWorkaround, t2 ) <= 0 )
    strcpy(date, "????");
  //
  
  int perm_index;
  if(fileInfo->isReadable())
    perm_index = fileInfo->isWritable() ? RW : RO;
  else
    perm_index = fileInfo->isWritable() ? WO : NA;
  
  // Fill the item with data
  setText(0, fileInfo->fileName());
  setText(1, fileInfo->dir().path());
  setText(2, size);
  setText(3, date);
  setText(4, perm[perm_index]);
}

KfFileLVI::~KfFileLVI() {
  delete fileInfo;
}

QString KfFileLVI::key(int column, bool) const
{
  // Returns time in secs from 1/1/1980. Used for sorting
  if(column == 3) {
    QDateTime epoch( QDate( 1980, 1, 1 ) );
    return QString().sprintf("%08d", epoch.secsTo(fileInfo->lastModified()));
  } 
  
  return text(column);
}

KfindWindow::KfindWindow( QWidget *parent, const char *name )
  : QListView( parent, name )          
{
  //    topLevelWidget()->installEventFilter(lbx);
  setMultiSelection(TRUE);

  addColumn(i18n("Name"));
  addColumn(i18n("In directory"));
  addColumn(i18n("Size"));
  setColumnAlignment(2, AlignRight);
  addColumn(i18n("Modified"));
  setColumnAlignment(3, AlignRight);
  addColumn(i18n("Permissions"));
  setColumnAlignment(4, AlignRight);

  // Disable autoresize for all columns
  // Resizing is done by resetColumns() function
  for(int i=0; i<5; i++)
    setColumnWidthMode(i, Manual);
  
  /* TODO
     connect(this, SIGNAL(rightButtonPressed(QListViewItem *, const QPoint &, int)),
     this, SLOT(rightButtonPressed(QListViewItem *, const QPoint &, int)));
  */
  
  connect(this, SIGNAL(doubleClicked(QListViewItem *)),
	  this, SLOT(openBinding()));
}

void KfindWindow::beginSearch() 
{
  haveSelection = false;
  
  clear();
  
  resetColumns();
  
  startTimer(100);
  
  QString str = i18n("%1 file(s) found").arg(0);
  emit statusChanged(str.ascii());
}

void KfindWindow::endSearch() 
{
  killTimers(); 
  
  QString str = i18n("%1 file(s) found").arg(childCount());
  emit statusChanged(str.ascii());
}

void KfindWindow::timerEvent(QTimerEvent *) 
{
  if(childCount() > 0) {
    QString str = i18n("%1 file(s) found").arg(childCount());
    emit statusChanged(str.ascii());
  }
}

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

void KfindWindow::updateResults(const char *file )
{
    kdebug(KDEBUG_INFO, 1902, "UPDATERESULTs\n");

    printf("UPDATE\n");

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

    //    insertStrList(strl,-1);
    QString statusmsg = i18n("%1 file(s) found").arg(childCount());
    emit statusChanged(statusmsg.ascii());

	unlink( file );
    fclose(f);    
    delete filename;
    delete strl;
  };

void KfindWindow::insertItem(QString file) {
  new KfFileLVI(this, file);
}

// copy to clipboard aka X11 selection
void KfindWindow::copySelection() 
{
  QString s;
  QListViewItem *item = firstChild();
  while(item != NULL) {
    if(isSelected(item)) {
      s.append(((KfFileLVI*)item)->fileInfo->absFilePath());
      s.append(" ");
    }
    item = item->nextSibling();
  }
  
  if(s.length() > 0) {
    QClipboard *cb = kapp->clipboard();
    cb->clear();
    cb->setText(s);
  }
}

void KfindWindow::changeItem(const char */*itemName*/)
{
  debug("CXHANGE ITEM CALLED\n");
  //    changeItem(itemName,currentItem());    
}

void KfindWindow::selectAll() 
{
  QListViewItem *item = firstChild();
  while(item != NULL) {
    setSelected(item, TRUE);
    item = item->nextSibling();
  }
}

void KfindWindow::unselectAll() 
{
  QListViewItem *item = firstChild();
  while(item != NULL) {
    setSelected(item, FALSE);
    item = item->nextSibling();
  }
}

void KfindWindow::saveResults()
{ 
  QListViewItem *item;
  FILE *results;
  QString filename;
  
  if ( saving->getSaveStandard() ) {
    filename = getenv("HOME");
    filename += "/.kfind-results.html";
  }
  else
    filename = saving->getSaveFile();
  
  results=fopen(filename.ascii(),"w");
  
  if (results == 0L)
    QMessageBox::warning(parentWidget(),i18n("Error"),
			 i18n("It wasn't possible to save results!"),
			 i18n("OK"));
  else {
    if ( saving->getSaveFormat() == "HTML" ) {
      fprintf(results,"<HTML><HEAD>\n");
      fprintf(results,"<!DOCTYPE %s>\n",
	      i18n("KFind Results File").ascii());
      fprintf(results,"<TITLE>%sKFind Results File</TITLE></HEAD>\n",
	      i18n("KFind Results File").ascii());
      fprintf(results,"<BODY><H1>%s</H1>\n",
	      i18n("KFind Results File").ascii());
      fprintf(results,"<DL><p>\n"); 
      
      item = firstChild();
      while(item != NULL) {
	QString path=((KfFileLVI*)item)->fileInfo->absFilePath();
	fprintf(results,"<DT><A HREF=\"file:%s\">file:%s</A>\n",
		path.ascii(), path.ascii());
	item = item->nextSibling();
      }
      fprintf(results,"</DL><P></BODY></HTML>\n");
    }
    else {
      item = firstChild();
      while(item != NULL) {
	QString path=((KfFileLVI*)item)->fileInfo->absFilePath();
	fprintf(results,"%s\n", path.ascii());
	item = item->nextSibling();
      }
    }	

    fclose(results); 
    QMessageBox::information(parentWidget(),
			     i18n("Information"),
			     i18n("Results were saved to file\n")+
			     filename,
			     i18n("OK"));
  }
}

// This function is called when selection is changed (both selected/deselected)
// It notifies the parent about selection status and enables/disables menubar
void KfindWindow::selectionChanged(bool selectionMade) 
{
  if(selectionMade) {
    if(!haveSelection) {
      haveSelection = true;
      emit resultSelected(true);
    }
  }
  else {
    // If user made deselection we want to check if any items left selected.
    // If no items are selected disable menubar.
    QListViewItem *item = firstChild();
    while(item != NULL) {
      if(isSelected(item))
	break;
      item = item->nextSibling();
    }
    
    // Item equal to NULL means we do not have any selection 
    if(item == NULL) {
      haveSelection = false;
      emit resultSelected(false);
    }
  }
}

// Currently disabled
void KfindWindow::rightButtonPressed(QListViewItem */*ci*/, 
				     const QPoint &/*pos*/, int) {
  /*
  QStringList urls;
  QListViewItem *item = firstChild();
  while(item != NULL) {
    if(isSelected(item)) {
      urls.append("file:" + ((KfFileLVI*)item)->fileInfo->absFilePath());
    }
    item = item->nextSibling();
  }
  KonqPopupMenu popup(urls,
		      0,
		      "file:" + ((KfFileLVI*)ci)->fileInfo->absFilePath(), 
		      FALSE,
		      FALSE,
		      FALSE);
  popup.exec(pos);
  */
}

// Currently disables. Should work using  KonqPopupMenu().
void KfindWindow::deleteFiles()
  {
    /*    QString tmp = i18n("Do you really want to delete file:\n%1")
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
    */
  };

void KfindWindow::fileProperties()
{
  QString tmp= "file:";
  QFileInfo *fileInfo = ((KfFileLVI *)currentItem())->fileInfo;
  if (fileInfo->isDir())
    tmp += fileInfo->filePath();
  else
    tmp += fileInfo->absFilePath();
  (void) new PropertiesDialog(tmp);
}

void KfindWindow::openFolder()
{
  QString tmp= "file:";
  QFileInfo *fileInfo = ((KfFileLVI *)currentItem())->fileInfo;
  if (fileInfo->isDir())
    tmp += fileInfo->filePath();
  else
    tmp += fileInfo->dirPath();
  (void) new KRun(tmp);
}

void KfindWindow::openBinding() 
{
  QString tmp= "file:";
  QFileInfo *fileInfo = ((KfFileLVI*)currentItem())->fileInfo;
  if (fileInfo->isDir())
    tmp += fileInfo->filePath();
  else
    tmp += fileInfo->absFilePath();
  (void) new KRun( tmp );
}

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
}

void KfindWindow::execAddToArchive(KfArchiver *arch, QString archname)
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
	  QFileInfo *fileInfo = ((KfFileLVI*)currentItem())->fileInfo;
	  pom = fileInfo->dirPath(TRUE)+'/';
	}
      
      if ( pom=="%a" )
	pom = archname;
      
      if ( pom=="%f" )
	pom = ((KfFileLVI*)currentItem())->fileInfo->absFilePath();;
      
      if ( pom=="%n" )
	{
	  QFileInfo *fileInfo = ((KfFileLVI*)currentItem())->fileInfo;
	  pom = fileInfo->fileName();
	}
      
      archProcess << pom;
      
      if (pos==-1) 
	pos = buffer.length();
      buffer = buffer.remove(0,pos+1);
    }
  
  if ( !archProcess.start(KProcess::DontCare) )
    warning(i18n("Error while creating child process!").ascii());
}

// Resizes QListView to ocuppy all space
void KfindWindow::resizeEvent(QResizeEvent *e) {
  resetColumns();
  QListView::resizeEvent(e);
}

// The following to functions are an attemp to implement MS-like selection
// (Control/Shift style). Not very elegant.

void KfindWindow::contentsMousePressEvent(QMouseEvent *e) {
  
  QListViewItem *item = itemAt(contentsToViewport(e->pos()));
  if(item == NULL) { // Just in case. Should not happen
    QListView::contentsMousePressEvent(e);
    return;
  }
  
  // We want to execute QListView::contentsMousePressEvent(e), but 
  // we do not want it to change selection and current item.
  // To do it we store current item and make the item we click on unselectable.
  // We restore all this after the contentsMousePressEvent() call
  QListViewItem *anchor;
  if(e->state() & ShiftButton) 
    anchor = currentItem();
  item->setSelectable(FALSE);
  
  QListView::contentsMousePressEvent(e);
  
  item->setSelectable(TRUE);
  if(e->state() & ShiftButton) 
    setCurrentItem(anchor);
  
  // No analize what we got and make our selections
  
  // No modifiers
  if(!(e->state() & ControlButton) &&
     !(e->state() & ShiftButton)) {
    clearSelection();
    setSelected(item, TRUE);
    selectionChanged(TRUE);
    return;
  }
  
  // Control
  if(e->state() & ControlButton) {
    setSelected(item, !isSelected(item));
    selectionChanged(isSelected(item));
    return;
  }

  // Shift
  if(e->state() & ShiftButton) {
    bool down = itemPos(currentItem()) < itemPos(item);
    
    // Selects area from the current item to our item
    QListViewItem *i = currentItem();
    if(down) {
      while(i != item) {
	setSelected(i, TRUE);
	i = i->itemBelow();
      }
    }
    else {
      while(i != item) {
	setSelected(i, TRUE);
	i = i->itemAbove();
      }
    }
    setSelected(item, TRUE);
  }
}

void KfindWindow::contentsMouseReleaseEvent(QMouseEvent *e) {

  QListViewItem *item = itemAt(contentsToViewport(e->pos()));
  if(item == NULL) { // Just in case. Should not happen
    QListView::contentsMouseReleaseEvent(e);
    return;
  }
  
  // See comment for the contentsMousePressEvent() function.
  // We just want to disable any selection/current item changes.
  QListViewItem *anchor;
  if(e->state() & ShiftButton) 
    anchor = currentItem();
  item->setSelectable(FALSE);
  
  QListView::contentsMouseReleaseEvent(e);
  
  item->setSelectable(TRUE);
  if(e->state() & ShiftButton) 
    setCurrentItem(anchor);
}

void KfindWindow::resetColumns() {
  

  QFontMetrics fm = fontMetrics();
  int perm_w = fm.width(perm[RO]) + 15;
  int size_w = fm.width("0000000") + 15;
  int mod_w = fm.width("00/00/00 00:00:00") + 15;
  int free_space = width() - perm_w - size_w - mod_w;
  int name_w = (int)(free_space*0.3); // 30% 
  int dir_w = free_space - name_w;    // 70%
  
  setColumnWidth(0, name_w);
  setColumnWidth(1, dir_w);
  setColumnWidth(2, size_w);
  setColumnWidth(3, mod_w);
  setColumnWidth(4, perm_w);
}
