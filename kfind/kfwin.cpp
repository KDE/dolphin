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

#include <qfileinfo.h>
#include <qdir.h>
#include <qclipboard.h>
#include <qpixmap.h>
#include <qdragobject.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kapp.h>
#include <krun.h>
#include <kprocess.h>
#include <kpropsdlg.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kglobal.h>

#include "kfwin.h"
#include "kfarch.h"
#include "kfsave.h"

extern KfSaveOptions *saving;
extern QList<KfArchiver> *archivers;

#define I18N_NOOP(x) x

// Permission strings
static const char* perm[4] = {
  I18N_NOOP( "Read-write" ),
  I18N_NOOP( "Read-only" ),
  I18N_NOOP( "Write-only" ),
  I18N_NOOP( "Inaccessible" ) };
#define RW 0
#define RO 1
#define WO 2
#define NA 3

// initialize static icons
QPixmap *KfFileLVI::folderPixmap       = 0L;
QPixmap *KfFileLVI::lockedFolderPixmap = 0L;
QPixmap *KfFileLVI::filePixmap         = 0L;
QPixmap *KfFileLVI::lockedFilePixmap   = 0L;

KfFileLVI::KfFileLVI(QListView* lv, QString file)
  : QListViewItem(lv)
{
  fileInfo = new QFileInfo(file);

  QString size = QString("%1").arg(fileInfo->size());

  QString date;
  date = KGlobal::locale()->formatDate(fileInfo->lastModified().date(), true);
  date += " ";
  date += KGlobal::locale()->formatTime(fileInfo->lastModified().time(), true);

  int perm_index;
  if(fileInfo->isReadable())
    perm_index = fileInfo->isWritable() ? RW : RO;
  else
    perm_index = fileInfo->isWritable() ? WO : NA;

  // Fill the item with data
  setText(0, fileInfo->fileName());
  setText(1, fileInfo->dir().path() + "/");
  setText(2, size);
  setText(3, date);
  setText(4, i18n(perm[perm_index]));


  // load the icons (same as in KFileInfoContents)
  // maybe we should use the concrete icon associated with the mimetype
  // in the future, but for now, this must suffice
  
  KIconLoader::Size s = KIconLoader::Small;
  KIconLoader *loader = KGlobal::iconLoader();
  if (!folderPixmap) // don't use IconLoader to always get the same icon
    folderPixmap = new QPixmap(loader->loadApplicationIcon("folder", s));

  if (!lockedFolderPixmap)
    lockedFolderPixmap = new QPixmap(loader->loadApplicationIcon("lockedfolder", s));

  if (!filePixmap)
    filePixmap = new QPixmap(loader->loadApplicationIcon("mimetypes/unknown", s));

  if (!lockedFilePixmap)
    lockedFilePixmap = new QPixmap(loader->loadApplicationIcon("locked", s));

  const int column = 0; // place the icons in leftmost column
  if (fileInfo->isDir()) {
    if (fileInfo->isReadable())
      setPixmap(column, *folderPixmap);
    else
      setPixmap(column, *lockedFolderPixmap);
  }
  else {
    if (fileInfo->isReadable())
      setPixmap(column, *filePixmap);
    else
      setPixmap(column, *lockedFilePixmap);
  }
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

  resetColumns(TRUE);

  connect(this, SIGNAL(doubleClicked(QListViewItem *)),
	  this, SLOT(openBinding()));
}

void KfindWindow::beginSearch()
{
  haveSelection = false;
  clear();
}

void KfindWindow::endSearch()
{
}

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

void KfindWindow::selectAll()
{
  QListViewItem *item = firstChild();
  while(item != NULL) {
    setSelected(item, TRUE);
    item = item->nextSibling();
  }
  selectionChanged(TRUE);
}

void KfindWindow::unselectAll()
{
  QListViewItem *item = firstChild();
  while(item != NULL) {
    setSelected(item, FALSE);
    item = item->nextSibling();
  }
  selectionChanged(FALSE);
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
    KMessageBox::error(parentWidget(), i18n("It wasn't possible to save results!"));
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
    KMessageBox::information(parentWidget(),
			     i18n("Results were saved to file\n")+
			     filename);
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

void KfindWindow::deleteFiles()
{
  QString tmp = i18n("Do you really want to delete selected file(s)?");
  if(KMessageBox::questionYesNo(parentWidget(), tmp) == KMessageBox::No)
    return;

  // Iterate on all selected elements
  QList<KfFileLVI> *selected = selectedItems();
  for ( uint i = 0; i < selected->count(); i++ ) {
    KfFileLVI *item = selected->at(i);
    QFileInfo *file = item->fileInfo;

    // Regular file

    if (file->isFile() ||
	file->isSymLink()) {
      if (remove(file->filePath().ascii()) == -1)
	switch(errno) {
	case EACCES:
	  KMessageBox::error(parentWidget(),
			     i18n("You have no permission\nto delete this file(s)"));
	  break;
	default:
	  KMessageBox::error(parentWidget(),
			     i18n("It is not possible to delete\nselected file(s)"));
	}
      else
	removeItem(item);
    }

    // Directory

    else {
      if (rmdir(file->filePath().ascii()) == -1) {
	switch(errno) {
	case EACCES:
	  KMessageBox::error(parentWidget(),
			     i18n("You have no permission\nto delete this directory"));
	  break;
	case ENOTEMPTY:
	  KMessageBox::error(parentWidget(),
			     i18n("Specified directory\nis not empty"));
	  break;
	default:
	  KMessageBox::error(parentWidget(),
			     i18n("It is not possible to delete\nselected directory"));
	}
      }
      else
	removeItem(item);
    }
  }
}

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
  (void) new KRun(tmp, 0, true, true);
}

void KfindWindow::openBinding()
{
  QString tmp= "file:";
  QFileInfo *fileInfo = ((KfFileLVI*)currentItem())->fileInfo;
  if (fileInfo->isDir())
    tmp += fileInfo->filePath();
  else
    tmp += fileInfo->absFilePath();
  (void) new KRun( tmp, 0, true, true );
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
      KMessageBox::error(parentWidget(),
			   i18n("Couldn't recognize archive type!"));
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

// Resizes QListView to ocuppy all visible space
void KfindWindow::resizeEvent(QResizeEvent *e)
{
  QListView::resizeEvent(e);
  resetColumns(FALSE);
  clipper()->repaint();
}

// The following to functions are an attempt to implement MS-like selection
// (Control/Shift style). Not very elegant.

void KfindWindow::contentsMousePressEvent(QMouseEvent *e)
{
  QListViewItem *item = itemAt(contentsToViewport(e->pos()));
  if(item == NULL) { // someone clicked where no listview item is (below items)
    QListView::contentsMousePressEvent(e);
    clearSelection();
    selectionChanged( false );
    return;
  }

  bool itemWasSelected = isSelected(item);


  // We want to execute QListView::contentsMousePressEvent(e), but
  // we do not want it to change selection and current item.
  // To do it we store current item and make the item we click on unselectable.
  // We restore all this after the contentsMousePressEvent() call
  QListViewItem *anchor = 0L;
  if(e->state() & ShiftButton)
    anchor = currentItem();
  item->setSelectable(FALSE);

  QListView::contentsMousePressEvent(e);

  item->setSelectable(TRUE);
  if(e->state() & ShiftButton)
    setCurrentItem(anchor);

  // Now analyze what we got and make our selections

  // No modifiers
  if(!(e->state() & ControlButton) &&
     !(e->state() & ShiftButton)) {
    if ( !itemWasSelected ) { // otherwise dragging wouldn't work
      clearSelection();
      setSelected(item, TRUE);
      selectionChanged(TRUE);
    }
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
   QListViewItem *i = currentItem();
   if(i == NULL)
     return;

   // Selects area from the current item to our item
   bool down = itemPos(i) < itemPos(item);
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

void KfindWindow::contentsMouseReleaseEvent(QMouseEvent *e)
{
  QListViewItem *item = itemAt(contentsToViewport(e->pos()));
  if(item == NULL) { // someone clicked where no listview item is (below items)
    QListView::contentsMouseReleaseEvent(e);
    return;
  }

  // See comment for the contentsMousePressEvent() function.
  // We just want to disable any selection/current item changes.
  QListViewItem *anchor = 0L;
  if(e->state() & ShiftButton)
    anchor = currentItem();
  item->setSelectable(FALSE);

  QListView::contentsMouseReleaseEvent(e);

  item->setSelectable(TRUE);
  if(e->state() & ShiftButton)
    setCurrentItem(anchor);
}

// drag items from the list (pfeiffer)
void KfindWindow::contentsMouseMoveEvent(QMouseEvent *e)
{
  QListView::contentsMouseMoveEvent(e);

  KfFileLVI *item = (KfFileLVI *) itemAt(contentsToViewport(e->pos()));
  if ( !item )
    return;

  QStringList uris;
  QList<KfFileLVI> *selected = selectedItems();

  // create a list of URIs from selection
  for ( uint i = 0; i < selected->count(); i++ ) {
    if ( (item = selected->at( i )) ) {
      uris.append( item->fileInfo->absFilePath() );;
    }
  }

  if ( uris.count() > 0 ) {
    QUriDrag *ud = new QUriDrag( this, "kfind uridrag" );
    ud->setFilenames( uris );

    if ( uris.count() == 1 ) {
      const QPixmap *pix = currentItem()->pixmap(0);
      if ( pix && !pix->isNull() )
	ud->setPixmap( *pix );
    }
    else {
      // do we have a pixmap symbolizing more than one file?
      const QPixmap *pix = KfFileLVI::filePixmap;
      if ( pix && !pix->isNull() )
	ud->setPixmap( *pix );
    }

    // true => move operation, we need to update the list
    if ( ud->drag() && false ) { // FIXME, why does drag() always return true??
      for ( uint i = 0; i < selected->count(); i++ ) {
	if ( (item = selected->at( i )) ) {
	  removeItem( item );
	}
      }
    }
  }
}

void KfindWindow::resetColumns(bool init)
{
  if(init) {
    QFontMetrics fm = fontMetrics();
    setColumnWidth(2, QMAX(fm.width(columnText(2)), fm.width("0000000")) + 15);
    setColumnWidth(3, QMAX(fm.width(columnText(3)), fm.width("00/00/00 00:00:00")) + 15);
    setColumnWidth(4, QMAX(fm.width(columnText(4)), fm.width(i18n(perm[RO]))) + 15);
  }

  int free_space = visibleWidth() -
    columnWidth(2) - columnWidth(3) - columnWidth(4);

  int name_w = (int)(free_space*0.3); // 30%
  int dir_w = free_space - name_w;    // 70%

  setColumnWidth(0, name_w);
  setColumnWidth(1, dir_w);
}


// returns a pointer to a list of all selected ListViewItems (pfeiffer)
QList<KfFileLVI> * KfindWindow::selectedItems()
{
  mySelectedItems.clear();

  if ( haveSelection ) {
    QListViewItem *item = firstChild();

    while ( item != 0L ) {
      if ( isSelected( item ) )
	mySelectedItems.append( (KfFileLVI *) item );

      item = item->nextSibling();
    }
  }

  return &mySelectedItems;
}
