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

#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QClipboard>
#include <QPixmap>
#include <q3dragobject.h>
#include <QTextDocument>
#include <q3ptrlist.h>
#include <QDateTime>

#include <kfiledialog.h>
#include <klocale.h>
#include <kapplication.h>
#include <krun.h>
#include <kprocess.h>
#include <kpropertiesdialog.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kglobal.h>
#include <kmenu.h>
#include <kio/netaccess.h>
#include <k3urldrag.h>
#include <kdebug.h>
#include <kiconloader.h>

#include "kfwin.h"

#include "kfwin.moc"

template class Q3PtrList<KfFileLVI>;

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

KfFileLVI::KfFileLVI(KfindWindow* lv, const KFileItem &item, const QString& matchingLine)
  : Q3ListViewItem(lv),
    fileitem(item)
{
  fileInfo = new QFileInfo(item.url().path());

  QString size = KGlobal::locale()->formatNumber(item.size(), 0);

  QDateTime dt;
  dt.setTime_t(item.time(KIO::UDS_MODIFICATION_TIME));
  QString date = KGlobal::locale()->formatDateTime(dt);

  int perm_index;
  if(fileInfo->isReadable())
    perm_index = fileInfo->isWritable() ? RW : RO;
  else
    perm_index = fileInfo->isWritable() ? WO : NA;

  // Fill the item with data
  setText(0, item.url().fileName(false));
  setText(1, lv->reducedDir(item.url().directory(false)));
  setText(2, size);
  setText(3, date);
  setText(4, i18n(perm[perm_index]));
  setText(5, matchingLine);

  // put the icon into the leftmost column
  setPixmap(0, item.pixmap(16));
}

KfFileLVI::~KfFileLVI()
{
  delete fileInfo;
}

QString KfFileLVI::key(int column, bool) const
{
  switch (column) {
  case 2:
    // Returns size in bytes. Used for sorting
    return QString().sprintf("%010ld", (long int)fileInfo->size());
  case 3:
    // Returns time in secs from 1/1/1970. Used for sorting
    return QString().sprintf("%010ld", fileitem.time(KIO::UDS_MODIFICATION_TIME));
  }

  return text(column);
}

KfindWindow::KfindWindow( QWidget *parent )
  : K3ListView( parent )
,m_baseDir()
,m_menu(0)
{
  setSelectionMode( Q3ListView::Extended );
  setShowSortIndicator( true );

  addColumn(i18n("Name"));
  addColumn(i18n("In Subfolder"));
  addColumn(i18n("Size"));
  setColumnAlignment(2, Qt::AlignRight);
  addColumn(i18n("Modified"));
  setColumnAlignment(3, Qt::AlignRight);
  addColumn(i18n("Permissions"));
  setColumnAlignment(4, Qt::AlignRight);

  addColumn(i18n("First Matching Line"));
  setColumnAlignment(5, Qt::AlignLeft);

  // Disable autoresize for all columns
  // Resizing is done by resetColumns() function
  for (int i = 0; i < 6; i++)
    setColumnWidthMode(i, Manual);

  resetColumns(true);

  connect( this, SIGNAL(selectionChanged()),
	   this, SLOT( selectionHasChanged() ));

  connect(this, SIGNAL(contextMenu(K3ListView *, Q3ListViewItem*,const QPoint&)),
	  this, SLOT(slotContextMenu(K3ListView *,Q3ListViewItem*,const QPoint&)));

  connect(this, SIGNAL(executed(Q3ListViewItem*)),
	  this, SLOT(slotExecute(Q3ListViewItem*)));
  setDragEnabled(true);

}


QString KfindWindow::reducedDir(const QString& fullDir)
{
   if (fullDir.indexOf(m_baseDir)==0)
   {
      QString tmp=fullDir.mid(m_baseDir.length());
      return tmp;
   };
   return fullDir;
}

void KfindWindow::beginSearch(const KUrl& baseUrl)
{
  kDebug()<<QString("beginSearch in: %1").arg(baseUrl.path())<<endl;
  m_baseDir=baseUrl.path(KUrl::AddTrailingSlash);
  haveSelection = false;
  clear();
}

void KfindWindow::endSearch()
{
}

void KfindWindow::insertItem(const KFileItem &item, const QString& matchingLine)
{
  new KfFileLVI(this, item, matchingLine);
}

// copy to clipboard aka X11 selection
void KfindWindow::copySelection()
{
  Q3DragObject *drag_obj = dragObject();

  if (drag_obj)
  {
    QClipboard *cb = kapp->clipboard();
    cb->setData(drag_obj);
  }
}

void KfindWindow::saveResults()
{
  Q3ListViewItem *item;

  KFileDialog *dlg = new KFileDialog(QString(), QString(), this);
  dlg->setOperationMode (KFileDialog::Saving);

  dlg->setCaption(i18n("Save Results As"));

  QStringList list;

  list << "text/plain" << "text/html";

  dlg->setOperationMode(KFileDialog::Saving);

  dlg->setMimeFilter(list, QString("text/plain"));

  dlg->exec();

  KUrl u = dlg->selectedUrl();
  KMimeType::Ptr mimeType = dlg->currentFilterMimeType();
  delete dlg;

  if (!u.isValid() || !u.isLocalFile())
     return;

  QString filename = u.path();

  QFile file(filename);

  if ( !file.open(QIODevice::WriteOnly) )
    KMessageBox::error(parentWidget(),
		       i18n("Unable to save results."));
  else {
    QTextStream stream( &file );
    stream.setCodec( QTextCodec::codecForLocale() );

    if ( mimeType->name() == "text/html") {
      stream << QString::fromLatin1("<HTML><HEAD>\n"
				    "<!DOCTYPE %1>\n"
				    "<TITLE>%2</TITLE></HEAD>\n"
				    "<BODY><H1>%3</H1>"
				    "<DL><p>\n")
	.arg(i18n("KFind Results File"))
	.arg(i18n("KFind Results File"))
	.arg(i18n("KFind Results File"));

      item = firstChild();
      while(item != NULL)
	{
	  QString path=((KfFileLVI*)item)->fileitem.url().url();
	  QString pretty=Qt::escape(((KfFileLVI*)item)->fileitem.url().prettyUrl());
	  stream << QString::fromLatin1("<DT><A HREF=\"") << path
		 << QString::fromLatin1("\">") << pretty
		 << QString::fromLatin1("</A>\n");

	  item = item->nextSibling();
	}
      stream << QString::fromLatin1("</DL><P></BODY></HTML>\n");
    }
    else {
      item = firstChild();
      while(item != NULL)
      {
	QString path=((KfFileLVI*)item)->fileitem.url().url();
	stream << path << endl;
	item = item->nextSibling();
      }
    }

    file.close();
    KMessageBox::information(parentWidget(),
			     i18n("Results were saved to file\n")+
			     filename);
  }
}

// This function is called when selection is changed (both selected/deselected)
// It notifies the parent about selection status and enables/disables menubar
void KfindWindow::selectionHasChanged()
{
  emit resultSelected(true);

  Q3ListViewItem *item = firstChild();
  while(item != 0L)
  {
    if(isSelected(item)) {
      emit resultSelected( true );
      haveSelection = true;
      return;
    }

    item = item->nextSibling();
  }

  haveSelection = false;
  emit resultSelected(false);
}

void KfindWindow::deleteFiles()
{
  QString tmp = i18np("Do you really want to delete the selected file?",
                     "Do you really want to delete the %n selected files?",selectedItems().count());
  if (KMessageBox::warningContinueCancel(parentWidget(), tmp, "", KStdGuiItem::del()) == KMessageBox::Cancel)
    return;

  // Iterate on all selected elements
  QList<Q3ListViewItem*> selected = selectedItems();
  foreach ( Q3ListViewItem* listViewItem, selected ) {
    KfFileLVI *item = static_cast<KfFileLVI*>(listViewItem);
    KFileItem file = item->fileitem;

    KIO::NetAccess::del(file.url(), this);
  }
  qDeleteAll(selected);
}

void KfindWindow::fileProperties()
{
  // This dialog must be modal because it parent dialog is modal as well.
  // Non-modal property dialog will hide behind the main window
  (void) new KPropertiesDialog( &((KfFileLVI *)currentItem())->fileitem, this,
				"propDialog", true);
}

void KfindWindow::openFolder()
{
  KFileItem fileitem = ((KfFileLVI *)currentItem())->fileitem;
  KUrl url = fileitem.url();
  url.setFileName(QString());

  (void) new KRun(url, this);
}

void KfindWindow::openBinding()
{
  ((KfFileLVI*)currentItem())->fileitem.run();
}

void KfindWindow::slotExecute(Q3ListViewItem* item)
{
   if (item==0)
      return;
  ((KfFileLVI*)item)->fileitem.run();
}

// Resizes K3ListView to occupy all visible space
void KfindWindow::resizeEvent(QResizeEvent *e)
{
  K3ListView::resizeEvent(e);
  resetColumns(false);
  clipper()->repaint();
}

Q3DragObject * KfindWindow::dragObject()
{
  KUrl::List uris;
  QList<Q3ListViewItem*> selected = selectedItems();

  // create a list of URIs from selection
  foreach ( Q3ListViewItem* listViewItem, selected )
  {
    KfFileLVI *item = static_cast<KfFileLVI*>(listViewItem);

    uris.append( item->fileitem.url() );
  }

  if ( uris.count() <= 0 )
     return 0;

  Q3UriDrag *ud = new K3URLDrag( uris, (QWidget *) this );
  ud->setObjectName( "kfind uridrag" );

  const QPixmap *pix = currentItem()->pixmap(0);
  if ( pix && !pix->isNull() )
    ud->setPixmap( *pix );

  return ud;
}

void KfindWindow::resetColumns(bool init)
{
   QFontMetrics fm = fontMetrics();
  if (init)
  {
    setColumnWidth(2, qMax(fm.width(columnText(2)), fm.width("0000000")) + 15);
    QString sampleDate =
      KGlobal::locale()->formatDateTime(QDateTime::currentDateTime());
    setColumnWidth(3, qMax(fm.width(columnText(3)), fm.width(sampleDate)) + 15);
    setColumnWidth(4, qMax(fm.width(columnText(4)), fm.width(i18n(perm[RO]))) + 15);
    setColumnWidth(5, qMax(fm.width(columnText(5)), fm.width("some text")) + 15);
  }

  int free_space = visibleWidth() -
    columnWidth(2) - columnWidth(3) - columnWidth(4) - columnWidth(5);

//  int name_w = qMin((int)(free_space*0.5), 150);
//  int dir_w = free_space - name_w;
  int name_w = qMax((int)(free_space*0.5), fm.width("some long filename"));
  int dir_w = name_w;

  setColumnWidth(0, name_w);
  setColumnWidth(1, dir_w);
}

void KfindWindow::slotContextMenu(K3ListView *,Q3ListViewItem *item,const QPoint&p)
{
  if (!item) return;
  int count = selectedItems().count();

  if (count == 0)
  {
     return;
  };

  if (m_menu==0)
     m_menu = new KMenu(this);
  else
     m_menu->clear();

  if (count == 1)
  {
     //menu = new KMenu(item->text(0), this);
     m_menu->addTitle(item->text(0));
     m_menu->addAction(SmallIcon("fileopen"),i18nc("Menu item", "Open"), this, SLOT(openBinding()));
     m_menu->addAction(SmallIcon("window_new"),i18n("Open Folder"), this, SLOT(openFolder()));
     m_menu->addSeparator();
     m_menu->addAction(SmallIcon("editcopy"),i18n("Copy"), this, SLOT(copySelection()));
     m_menu->addAction(SmallIcon("editdelete"),i18n("Delete"), this, SLOT(deleteFiles()));
     m_menu->addSeparator();
     m_menu->addAction(i18n("Open With..."), this, SLOT(slotOpenWith()));
     m_menu->addSeparator();
     m_menu->addAction(i18n("Properties"), this, SLOT(fileProperties()));
  }
  else
  {
     m_menu->addTitle(i18n("Selected Files"));
     m_menu->addAction(SmallIcon("editcopy"),i18n("Copy"), this, SLOT(copySelection()));
     m_menu->addAction(SmallIcon("editdelete"),i18n("Delete"), this, SLOT(deleteFiles()));
  }
  m_menu->exec(p);
}

void KfindWindow::slotOpenWith()
{
   KRun::displayOpenWithDialog( KUrl::split(((KfFileLVI*)currentItem())->fileitem.url()), topLevelWidget() );
}
