/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (c) 1999 Preston Brown <pbrown@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/*
 * kpropsdlg.cpp
 * View/Edit Properties of files, locally or remotely
 *
 * some FilePermissionsPropsPage-changes by
 *  Henner Zeller <zeller@think.de>
 * some layout management by
 *  Bertrand Leconte <B.Leconte@mail.dotcom.fr>
 * the rest of the layout management, bug fixes, adaptation to libkio,
 * template feature by
 *  David Faure <faure@kde.org>
 * More layout and cleanups by
 *  Preston Brown <pbrown@kde.org>
 */

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include <qfile.h>
#include <qdir.h>
#include <qdict.h>
#include <klined.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qstrlist.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qpainter.h>
#include <qlayout.h>

#include <kdialog.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kiconloaderdialog.h>
#include <kurl.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kfiledialog.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <kfileitem.h>
#include <kuserpaths.h>

#include "kpropsdlg.h"

#include <X11/Xlib.h> // for XSetTransientForHint

#define ROUND(x) ((int)(0.5 + (x)))

mode_t FilePermissionsPropsPage::fperm[3][4] = {
        {S_IRUSR, S_IWUSR, S_IXUSR, S_ISUID},
        {S_IRGRP, S_IWGRP, S_IXGRP, S_ISGID},
        {S_IROTH, S_IWOTH, S_IXOTH, S_ISVTX}
    };

PropertiesDialog::PropertiesDialog( KFileItemList _items ) :
  m_singleUrl( _items.first()->url() ), m_items( _items ),
  m_bMustDestroyItems( false )
{
  init();
}

PropertiesDialog::PropertiesDialog( const QString& _url, mode_t _mode ) :
  m_singleUrl( _url ), m_bMustDestroyItems( true )
{
  // Create a KFileItem from the information we have
  m_items.append( new KFileItem( _mode, m_singleUrl ) );
  init();
}

PropertiesDialog::PropertiesDialog( const QString& _tempUrl, const QString&
                                    _currentDir, const QString& _defaultName )
  : m_singleUrl( _tempUrl ), m_bMustDestroyItems( true ),
    m_defaultName( _defaultName ), m_currentDir( _currentDir )
{
  if ( m_currentDir.right(1) != "/" )
    m_currentDir.append( "/" );
  // Create the KFileItem for the _template_ file, in order to read from it.
  m_items.append( new KFileItem( -1, m_singleUrl ) );
  init();
}

void PropertiesDialog::init()
{
  pageList.setAutoDelete( true );

  // Shouldn't this be ported to KDialogBase ?
  tab = new QTabDialog( 0L, 0L );
  tab->setCaption( i18n( "Properties Dialog" ) );

  // Matthias: let the dialog look like a modal dialog
  XSetTransientForHint(qt_xdisplay(), tab->winId(), tab->winId());

  tab->setGeometry( tab->x(), tab->y(), 400, 400 );

  insertPages();

  tab->setOkButton(i18n("OK"));
  tab->setCancelButton(i18n("Cancel"));

  connect( tab, SIGNAL( applyButtonPressed() ), this, SLOT( slotApply() ) );
  connect( tab, SIGNAL( cancelButtonPressed() ), this, SLOT( slotCancel() ) );

  tab->resize(tab->sizeHint());
  tab->show();
}


PropertiesDialog::~PropertiesDialog()
{
  pageList.clear();
  // HACK
  if ( m_bMustDestroyItems ) delete m_items.first();
}

void PropertiesDialog::addPage(PropsPage *page)
{
  tab->addTab( page, page->tabName() );
  pageList.append( page );
}

bool PropertiesDialog::canDisplay( KFileItemList _items )
{
  return FilePropsPage::supports( _items ) ||
         FilePermissionsPropsPage::supports( _items ) ||
         ExecPropsPage::supports( _items ) ||
         ApplicationPropsPage::supports( _items ) ||
         BindingPropsPage::supports( _items ) ||
         URLPropsPage::supports( _items ) ||
         DevicePropsPage::supports( _items );
}

void PropertiesDialog::slotApply()
{
  PropsPage *page;
  // Apply the changes in the _normal_ order of the tabs now
  // This is because in case of renaming a file, FilePropsPage will call
  // PropertiesDialog::rename, so other tab will be ok with whatever order
  // BUT for file copied from templates, we need to do the renaming first !
  for ( page = pageList.first(); page != 0L; page = pageList.next() )
    page->applyChanges();

  emit applied();
  emit propertiesClosed();
  delete this;
}

void PropertiesDialog::slotCancel()
{
  emit canceled();
  emit propertiesClosed();
  delete this;
}

void PropertiesDialog::insertPages()
{
  if ( FilePropsPage::supports( m_items ) )
  {
    PropsPage *p = new FilePropsPage( this );
    tab->addTab( p, p->tabName() );
    pageList.append( p );
  }

  if ( FilePermissionsPropsPage::supports( m_items ) )
  {
    PropsPage *p = new FilePermissionsPropsPage( this );
    tab->addTab( p, p->tabName() );
    pageList.append( p );
  }

  if ( ExecPropsPage::supports( m_items ) )
  {
    PropsPage *p = new ExecPropsPage( this );
    tab->addTab( p, p->tabName() );
    pageList.append( p );
  }

  if ( ApplicationPropsPage::supports( m_items ) )
  {
    PropsPage *p = new ApplicationPropsPage( this );
    tab->addTab( p, p->tabName() );
    pageList.append( p );
  }

  if ( BindingPropsPage::supports( m_items ) )
  {
    PropsPage *p = new BindingPropsPage( this );
    tab->addTab( p, p->tabName() );
    pageList.append( p );
  }

  if ( URLPropsPage::supports( m_items ) )
  {
    PropsPage *p = new URLPropsPage( this );
    tab->addTab( p, p->tabName() );
    pageList.append( p );
  }

  if ( DevicePropsPage::supports( m_items ) )
  {
    PropsPage *p = new DevicePropsPage( this );
    tab->addTab( p, p->tabName() );
    pageList.append( p );
  }
}

void PropertiesDialog::rename( const QString& _name )
{
  KURL newUrl;
  // if we're creating from a template : use currentdir
  if ( !m_currentDir.isEmpty() )
    newUrl = m_currentDir + _name;
  else
  {
    QString tmpurl = m_singleUrl.url();
    if ( tmpurl.at(tmpurl.length() - 1) == '/')
      // It's a directory, so strip the trailing slash first
      tmpurl.truncate( tmpurl.length() - 1);
    newUrl = tmpurl;
    newUrl.setFileName( _name );
  }
  updateUrl( newUrl );
}

PropsPage::PropsPage( PropertiesDialog *_props ) : QWidget( _props->tabDialog(), 0L )
{
  properties = _props;
  fontHeight = 2*fontMetrics().height();
}

bool PropsPage::isDesktopFile( KFileItem * _item )
{
  // only local files
  if ( !_item->url().isLocalFile() )
    return false;

  // only regular files
  if ( !S_ISREG( _item->mode() ) )
    return false;

  QString t( _item->url().path() );

  // only if readable
  FILE *f = fopen( t, "r" );
  if ( f == 0L )
    return false;

  // return true if desktop file
  return ( _item->mimetype() == "application/x-desktop" );
}

///////////////////////////////////////////////////////////////////////////////

FilePropsPage::FilePropsPage( PropertiesDialog *_props )
  : PropsPage( _props )
{
  m_bFromTemplate = false;
  // Extract the file name only
  QString filename = properties->defaultName();
  if ( filename.isEmpty() ) // no template
    filename = properties->kurl().filename();
  else
    m_bFromTemplate = true;

  // Make it human-readable (%2F => '/', ...)
  filename = KFileItem::decodeFileName( filename );

  bool isTrash = false;
  QString path, directory;

  if ( !m_bFromTemplate ) {
    QString tmp = properties->kurl().path( 1 );
    // is it the trash bin ?
    if ( properties->kurl().isLocalFile() && tmp == KUserPaths::trashPath())
      isTrash = true;

    // Extract the full name, but without file: for local files
    if ( properties->kurl().isLocalFile() )
      path = properties->kurl().path();
    else
      path = properties->kurl().url();
    directory = properties->kurl().directory();
  } else {
    path = properties->currentDir() + properties->defaultName();
    directory = properties->currentDir();
  }

  if (ExecPropsPage::supports(properties->items()) ||
      BindingPropsPage::supports(properties->items())) {
    m_sRelativePath = "";
    // now let's make it relative
    QStringList dirs;
    if (BindingPropsPage::supports(properties->items()))
      dirs = KGlobal::dirs()->resourceDirs("mime");
    else
      dirs = KGlobal::dirs()->resourceDirs("apps");

    QStringList::ConstIterator it = dirs.begin();
    for ( ; it != dirs.end() && m_sRelativePath.isEmpty(); ++it ) {
      // might need canonicalPath() ...
      if ( path.find( *it ) == 0 ) // path is dirs + relativePath
	m_sRelativePath = path.mid( (*it).length() ); // skip appsdirs
    }
    if ( m_sRelativePath.isEmpty() )
    {
      if (BindingPropsPage::supports(properties->items()))
        kdebug(KDEBUG_WARN,1202,"Warning : editing a mimetype file out of the mimetype dirs!");
      // for Application desktop files, no problem : we can editing a .desktop file anywhere...
    } else
      while ( m_sRelativePath.left( 1 ) == '/' ) m_sRelativePath.remove( 0, 1 );
  }

  QVBoxLayout *vbl = new QVBoxLayout(this, KDialog::marginHint(),
				     KDialog::spacingHint(), "vbl");
  QGridLayout *grid = new QGridLayout(0, 3); // unknown rows
  grid->setColStretch(2, 1);
  grid->addColSpacing(1, KDialog::spacingHint());
  vbl->addLayout(grid);
  int curRow = 0;

  if (isDesktopFile(properties->item()) ||
      properties->item()->mimetype() == "inode/directory") {

    KIconLoaderButton *iconButton = new KIconLoaderButton(KGlobal::iconLoader(), this);
    iconButton->setFixedSize(50, 50);
    iconButton->setIconType("icon");
    QString iconStr;
    KMimeType::pixmapForURL(properties->kurl(),
                            properties->item()->mode(),
                            KIconLoader::Medium,
                            &iconStr);
    iconButton->setIcon(iconStr);
    iconArea = iconButton;
  } else {
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(50, 50);
    iconLabel->setPixmap(KMimeType::pixmapForURL(properties->kurl(),
                                                 properties->item()->mode(),
                                                 KIconLoader::Medium));
    iconArea = iconLabel;
  }
  grid->addWidget(iconArea, curRow, 0, AlignLeft);

  if (isTrash || filename == "/") {
    QLabel *lab = new QLabel(filename, this);
    nameArea = lab;
  } else {
    KLineEdit *lined = new KLineEdit(this);
    lined->setText(filename);
    nameArea = lined;
  }

  grid->addWidget(nameArea, curRow++, 2);
  oldName = filename;

  QLabel *l = new QLabel(this);
  l->setFrameStyle(QFrame::HLine|QFrame::Sunken);
  grid->addMultiCellWidget(l, curRow, curRow, 0, 2);
  ++curRow;

  l = new QLabel(i18n("Type:"), this);
  grid->addWidget(l, curRow, 0);

  QString tempstr = properties->item()->mimeComment();
  l = new QLabel(properties->item()->mimeComment(), this);
  grid->addWidget(l, curRow++, 2);

  l = new QLabel( i18n("Location:"), this);
  grid->addWidget(l, curRow, 0);

  l = new QLabel(this);
  l->setText( directory );
  grid->addWidget(l, curRow++, 2);

  if (S_ISREG(properties->item()->mode())) {
    l = new QLabel(i18n("Size:"), this);
    grid->addWidget(l, curRow, 0);

    int size = properties->item()->size();
    if (size > 1024*1024) {
      tempstr = i18n("%1MB ").arg(KGlobal::locale()->formatNumber(ROUND(size/(1024*1024.0)), 0));
      tempstr += i18n("(%1 bytes)").arg(KGlobal::locale()->formatNumber(size, 0));

    } else if (size > 1024) {
      tempstr = i18n("%1KB ").arg(KGlobal::locale()->formatNumber(ROUND(size/1024.0), 2));
      tempstr += i18n("(%1 bytes)").arg(KGlobal::locale()->formatNumber(size, 0));
    } else
      tempstr = i18n("%1 bytes").arg(KGlobal::locale()->formatNumber(size, 0));
    l = new QLabel(tempstr, this);
    grid->addWidget(l, curRow++, 2);
  }

  if (S_ISLNK(properties->item()->mode())) {
    l = new QLabel(i18n("Points to:"), this);
    grid->addWidget(l, curRow, 0);

    l = new QLabel(properties->item()->linkDest(), this);
    grid->addWidget(l, curRow++, 2);
  }

  l = new QLabel(this);
  l->setFrameStyle(QFrame::HLine|QFrame::Sunken);
  grid->addMultiCellWidget(l, curRow, curRow, 0, 2);
  ++curRow;

  grid = new QGridLayout(0, 3); // unknown # of rows
  grid->setColStretch(2, 1);
  grid->addColSpacing(1, KDialog::spacingHint());
  vbl->addLayout(grid);
  curRow = 0;

  l = new QLabel(i18n("Created:"), this);
  grid->addWidget(l, curRow, 0);

  tempstr = properties->item()->time(KIO::UDS_CREATION_TIME);
  l = new QLabel(tempstr, this);
  grid->addWidget(l, curRow++, 2);

  l = new QLabel(i18n("Modified:"), this);
  grid->addWidget(l, curRow, 0);

  tempstr = properties->item()->time(KIO::UDS_MODIFICATION_TIME);
  l = new QLabel(tempstr, this);
  grid->addWidget(l, curRow++, 2);

  l = new QLabel(i18n("Accessed:"), this);
  grid->addWidget(l, curRow, 0);

  tempstr = properties->item()->time(KIO::UDS_ACCESS_TIME);
  l = new QLabel(tempstr, this);
  grid->addWidget(l, curRow++, 2);

  vbl->addStretch(1);
}

bool FilePropsPage::supports( KFileItemList /*_items*/ )
{
  return true;
}

void FilePropsPage::applyChanges()
{
  QString fname = properties->kurl().filename();
  QString n;
  if (nameArea->isA("QLabel"))
    n = KFileItem::encodeFileName(((QLabel *) nameArea)->text());
  else
    n = KFileItem::encodeFileName(((QLineEdit *) nameArea)->text());

  // Do we need to rename the file ?
  if ( oldName != n || m_bFromTemplate ) { // true for any from-template file
    QString oldpath = properties->kurl().path(-1);
    // Tell properties. Warning, this changes the result of properties->kurl() !
    properties->rename( n );

    // Don't remove the template !!
    if ( !m_bFromTemplate ) {
      if ( rename( oldpath, properties->kurl().path(-1) ) != 0 )
	KMessageBox::sorry(this,
			   i18n("Could not rename file or directory\n%1\nto\n%2\n")
                           .arg(oldpath).arg(properties->kurl().path(-1)));
    }
  }

  // Save the file where we can -> usually in ~/.kde/...
  if (BindingPropsPage::supports(properties->items()) && !m_sRelativePath.isEmpty())
  {
    QString path = locateLocal("mime", m_sRelativePath);
    properties->updateUrl( KURL( path ) );
  }
  else if (ExecPropsPage::supports(properties->items()) && !m_sRelativePath.isEmpty())
  {
    QString path = locateLocal("apps", m_sRelativePath);
    properties->updateUrl( KURL( path ) );
  }

  // handle icon changes
  if (!iconArea->isA("QLabel")) {
    KIconLoaderButton *iconButton = (KIconLoaderButton *) iconArea;
    QString path;

    if (properties->item()->mimetype() == "inode/directory")
    {
      path = properties->kurl().path(1) + ".directory";
      // don't call updateUrl because the other tabs (i.e. permissions)
      // apply to the directory, not the .directory file.
    }
    else
      path = properties->kurl().path();

    QFile f( path );
    if ( !f.open( IO_ReadWrite ) ) {
      KMessageBox::sorry( 0, i18n("Could not save properties\nYou most likely do not have access to write to %1.").arg(path));
      return;
    }
    f.close();

    KDesktopFile cfg(path);
    // Get the default image
    QString str = KMimeType::findByURL( properties->kurl(),
					properties->item()->mode(),
					true )->KServiceType::icon();
    // Is it another one than the default ?
    QString sIcon;
    if ( str != iconButton->icon() )
      sIcon = iconButton->icon();
    // (otherwise write empty value)
    cfg.writeEntry( "Icon", sIcon );
    //cfg.writeEntry( "MiniIcon", sIcon ); // deprecated
    cfg.sync();

    // Force updates if that file is displayed.
    KDirWatch::self()->setFileDirty( properties->kurl().path() );
  }
}

FilePermissionsPropsPage::FilePermissionsPropsPage( PropertiesDialog *_props )
  : PropsPage( _props )
{
  QString path = properties->kurl().path(-1);
  QString fname = properties->kurl().filename();
  bool isLocal = properties->kurl().isLocalFile();

  bool IamRoot = (geteuid() == 0);
  bool isMyFile, isDir, isLink;

  isLink = properties->item()->isLink();
  isDir = properties->item()->mimetype() == "inode/directory";
  permissions = properties->item()->permissions();
  strOwner = properties->item()->user();
  strGroup = properties->item()->group();

  struct passwd *user;
  struct group *ge;
  if (isLocal) {
    struct stat buff;
    lstat( path.ascii(), &buff ); // display uid/gid of the link, if it's a link
    user = getpwuid( buff.st_uid );
    ge = getgrgid( buff.st_gid );

    isMyFile = (geteuid() == buff.st_uid);
    if ( user != 0L )
      strOwner = user->pw_name;


    if ( ge != 0L ) {
      strGroup = ge->gr_name;
      if (strGroup.isEmpty())
        strGroup.sprintf("%d",ge->gr_gid);
    } else
      strGroup.sprintf("%d",buff.st_gid);
  } else {
    isMyFile = false; // we have to assume remote files aren't ours.
  }

  QBoxLayout *box = new QVBoxLayout( this, KDialog::spacingHint() );

  QLabel *l;
  QGroupBox *gb;
  QGridLayout *gl;

  /* Group: Access Permissions */
  gb = new QGroupBox ( i18n("Access permissions"), this );
  box->addWidget (gb);

  gl = new QGridLayout (gb, 4, 6, 15);

  if (isDir)
    l = new QLabel( i18n("Show\nEntries"), gb );
  else
    l = new QLabel( i18n("Read"), gb );
  gl->addWidget (l, 0, 1);

  if (isDir)
    l = new QLabel( i18n("Write\nEntries"), gb );
  else
    l = new QLabel( i18n("Write"), gb );
  gl->addWidget (l, 0, 2);

  if (isDir)
    l = new QLabel( i18n("Enter"), gb );
  else
    l = new QLabel( i18n("Exec"), gb );
  gl->addWidget (l, 0, 3);

  l = new QLabel( i18n("Special"), gb );
  gl->addWidget (l, 0, 4);

  l = new QLabel( i18n("User"), gb );
  l->setEnabled (IamRoot || isMyFile);
  gl->addWidget (l, 1, 0);

  l = new QLabel( i18n("Group"), gb );
  gl->addWidget (l, 2, 0);

  l = new QLabel( i18n("Others"), gb );
  gl->addWidget (l, 3, 0);

    /* Draw Checkboxes */
  for (int row = 0; row < 3 ; ++row) {
    for (int col = 0; col < 4; ++col) {
      QCheckBox *cb;
      QString desc;
	
      /* some boxes need further description .. */
      switch (fperm[row][col]) {
	case S_ISUID : desc = i18n("Set UID"); break;
	case S_ISGID : desc = i18n("Set GID"); break;
	case S_ISVTX : desc = i18n("Sticky"); break;
	default      : desc = "";
      }
	
      cb = new QCheckBox (desc, gb);
      cb->setChecked ((permissions & fperm[row][col])
                      == fperm[row][col]);
      cb->setEnabled ((isMyFile || IamRoot) && (!isLink));
      permBox[row][col] = cb;
      gl->addWidget (permBox[row][col], row+1, col+1);
    }
  }
  gl->setColStretch(5, 10);

    /**** Group: Ownership ****/
  gb = new QGroupBox ( i18n("Ownership"), this );
  box->addWidget (gb);

  gl = new QGridLayout (gb, 4, 3, 15);

  /*** Set Owner ***/
  l = new QLabel( i18n("Owner"), gb );
  gl->addWidget (l, 1, 0);

    /* maybe this should be a combo-box, but this isn't handy
     * if there are 2000 Users (tiny scrollbar!)
     * anyone developing a combo-box with incremental search .. ?
     */
  owner = new KLineEdit( gb );
  gl->addWidget (owner, 1, 1);
  owner->setText( strOwner );
  owner->setEnabled ( IamRoot && isLocal ); // we can just change the user if we're root

    /*** Set Group ***/
    /* get possible groups .. */
  QStrList *groupList = new QStrList (true);

  if (isMyFile || IamRoot) {  // build list just if we have to
    setgrent();
    while ((ge = getgrent()) != 0L) {
      if (IamRoot)
        groupList->inSort (ge->gr_name);
      else {
        /* pick just the groups the user can change the file to */
        char ** members = ge->gr_mem;
        char * member;
        while ((member = *members) != 0L) {
          if (member == strOwner) {
            groupList->inSort (ge->gr_name);
            break;
          }
          ++members;
        }
      }
    }
    endgrent();

    /* add the effective Group to the list .. */
    ge = getgrgid (getegid());
    if (ge) {
      QString name = ge->gr_name;
      if (name.isEmpty())
        name.sprintf("%d",ge->gr_gid);
      if (groupList->find (name) < 0)
        groupList->inSort (name);
    }
  }

  /* add the group the file currently belongs to ..
     * .. if its not there already
     */
  if (groupList->find (strGroup) < 0)
    groupList->inSort (strGroup);

  l = new QLabel( i18n("Group"), gb );
  gl->addWidget (l, 2, 0);

  if (groupList->count() > 1 &&
      (IamRoot || isMyFile)) {
    /* Combo-Box .. if there is anything to change */
    if (groupList->count() <= 10)
      /* Motif-Style looks _much_ nicer for few items */
      grp = new QComboBox( gb );
    else
      grp = new QComboBox( false, gb );

    grp->insertStrList ( groupList );
    grp->setCurrentItem (groupList->find ( strGroup ));
    gl->addWidget (grp, 2, 1);
    grp->setEnabled (IamRoot || isMyFile);
  }
  else {
    // no choice; just present the group in a disabled edit-field
    QLineEdit *e = new KLineEdit ( gb );
    e->setText ( strGroup );
    e->setEnabled (false);
    gl->addWidget (e, 2, 1);
    grp = 0L;
  }

  delete groupList;

  gl->setColStretch(2, 10);

  box->addStretch (10);
}

bool FilePermissionsPropsPage::supports( KFileItemList /*_items*/ )
{
  return true;
}

void FilePermissionsPropsPage::applyChanges()
{
  QString path = properties->kurl().path();
  QString fname = properties->kurl().filename();

  mode_t p = 0L;
  for (int row = 0;row < 3; ++row)
    for (int col = 0; col < 4; ++col)
      if (permBox[row][col]->isChecked())
	p |= fperm[row][col];

  // First update group / owner
  // (permissions have to set after, in case of suid and sgid)
  if ( owner->text() != strOwner ||
       (grp && grp->text(grp->currentItem()) != strGroup )) {
    struct passwd* pw = getpwnam( owner->text() );
    struct group* g;
    if (grp)
      g = getgrnam( grp->text(grp->currentItem()) );
    else
      g = 0L;

    if ( pw == 0L ) {
      kdebug(KDEBUG_ERROR,1202," ERROR: No user %s",(const char*)owner->text());
      return;
    }
    if ( g == 0L ) {
      kdebug(KDEBUG_ERROR,1202," ERROR: No group %s",
             (const char*)grp->text(grp->currentItem()) ); // should never happen
      return;
    }
    if ( chown( path, pw->pw_uid, g->gr_gid ) != 0 )
      KMessageBox::sorry( 0, i18n( "Could not change owner/group\nPerhaps access denied." ));
  }

  debug("permissions : %d", permissions);
  debug("p : %d", p);
  debug("path : %s", path.ascii());
  if ( permissions != p )
  {
    if ( chmod( path, p ) != 0 )
      KMessageBox::sorry( 0, i18n("Could not change permissions. \nYou most likely do not have access to write to %1.").arg(path));
    else
    {
      // Force refreshing information about that file if displayed.
      KDirWatch::self()->setFileDirty( path );
    }
  }

}

ExecPropsPage::ExecPropsPage( PropertiesDialog *_props )
  : PropsPage( _props )
{
  QVBoxLayout * mainlayout = new QVBoxLayout(this, KDialog::spacingHint());

  // Now the widgets in the top layout

  QLabel* l;
  l = new QLabel( this, "Label_1" );
  l->setText( i18n("Program Name:") );
  mainlayout->addWidget(l, 1);

  QHBoxLayout * hlayout;
  hlayout = new QHBoxLayout(KDialog::spacingHint());
  mainlayout->addLayout(hlayout);

  execEdit = new KLineEdit( this, "LineEdit_1" );
  hlayout->addWidget(execEdit, 1);

  execBrowse = new QPushButton( this, "Button_1" );
  execBrowse->setText( i18n("&Browse...") );
  hlayout->addWidget(execBrowse);

  hlayout->addSpacing(KDialog::spacingHint());

  // The groupbox about swallowing
  QGroupBox* tmpQGroupBox;
  tmpQGroupBox = new QGroupBox( i18n("Panel Embedding"), this, "GroupBox_1" );
  tmpQGroupBox->setFrameStyle(49);
  mainlayout->addWidget(tmpQGroupBox, 2); // 2 vertical items

  QVBoxLayout * grouplayout;
  grouplayout = new QVBoxLayout(tmpQGroupBox, KDialog::spacingHint());
  grouplayout->addSpacing( fontMetrics().height() );

  QGridLayout *grid = new QGridLayout(KDialog::spacingHint(), 0, 2);
  grid->setColStretch(1, 1);
  grouplayout->addLayout(grid);

  l = new QLabel( tmpQGroupBox, "Label_6" );
  l->setText( i18n("Execute on click:") );
  grid->addWidget(l, 0, 0);

  swallowExecEdit = new KLineEdit( tmpQGroupBox, "LineEdit_3" );
  grid->addWidget(swallowExecEdit, 0, 1);

  l = new QLabel( tmpQGroupBox, "Label_7" );
  l->setText( i18n("Window Title:") );
  grid->addWidget(l, 1, 0);

  swallowTitleEdit = new KLineEdit( tmpQGroupBox, "LineEdit_4" );
  grid->addWidget(swallowTitleEdit, 1, 1);

  // The groupbox about run in terminal

  tmpQGroupBox = new QGroupBox( this, "GroupBox_2" );
  tmpQGroupBox->setFrameStyle( 49 );
  tmpQGroupBox->setAlignment( 1 );
  mainlayout->addWidget(tmpQGroupBox, 2);  // 2 vertical items -> stretch = 2

  grouplayout = new QVBoxLayout(tmpQGroupBox, KDialog::spacingHint());

  terminalCheck = new QCheckBox( tmpQGroupBox, "RadioButton_3" );
  terminalCheck->setText( i18n("Run in terminal") );
  grouplayout->addWidget(terminalCheck, 0);

  hlayout = new QHBoxLayout(KDialog::spacingHint());
  grouplayout->addLayout(hlayout, 1);

  l = new QLabel( tmpQGroupBox, "Label_5" );
  l->setText( i18n("Terminal Options") );
  hlayout->addWidget(l, 0);

  terminalEdit = new KLineEdit( tmpQGroupBox, "LineEdit_5" );
  hlayout->addWidget(terminalEdit, 1);

  mainlayout->addStretch(2);


  // now populate the page
  QString path = _props->kurl().path();
  QFile f( path );
  if ( !f.open( IO_ReadOnly ) )
    return;
  f.close();

  KSimpleConfig config( path );
  config.setDollarExpansion( false );
  config.setDesktopGroup();
  execStr = config.readEntry( "Exec" );
  swallowExecStr = config.readEntry( "SwallowExec" );
  swallowTitleStr = config.readEntry( "SwallowTitle");
  termStr = config.readEntry( "Terminal" );
  termOptionsStr = config.readEntry( "TerminalOptions" );

  if ( !swallowExecStr.isNull() )
    swallowExecEdit->setText( swallowExecStr );
  if ( !swallowTitleStr.isNull() )
    swallowTitleEdit->setText( swallowTitleStr );

  if ( !execStr.isNull() )
    execEdit->setText( execStr );
  if ( !termOptionsStr.isNull() )
    terminalEdit->setText( termOptionsStr );

  if ( !termStr.isNull() )
    terminalCheck->setChecked( termStr == "1" );
  enableCheckedEdit();

  connect( execBrowse, SIGNAL( clicked() ), this, SLOT( slotBrowseExec() ) );
  connect( terminalCheck, SIGNAL( clicked() ), this,  SLOT( enableCheckedEdit() ) );

}


void ExecPropsPage::enableCheckedEdit()
{
  terminalEdit->setEnabled(terminalCheck->isChecked());
}


bool ExecPropsPage::supports( KFileItemList _items )
{
  KFileItem * item = _items.first();
  // check if desktop file
  if ( !PropsPage::isDesktopFile( item ) )
    return false;
  // open file and check type
  KDesktopFile config( item->url().path(), true /* readonly */ );
  return config.hasApplicationType();
}

void ExecPropsPage::applyChanges()
{
  QString path = properties->kurl().path();

  QFile f( path );

  if ( !f.open( IO_ReadWrite ) ) {
    KMessageBox::sorry( 0, i18n("Could not save properties\nYou most likely do not have access to write to %1.").arg(path));
    return;
  }
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  config.writeEntry( "Type", "Application");
  config.writeEntry( "Exec", execEdit->text() );
  config.writeEntry( "SwallowExec", swallowExecEdit->text() );
  config.writeEntry( "SwallowTitle", swallowTitleEdit->text() );
  config.writeEntry( "Terminal", terminalCheck->isChecked() ? "1" : "0" );
  config.writeEntry( "TerminalOptions", terminalEdit->text() );
}


void ExecPropsPage::slotBrowseExec()
{
  QString f = KFileDialog::getOpenFileName( QString::null,
					    QString::null, this );
  if ( f.isNull() )
    return;

  execEdit->setText( f );
}

URLPropsPage::URLPropsPage( PropertiesDialog *_props )
  : PropsPage( _props )
{
  QVBoxLayout * layout = new QVBoxLayout(this, KDialog::spacingHint());

  QLabel *l;
  l = new QLabel( this, "Label_1" );
  l->setText( i18n("URL:") );
  layout->addWidget(l);

  URLEdit = new KLineEdit( this, "LineEdit_1" );
  layout->addWidget(URLEdit);

  QString path = properties->kurl().path();

  QFile f( path );
  if ( !f.open( IO_ReadOnly ) )
    return;
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  URLStr = config.readEntry(  "URL" );

  if ( !URLStr.isNull() )
    URLEdit->setText( URLStr );

  layout->addStretch (1);
}

bool URLPropsPage::supports( KFileItemList _items )
{
  KFileItem * item = _items.first();
  // check if desktop file
  if ( !PropsPage::isDesktopFile( item ) )
    return false;

  // open file and check type
  KDesktopFile config( item->url().path(), true /* readonly */ );
  return config.hasLinkType();
}

void URLPropsPage::applyChanges()
{
  QString path = properties->kurl().path();

  QFile f( path );
  if ( !f.open( IO_ReadWrite ) ) {
    KMessageBox::sorry( 0, i18n("Could not save properties\nYou most likely do not have access to write to %1.").arg(path));
    return;
  }
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  config.writeEntry( "Type", "Link");
  config.writeEntry( "URL", URLEdit->text() );
}

/* ----------------------------------------------------
 *
 * ApplicationPropsPage
 *
 * -------------------------------------------------- */

ApplicationPropsPage::ApplicationPropsPage( PropertiesDialog *_props )
  : PropsPage( _props )
{
  QVBoxLayout *toplayout = new QVBoxLayout(this, KDialog::spacingHint());

  availableExtensionsList = new QListBox( this );
  addExtensionButton = new QPushButton( "<<", this );
  delExtensionButton = new QPushButton( ">>", this );

  QLabel *l;

  QGridLayout *grid = new QGridLayout(2, 2);
  grid->setColStretch(1, 1);
  toplayout->addLayout(grid);

  l = new QLabel(i18n("Name:"),  this, "Label_4" );
  grid->addWidget(l, 0, 0);

  nameEdit = new KLineEdit( this, "LineEdit_3" );
  grid->addWidget(nameEdit, 0, 1);

  l = new QLabel(i18n("Comment:"),  this, "Label_3" );
  grid->addWidget(l, 1, 0);

  commentEdit = new KLineEdit( this, "LineEdit_2" );
  grid->addWidget(commentEdit, 1, 1);

  l = new QLabel(i18n("File types:"), this);
  toplayout->addWidget(l, 0, AlignLeft);

  grid = new QGridLayout(4, 3);
  grid->setColStretch(0, 1);
  grid->setColStretch(2, 1);
  toplayout->addLayout(grid, 2);

  extensionsList = new QListBox( this );
  grid->addMultiCellWidget(extensionsList, 0, 3, 0, 0);

  grid->addWidget(addExtensionButton, 1, 1);
  connect( addExtensionButton, SIGNAL( pressed() ),
	   this, SLOT( slotAddExtension() ) );
  grid->addWidget(delExtensionButton, 2, 1);
  connect( delExtensionButton, SIGNAL( pressed() ),
	   this, SLOT( slotDelExtension() ) );
  grid->addMultiCellWidget(availableExtensionsList, 0, 3, 2, 2);

  QString path = properties->kurl().path() ;
  QFile f( path );
  if ( !f.open( IO_ReadOnly ) )
    return;
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  commentStr = config.readEntry( "Comment" );
  extensions = config.readListEntry( "MimeType" );
  nameStr = config.readEntry( "Name" );
  // Use the file name if no name is specified
  if ( nameStr.isEmpty() ) {
    nameStr = _props->kurl().filename();
    if ( nameStr.right(8) == ".desktop" )
      nameStr.truncate( nameStr.length() - 8 );
    if ( nameStr.right(7) == ".kdelnk" )
      nameStr.truncate( nameStr.length() - 7 );
    //KURL::decodeURL( nameStr );
  }

  if ( !commentStr.isNull() )
    commentEdit->setText( commentStr );
  if ( !nameStr.isNull() )
    nameEdit->setText( nameStr );
  QStringList::Iterator sit = extensions.begin();
  for( ; sit != extensions.end(); ++sit )
    extensionsList->inSort( *sit );

  addMimeType( "all" );
  addMimeType( "alldirs" );
  addMimeType( "allfiles" );

  KMimeType::List mimeTypes = KMimeType::allMimeTypes();
  QValueListIterator<KMimeType::Ptr> it2 = mimeTypes.begin();
  for ( ; it2 != mimeTypes.end(); ++it2 )
    addMimeType ( (*it2)->name() );
}

void ApplicationPropsPage::addMimeType( const char * name )
{
  // Add a mimetype to the list of available mime types if not in the extensionsList

  bool insert = true;

  for ( uint i = 0; i < extensionsList->count(); i++ )
    if ( strcmp( name, extensionsList->text( i ) ) == 0 )
      insert = false;

  if ( insert )
    availableExtensionsList->inSort( name );
}

bool ApplicationPropsPage::supports( KFileItemList _items )
{
  // same constraints as ExecPropsPage : desktop file with Type = Application
  return ExecPropsPage::supports( _items );
}

void ApplicationPropsPage::applyChanges()
{
  QString path = properties->kurl().path();

  QFile f( path );

  if ( !f.open( IO_ReadWrite ) ) {
    KMessageBox::sorry( 0, i18n("Could not save properties\nYou most likely do not have access to write to %1.").arg(path));
    return;
  }
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  config.writeEntry( "Type", "Application");
  config.writeEntry( "Comment", commentEdit->text(), true, false, true );

  extensions.clear();
  for ( uint i = 0; i < extensionsList->count(); i++ )
    extensions.append( extensionsList->text( i ) );

  config.writeEntry( "MimeType", extensions );
  config.writeEntry( "Name", nameEdit->text(), true, false, true );

  config.sync();
  f.close();
}

void ApplicationPropsPage::slotAddExtension()
{
  int pos = availableExtensionsList->currentItem();

  if ( pos == -1 )
    return;

  extensionsList->inSort( availableExtensionsList->text( pos ) );
  availableExtensionsList->removeItem( pos );
}

void ApplicationPropsPage::slotDelExtension()
{
  int pos = extensionsList->currentItem();

  if ( pos == -1 )
    return;

  availableExtensionsList->inSort( extensionsList->text( pos ) );
  extensionsList->removeItem( pos );
}

/* ----------------------------------------------------
 *
 * BindingPropsPage
 *
 * -------------------------------------------------- */

BindingPropsPage::BindingPropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
  patternEdit = new KLineEdit( this, "LineEdit_1" );
  commentEdit = new KLineEdit( this, "LineEdit_2" );
  mimeEdit = new KLineEdit( this, "LineEdit_3" );

  QBoxLayout * mainlayout = new QVBoxLayout(this, KDialog::spacingHint());
  QLabel* tmpQLabel;

  tmpQLabel = new QLabel( this, "Label_1" );
  tmpQLabel->setText(  i18n("Pattern ( example: *.html;*.HTML; )") );
  tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
  mainlayout->addWidget(tmpQLabel, 1);

  patternEdit->setGeometry( 10, 40, 210, 30 );
  patternEdit->setText( "" );
  patternEdit->setMaxLength( 512 );
  patternEdit->setMinimumSize( patternEdit->sizeHint() );
  patternEdit->setFixedHeight( fontHeight );
  mainlayout->addWidget(patternEdit, 1);

  tmpQLabel = new QLabel( this, "Label_2" );
  tmpQLabel->setText(  i18n("Mime Type") );
  tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
  mainlayout->addWidget(tmpQLabel, 1);

  mimeEdit->setGeometry( 10, 160, 210, 30 );
  mimeEdit->setMaxLength( 256 );
  mimeEdit->setMinimumSize( mimeEdit->sizeHint() );
  mimeEdit->setFixedHeight( fontHeight );
  mainlayout->addWidget(mimeEdit, 1);

  tmpQLabel = new QLabel( this, "Label_3" );
  tmpQLabel->setText(  i18n("Comment") );
  tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
  mainlayout->addWidget(tmpQLabel, 1);

  commentEdit->setGeometry( 10, 100, 210, 30 );
  commentEdit->setMaxLength( 256 );
  commentEdit->setMinimumSize( commentEdit->sizeHint() );
  commentEdit->setFixedHeight( fontHeight );
  mainlayout->addWidget(commentEdit, 1);

  QHBoxLayout * hlayout = new QHBoxLayout(KDialog::spacingHint());
  mainlayout->addLayout(hlayout, 2); // double stretch, because two items

  QVBoxLayout * vlayout; // a vertical layout for the two following items
  vlayout = new QVBoxLayout(KDialog::spacingHint());
  hlayout->addLayout(vlayout, 1);

  mainlayout->addStretch (10);
  mainlayout->activate();

  QFile f( _props->kurl().path() );
  if ( !f.open( IO_ReadOnly ) )
    return;
  f.close();

  KSimpleConfig config( _props->kurl().path() );
  config.setDesktopGroup();
  QString patternStr = config.readEntry( "Patterns" );
  QString iconStr = config.readEntry( "Icon" );
  QString commentStr = config.readEntry( "Comment" );
  m_sMimeStr = config.readEntry( "MimeType" );

  if ( !patternStr.isEmpty() )
    patternEdit->setText( patternStr );
  if ( !commentStr.isEmpty() )
    commentEdit->setText( commentStr );
  if ( !m_sMimeStr.isEmpty() )
    mimeEdit->setText( m_sMimeStr );

}

bool BindingPropsPage::supports( KFileItemList _items )
{
  KFileItem * item = _items.first();
  // check if desktop file
  if ( !PropsPage::isDesktopFile( item ) )
    return false;

  // open file and check type
  KDesktopFile config( item->url().path(), true /* readonly */ );
  return config.hasMimeTypeType();
}

void BindingPropsPage::applyChanges()
{
  QString path = properties->kurl().path();
  QFile f( path );

  if ( !f.open( IO_ReadWrite ) )
  {
    KMessageBox::sorry( 0, i18n("Could not save properties\nYou most likely do not have access to write to %1.").arg(path));
    return;
  }
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  config.writeEntry( "Type", "MimeType" );

  QString tmp = patternEdit->text();
  if ( tmp.length() > 1 )
    if ( tmp.at(tmp.length() - 1) != ';' )
      tmp += ';';
  config.writeEntry( "Patterns", tmp );
  config.writeEntry( "Comment", commentEdit->text(), true, false, true );
  config.writeEntry( "MimeType", mimeEdit->text() );
  config.sync();
}

/* ----------------------------------------------------
 *
 * DevicePropsPage
 *
 * -------------------------------------------------- */

DevicePropsPage::DevicePropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
  IamRoot = (geteuid() == 0);

  QStringList devices;
  QString fstabFile;
  indexDevice = 0;  // device on first column
  indexMountPoint = 1; // mount point on second column
  indexFSType = 2; // fstype on third column
  if ( QFile::exists("/etc/fstab") ) // Linux, ...
  {
    fstabFile = "/etc/fstab";
  }
  else if ( QFile::exists("/etc/vfstab") ) // Solaris
  {
    fstabFile = "/etc/vfstab";
    indexMountPoint++;
    indexFSType++;
  }

  // insert your favorite location for fstab here
  if ( !fstabFile.isEmpty() )
  {
    QFile f( fstabFile );
    f.open( IO_ReadOnly );
    QTextStream stream( &f );
    while ( !stream.eof() )
    {
      QString line = stream.readLine();
      line = line.simplifyWhiteSpace();
      if (!line.isEmpty() && line[0] == '/') // skip comments but also
      {
        QStringList lst = QStringList::split( ' ', line );
        if ( lst.count() > 2 && lst[indexDevice] != "/proc"
            && lst[indexMountPoint] != "none" && lst[indexMountPoint] != "-" )
        {
          devices.append( lst[indexDevice]+" ("+lst[indexMountPoint]+")" );
          m_devicelist.append( line );
        }
      }
    }
    f.close();
  }


  QGridLayout *layout = new QGridLayout(this, 0, 3, KDialog::marginHint(),
					KDialog::spacingHint());
  layout->setColStretch(1, 1);

  QLabel* label;
  label = new QLabel( this );
  label->setText( devices.count() == 0 ?
                      i18n("Device (/dev/fd0):") : // old style
                      i18n("Device:") ); // new style (combobox)
  layout->addWidget(label, 0, 0);

  device = new QComboBox( true, this, "ComboBox_device" );
  device->insertStringList( devices );
  layout->addWidget(device, 0, 1);
  connect( device, SIGNAL( activated( int ) ),
           this, SLOT( slotActivated( int ) ) );

  readonly = new QCheckBox( this, "CheckBox_readonly" );
  readonly->setText(  i18n("Read Only") );
  layout->addWidget(readonly, 1, 1);
  if ( !IamRoot )
    readonly->setEnabled( false );

  label = new QLabel( this );
  label->setText( devices.count()==0 ?
                      i18n("Mount Point (/mnt/floppy):") : // old style
                      i18n("Mount Point:")); // new style (combobox)
  layout->addWidget(label, 2, 0);

  mountpoint = new KLineEdit( this, "LineEdit_mountpoint" );
  layout->addWidget(mountpoint, 2, 1);
  if ( !IamRoot )
    mountpoint->setEnabled( false );

  label = new QLabel( this );
  label->setText(  i18n("File System Type:") );
  layout->addWidget(label, 3, 0);

  fstype = new KLineEdit( this, "LineEdit_fstype" );
  layout->addWidget(fstype, 3, 1);
  if ( !IamRoot )
    fstype->setEnabled( false );

  QFrame *frame = new QFrame(this);
  frame->setFrameStyle(QFrame::HLine|QFrame::Sunken);
  layout->addMultiCellWidget(frame, 4, 4, 0, 2);


  unmounted = new KIconLoaderButton( KGlobal::iconLoader(), this );
  unmounted->setFixedSize(50, 50);
  unmounted->setIconType("icon"); // Choose from app icons
  layout->addWidget(unmounted, 5, 0);

  label = new QLabel( i18n("Unmounted Icon"),  this );
  layout->addWidget(label, 5, 1);

  layout->setRowStretch(6, 1);

  QString path( _props->kurl().path() );

  QFile f( path );
  if ( !f.open( IO_ReadOnly ) )
    return;
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  QString deviceStr = config.readEntry( "Dev" );
  QString mountPointStr = config.readEntry( "MountPoint" );
  bool ro = config.readBoolEntry( "ReadOnly", false );
  QString fstypeStr = config.readEntry( "FSType" );
  QString unmountedStr = config.readEntry( "UnmountIcon" );

  device->setEditText( deviceStr );
  if ( !deviceStr.isEmpty() ) {
    // Set default options for this device (first matching entry)
    int index = 0;
    for ( QStringList::Iterator it = m_devicelist.begin();
          it != m_devicelist.end(); ++it, ++index ) {
      // WARNING : this works only if indexDevice == 0
      if ( (*it).left( deviceStr.length() ) == deviceStr ) {
        //qDebug( "found it %d", index );
        slotActivated( index );
        break;
      }
    }
  }

  if ( !mountPointStr.isEmpty() )
    mountpoint->setText( mountPointStr );

  if ( !fstypeStr.isEmpty() )
    fstype->setText( fstypeStr );

  readonly->setChecked( ro );

  if ( unmountedStr.isEmpty() )
    unmountedStr = KMimeType::mimeType("")->KServiceType::icon(); // default icon

  unmounted->setIcon( unmountedStr );
}

void DevicePropsPage::slotActivated( int index )
{
  QStringList lst = QStringList::split( ' ', m_devicelist[index] );
  device->setEditText( lst[indexDevice] );
  mountpoint->setText( lst[indexMountPoint] );
  fstype->setText( lst[indexFSType] );
}

bool DevicePropsPage::supports( KFileItemList _items )
{
  KFileItem * item = _items.first();
  // check if desktop file
  if ( !PropsPage::isDesktopFile( item ) )
    return false;
  // open file and check type
  KDesktopFile config( item->url().path(), true /* readonly */ );
  return config.hasDeviceType();
}

void DevicePropsPage::applyChanges()
{
  QString path = properties->kurl().path();
  QFile f( path );
  if ( !f.open( IO_ReadWrite ) )
  {
    KMessageBox::sorry( 0, i18n("Could not save properties\nYou most likely do not have access to write to %1.").arg(path));
    return;
  }
  f.close();

  KSimpleConfig config( path );
  config.setDesktopGroup();
  config.writeEntry( "Type", "FSDevice" );

  config.writeEntry( "Dev", device->currentText() );
  if ( IamRoot )
  {
    config.writeEntry( "MountPoint", mountpoint->text() );
    config.writeEntry( "FSType", fstype->text() );
  }

  config.writeEntry( "UnmountIcon", unmounted->icon() );

  config.writeEntry( "ReadOnly", readonly->isChecked() );

  config.sync();
}

#include "kpropsdlg.moc"
