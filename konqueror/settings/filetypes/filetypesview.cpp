/* Missing license header */

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTimer>
//Added by qt3to4:
#include <QGridLayout>
#include <QHBoxLayout>

#include <kapplication.h>
#include <kbuildsycocaprogressdialog.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kipc.h>
#include <klineedit.h>
#include <k3listview.h>
#include <klocale.h>
#include <kservicetypeprofile.h>
#include <kstandarddirs.h>

#include "newtypedlg.h"
#include "filetypedetails.h"
#include "filegroupdetails.h"
#include "filetypesview.h"
#include <ksycoca.h>

FileTypesView::FileTypesView(KInstance *inst, QWidget *parent)
  : KCModule(inst, parent)
{
  m_konqConfig = KSharedConfig::openConfig("konquerorrc", false, false);

  setQuickHelp( i18n("<h1>File Associations</h1>"
    " This module allows you to choose which applications are associated"
    " with a given type of file. File types are also referred to MIME types"
    " (MIME is an acronym which stands for \"Multipurpose Internet Mail"
    " Extensions\".)<p> A file association consists of the following:"
    " <ul><li>Rules for determining the MIME-type of a file, for example"
    " the filename pattern *.kwd, which means 'all files with names that end"
    " in .kwd', is associated with the MIME type \"x-kword\";</li>"
    " <li>A short description of the MIME-type, for example the description"
    " of the MIME type \"x-kword\" is simply 'KWord document';</li>"
    " <li>An icon to be used for displaying files of the given MIME-type,"
    " so that you can easily identify the type of file in, say, a Konqueror"
    " view (at least for the types you use often);</li>"
    " <li>A list of the applications which can be used to open files of the"
    " given MIME-type -- if more than one application can be used then the"
    " list is ordered by priority.</li></ul>"
    " You may be surprised to find that some MIME types have no associated"
    " filename patterns; in these cases, Konqueror is able to determine the"
    " MIME-type by directly examining the contents of the file."));

  KServiceTypeProfile::setConfigurationMode();
  setButtons(Help | Apply);
  QString wtstr;

  QHBoxLayout *l = new QHBoxLayout(this);
  l->setMargin(KDialog::marginHint());
  QGridLayout *leftLayout = new QGridLayout((QWidget*)0L);
  leftLayout->setSpacing(KDialog::spacingHint());
  leftLayout->setColumnStretch(1, 1);

  l->addLayout( leftLayout );

  QLabel *patternFilterLBL = new QLabel(i18n("F&ind filename pattern:"), this);
  leftLayout->addWidget(patternFilterLBL, 0, 0, 1, 3);

  patternFilterLE = new KLineEdit(this);
  patternFilterLBL->setBuddy( patternFilterLE );
  leftLayout->addWidget(patternFilterLE, 1, 0, 1, 3);

  connect(patternFilterLE, SIGNAL(textChanged(const QString &)),
          this, SLOT(slotFilter(const QString &)));

  wtstr = i18n("Enter a part of a filename pattern. Only file types with a "
               "matching file pattern will appear in the list.");

  patternFilterLE->setWhatsThis( wtstr );
  patternFilterLBL->setWhatsThis( wtstr );

  typesLV = new K3ListView(this);
  typesLV->setRootIsDecorated(true);
  typesLV->setFullWidth(true);

  typesLV->addColumn(i18n("Known Types"));
  leftLayout->addWidget(typesLV, 2, 0, 1, 3);
  connect(typesLV, SIGNAL(selectionChanged(Q3ListViewItem *)),
          this, SLOT(updateDisplay(Q3ListViewItem *)));
  connect(typesLV, SIGNAL(doubleClicked(Q3ListViewItem *)),
          this, SLOT(slotDoubleClicked(Q3ListViewItem *)));

  typesLV->setWhatsThis( i18n("Here you can see a hierarchical list of"
    " the file types which are known on your system. Click on the '+' sign"
    " to expand a category, or the '-' sign to collapse it. Select a file type"
    " (e.g. text/html for HTML files) to view/edit the information for that"
    " file type using the controls on the right.") );

  QPushButton *addTypeB = new QPushButton(i18n("Add..."), this);
  connect(addTypeB, SIGNAL(clicked()), SLOT(addType()));
  leftLayout->addWidget(addTypeB, 3, 0);

  addTypeB->setWhatsThis( i18n("Click here to add a new file type.") );

  m_removeTypeB = new QPushButton(i18n("&Remove"), this);
  connect(m_removeTypeB, SIGNAL(clicked()), SLOT(removeType()));
  leftLayout->addWidget(m_removeTypeB, 3, 2);
  m_removeTypeB->setEnabled(false);

  m_removeTypeB->setWhatsThis( i18n("Click here to remove the selected file type.") );

  // For the right panel, prepare a widget stack
  m_widgetStack = new QStackedWidget(this);

  l->addWidget( m_widgetStack );

  // File Type Details
  m_details = new FileTypeDetails( m_widgetStack );
  connect( m_details, SIGNAL( changed(bool) ),
           this, SLOT( setDirty(bool) ) );
  connect( m_details, SIGNAL( embedMajor(const QString &, bool &) ),
           this, SLOT( slotEmbedMajor(const QString &, bool &)));
  m_widgetStack->insertWidget( 1, m_details /*id*/ );

  // File Group Details
  m_groupDetails = new FileGroupDetails( m_widgetStack );
  connect( m_groupDetails, SIGNAL( changed(bool) ),
           this, SLOT( setDirty(bool) ) );
  m_widgetStack->insertWidget( 2,m_groupDetails /*id*/ );

  // Widget shown on startup
  m_emptyWidget = new QLabel( i18n("Select a file type by name or by extension"), m_widgetStack);
  m_emptyWidget->setAlignment( Qt::AlignCenter );

  m_widgetStack->insertWidget( 3,m_emptyWidget );

  m_widgetStack->setCurrentWidget( m_emptyWidget );

  QTimer::singleShot( 0, this, SLOT( init() ) ); // this takes some time

  connect( KSycoca::self(), SIGNAL( databaseChanged() ), SLOT( slotDatabaseChanged() ) );
}

FileTypesView::~FileTypesView()
{
}

void FileTypesView::setDirty(bool state)
{
  emit changed(state);
  m_dirty = state;
}

void FileTypesView::init()
{
  show();
  setEnabled( false );

  setCursor( KCursor::waitCursor() );
  readFileTypes();
  unsetCursor();

  setDirty(false);
  setEnabled( true );
}

// only call this method once on startup, then never again! Otherwise, newly
// added Filetypes will be lost.
void FileTypesView::readFileTypes()
{
    typesLV->clear();
    m_majorMap.clear();
    m_itemList.clear();
    TypesListItem::reset();

    TypesListItem *groupItem;
    KMimeType::List mimetypes = KMimeType::allMimeTypes();
    KMimeType::List::const_iterator it2(mimetypes.begin());
    for (; it2 != mimetypes.end(); ++it2) {
	QString mimetype = (*it2)->name();
	int index = mimetype.indexOf("/");
	QString maj = mimetype.left(index);
	QString min = mimetype.right(mimetype.length() - index+1);

	QMap<QString,TypesListItem*>::const_iterator mit = m_majorMap.find( maj );
	if ( mit == m_majorMap.end() ) {
	    groupItem = new TypesListItem( typesLV, maj );
	    m_majorMap.insert( maj, groupItem );
	}
	else
	    groupItem = mit.value();

	TypesListItem *item = new TypesListItem(groupItem, (*it2));
	m_itemList.append( item );
    }
    updateDisplay(0L);

}

void FileTypesView::slotEmbedMajor(const QString &major, bool &embed)
{
    TypesListItem *groupItem;
    QMap<QString,TypesListItem*>::const_iterator mit = m_majorMap.find( major );
    if ( mit == m_majorMap.end() )
        return;

    groupItem = mit.value();

    embed = (groupItem->autoEmbed() == 0);
}

void FileTypesView::slotFilter(const QString & patternFilter)
{
    // one of the few ways to clear a listview without destroying the
    // listviewitems and without making QListView crash.
    Q3ListViewItem *item;
    while ( (item = typesLV->firstChild()) ) {
	while ( item->firstChild() )
	    item->takeItem( item->firstChild() );

	typesLV->takeItem( item );
    }

    // insert all items and their group that match the filter
    Q3PtrListIterator<TypesListItem> it( m_itemList );
    while ( it.current() ) {
	if ( patternFilter.isEmpty() ||
	     !((*it)->patterns().filter( patternFilter, Qt::CaseInsensitive )).isEmpty() ) {

	    TypesListItem *group = m_majorMap[ (*it)->majorType() ];
	    // QListView makes sure we don't insert a group-item more than once
	    typesLV->insertItem( group );
	    group->insertItem( *it );
	}
	++it;
    }
}

void FileTypesView::addType()
{
  QStringList allGroups;
  QMap<QString,TypesListItem*>::iterator it = m_majorMap.begin();
  while ( it != m_majorMap.end() ) {
      allGroups.append( it.key() );
      ++it;
  }

  NewTypeDialog m(allGroups, this);

  if (m.exec()) {
    Q3ListViewItemIterator it(typesLV);
    QString loc = m.group() + "/" + m.text() + ".desktop";
    loc = KStandardDirs::locate("mime", loc);
    KMimeType::Ptr mimetype(new KMimeType(loc,
                                          m.group() + "/" + m.text(),
                                          QString(), QString(),
                                          QStringList()));

    TypesListItem *group = m_majorMap[ m.group() ];
    if ( !group )
    {
       //group = new TypesListItem(
       //TODO ! (The combo in NewTypeDialog must be made editable again when that happens)
       Q_ASSERT(group);
    }

    // find out if our group has been filtered out -> insert if necessary
    Q3ListViewItem *item = typesLV->firstChild();
    bool insert = true;
    while ( item ) {
	if ( item == group ) {
	    insert = false;
	    break;
	}
	item = item->nextSibling();
    }
    if ( insert )
	typesLV->insertItem( group );

    TypesListItem *tli = new TypesListItem(group, mimetype, true);
    m_itemList.append( tli );

    group->setOpen(true);
    typesLV->setSelected(tli, true);

    setDirty(true);
  }
}

void FileTypesView::removeType()
{
  TypesListItem *current = (TypesListItem *) typesLV->currentItem();

  if ( !current )
      return;

  // Can't delete groups
  if ( current->isMeta() )
      return;
  // nor essential mimetypes
  if ( current->isEssential() )
      return;

  Q3ListViewItem *li = current->itemAbove();
  if (!li)
      li = current->itemBelow();
  if (!li)
      li = current->parent();

  removedList.append(current->name());
  current->parent()->takeItem(current);
  m_itemList.removeRef( current );
  setDirty(true);

  if ( li )
      typesLV->setSelected(li, true);
}

void FileTypesView::slotDoubleClicked(Q3ListViewItem *item)
{
  if ( !item ) return;
  item->setOpen( !item->isOpen() );
}

void FileTypesView::updateDisplay(Q3ListViewItem *item)
{
  if (!item)
  {
    m_widgetStack->setCurrentWidget( m_emptyWidget );
    m_removeTypeB->setEnabled(false);
    return;
  }

  bool wasDirty = m_dirty;

  TypesListItem *tlitem = (TypesListItem *) item;
  if (tlitem->isMeta()) // is a group
  {
    m_widgetStack->setCurrentWidget( m_groupDetails );
    m_groupDetails->setTypeItem( tlitem );
    m_removeTypeB->setEnabled(false);
  }
  else
  {
    m_widgetStack->setCurrentWidget( m_details );
    m_details->setTypeItem( tlitem );
    m_removeTypeB->setEnabled( !tlitem->isEssential() );
  }

  // Updating the display indirectly called change(true)
  if ( !wasDirty )
    setDirty(false);
}

bool FileTypesView::sync( QList<TypesListItem *>& itemsModified )
{
  bool didIt = false;
  // first, remove those items which we are asked to remove.
  QStringList::Iterator it(removedList.begin());
  QString loc;

  for (; it != removedList.end(); ++it) {
    didIt = true;
    KMimeType::Ptr m_ptr = KMimeType::mimeType(*it);

    loc = m_ptr->desktopEntryPath();
    loc = KStandardDirs::locate("mime", loc);

    KDesktopFile config(loc, false, "mime");
    config.writeEntry("Type", "MimeType");
    config.writeEntry("MimeType", m_ptr->name());
    config.writeEntry("Hidden", true);
  }

  // now go through all entries and sync those which are dirty.
  // don't use typesLV, it may be filtered
  QMap<QString,TypesListItem*>::iterator it1 = m_majorMap.begin();
  while ( it1 != m_majorMap.end() ) {
    TypesListItem *tli = *it1;
    if (tli->isDirty()) {
      kDebug() << "Entry " << tli->name() << " is dirty. Saving." << endl;
      tli->sync();
      itemsModified.append( tli );
      didIt = true;
    }
    ++it1;
  }
  Q3PtrListIterator<TypesListItem> it2( m_itemList );
  while ( it2.current() ) {
    TypesListItem *tli = *it2;
    if (tli->isDirty()) {
      kDebug() << "Entry " << tli->name() << " is dirty. Saving." << endl;
      tli->sync();
      itemsModified.append( tli );
      didIt = true;
    }
    ++it2;
  }

  m_konqConfig->sync();

  setDirty(false);
  return didIt;
}

void FileTypesView::load()
{
    readFileTypes();
}

void FileTypesView::save()
{
  m_itemsModified.clear();
  if (sync(m_itemsModified)) {
    // only rebuild if sync() was necessary
    KBuildSycocaProgressDialog::rebuildKSycoca(this);
    KIPC::sendMessageAll(KIPC::SettingsChanged);
  }
}

void FileTypesView::slotDatabaseChanged()
{
  if ( KSycoca::self()->isChanged( "mime" ) )
  {
    // ksycoca has new KMimeTypes objects for us, make sure to update
    // our 'copies' to be in sync with it. Not important for OK, but
    // important for Apply (how to differentiate those 2?).
    // See BR 35071.
    QList<TypesListItem *>::Iterator it = m_itemsModified.begin();
    for( ; it != m_itemsModified.end(); ++it ) {
        QString name = (*it)->name();
        if ( removedList.indexOf( name ) == -1 ) // if not deleted meanwhile
            (*it)->refresh();
    }
    m_itemsModified.clear();
  }
}

void FileTypesView::defaults()
{
}

#include "filetypesview.moc"


