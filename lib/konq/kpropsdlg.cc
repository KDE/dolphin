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
#include <assert.h>

#include <qfile.h>
#include <qdir.h>
#include <qdict.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qstrlist.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qcombobox.h>

#include <kdialog.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kiconloaderdialog.h>
#include <kurl.h>
#include <klocale.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kstddirs.h>
#include <kio/job.h>
#include <kio/renamedlg.h>
#include <kfiledialog.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <konqfileitem.h>
#include <kuserpaths.h>
#include <kglobal.h>
#include <kcompletion.h>
#include <klineedit.h>

#include "kpropsdlg.h"

#include <X11/Xlib.h> // for XSetTransientForHint

template class QList<KonqFileItem>;

#define ROUND(x) ((int)(0.5 + (x)))

mode_t FilePermissionsPropsPage::fperm[3][4] = {
        {S_IRUSR, S_IWUSR, S_IXUSR, S_ISUID},
        {S_IRGRP, S_IWGRP, S_IXGRP, S_ISGID},
        {S_IROTH, S_IWOTH, S_IXOTH, S_ISVTX}
    };

template class QList<PropsPage>;

PropertiesDialog::PropertiesDialog( KonqFileItemList _items ) :
  m_singleUrl( _items.first()->url() ), m_items( _items ),
  m_bMustDestroyItems( false )
{
  assert( !_items.isEmpty() );
  assert(!m_singleUrl.isEmpty());
  init();
}

PropertiesDialog::PropertiesDialog( const KURL& _url, mode_t _mode ) :
  m_singleUrl( _url ), m_bMustDestroyItems( true )
{
  assert(!_url.isEmpty());
  // Create a KonqFileItem from the information we have
  m_items.append( new KonqFileItem( _mode, -1, m_singleUrl ) );
  init();
}

PropertiesDialog::PropertiesDialog( const KURL& _tempUrl, const KURL& _currentDir,
                                    const QString& _defaultName )
  : m_singleUrl( _tempUrl ), m_bMustDestroyItems( true ),
    m_defaultName( _defaultName ), m_currentDir( _currentDir )
{
  assert(!m_singleUrl.isEmpty());
  // Create the KonqFileItem for the _template_ file, in order to read from it.
  m_items.append( new KonqFileItem( -1, -1, m_singleUrl ) );
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

bool PropertiesDialog::canDisplay( KonqFileItemList _items )
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

void PropertiesDialog::updateUrl( const KURL& _newUrl )
{
  m_singleUrl = _newUrl;
  assert(!m_singleUrl.isEmpty());
}

void PropertiesDialog::rename( const QString& _name )
{
  KURL newUrl;
  // if we're creating from a template : use currentdir
  if ( !m_currentDir.isEmpty() )
  {
    newUrl = m_currentDir;
    newUrl.addPath( _name );
  }
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

bool PropsPage::isDesktopFile( KonqFileItem * _item )
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
  filename = KIO::decodeFileName( filename );

  KonqFileItem * item = properties->item();
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
    path = properties->currentDir().path(1) + properties->defaultName();
    directory = properties->currentDir().url();
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
        kDebugWarning(1202,"Warning : editing a mimetype file out of the mimetype dirs!");
      // for Application desktop files, no problem : we can editing a .desktop file anywhere...
    } else
      while ( m_sRelativePath.left( 1 ).at(0) == '/' ) m_sRelativePath.remove( 0, 1 );
  }

  QVBoxLayout *vbl = new QVBoxLayout(this, KDialog::marginHint(),
				     KDialog::spacingHint(), "vbl");
  QGridLayout *grid = new QGridLayout(0, 3); // unknown rows
  grid->setColStretch(2, 1);
  grid->addColSpacing(1, KDialog::spacingHint());
  vbl->addLayout(grid);
  int curRow = 0;

  bool bDesktopFile = isDesktopFile(item);
  if (bDesktopFile || S_ISDIR( item->mode())) {

    KIconLoaderButton *iconButton = new KIconLoaderButton(KGlobal::iconLoader(), this);
    iconButton->setFixedSize(50, 50);
    iconButton->setIconType("apps");
    // This works for everything except Device icons on unmounted devices
    // So we have to really open .desktop files
    QString iconStr = KMimeType::findByURL( properties->kurl(),
					item->mode(),
					true )->icon( properties->kurl(),
                                                      properties->kurl().isLocalFile() );
    if ( bDesktopFile )
    {
      KSimpleConfig config( path );
      config.setDesktopGroup();
      iconStr = config.readEntry( "Icon" );
    }
    iconButton->setIcon(iconStr);
    iconArea = iconButton;
  } else {
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(50, 50);
    iconLabel->setPixmap(KMimeType::pixmapForURL(properties->kurl(),
                                                 item->mode(),
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
    lined->setFocus();
  }

  grid->addWidget(nameArea, curRow++, 2);
  oldName = filename;

  QLabel *l = new QLabel(this);
  l->setFrameStyle(QFrame::HLine|QFrame::Sunken);
  grid->addMultiCellWidget(l, curRow, curRow, 0, 2);
  ++curRow;

  l = new QLabel(i18n("Type:"), this);
  grid->addWidget(l, curRow, 0);

  QString tempstr = item->mimeComment();
  l = new QLabel(tempstr, this);
  grid->addWidget(l, curRow++, 2);

  l = new QLabel( i18n("Location:"), this);
  grid->addWidget(l, curRow, 0);

  l = new QLabel(this);
  l->setText( directory );
  grid->addWidget(l, curRow++, 2);

  if (S_ISREG(item->mode())) {
    l = new QLabel(i18n("Size:"), this);
    grid->addWidget(l, curRow, 0);

    // Should we use KIO::convertSize ? Seems less accurate...
    int size = item->size();
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

  if (item->isLink()) {
    l = new QLabel(i18n("Points to:"), this);
    grid->addWidget(l, curRow, 0);

    l = new QLabel(item->linkDest(), this);
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

  tempstr = item->time(KIO::UDS_CREATION_TIME);
  l = new QLabel(tempstr, this);
  grid->addWidget(l, curRow++, 2);

  l = new QLabel(i18n("Modified:"), this);
  grid->addWidget(l, curRow, 0);

  tempstr = item->time(KIO::UDS_MODIFICATION_TIME);
  l = new QLabel(tempstr, this);
  grid->addWidget(l, curRow++, 2);

  l = new QLabel(i18n("Accessed:"), this);
  grid->addWidget(l, curRow, 0);

  tempstr = item->time(KIO::UDS_ACCESS_TIME);
  l = new QLabel(tempstr, this);
  grid->addWidget(l, curRow++, 2);

  vbl->addStretch(1);
}

bool FilePropsPage::supports( KonqFileItemList /*_items*/ )
{
  return true;
}

void FilePropsPage::applyChanges()
{
  QString fname = properties->kurl().filename();
  QString n;
  if (nameArea->isA("QLabel"))
    n = KIO::encodeFileName(((QLabel *) nameArea)->text());
  else
    n = KIO::encodeFileName(((QLineEdit *) nameArea)->text());

  KIO::Job * job = 0L;
  // Do we need to rename the file ?
  if ( oldName != n || m_bFromTemplate ) { // true for any from-template file
    KURL oldurl = properties->kurl();
    // Tell properties. Warning, this changes the result of properties->kurl() !
    properties->rename( n );

    // Don't remove the template !!
    if ( !m_bFromTemplate ) { // (normal renaming)
        job = KIO::move( oldurl, properties->kurl() );
        connect( job, SIGNAL( result( KIO::Job * ) ),
                 SLOT( slotRenameFinished( KIO::Job * ) ) );
        kDebugInfo(1202,"oldpath = %s",oldurl.url().ascii());
        kDebugInfo(1202,"newpath = %s",properties->kurl().url().ascii());
    } else // (writing stuff from a template)
    {
        bool bOk;
        do {
            // No support for remote urls
            // We could, but first I'd like to see WHO needs that.
            if ( !properties->kurl().isLocalFile() )
            {
                KMessageBox::error( 0, i18n( "Can't generate a remote file from a template (not implemented)" ) );
                return;
            }
            // We need to check that the destination doesn't exist
            QString path = properties->kurl().path();
            bOk = true;
            if ( QFile::exists( path ) )
            {
                QString newDest;
                KIO::RenameDlg_Result res = KIO::open_RenameDlg(
                    oldurl.decodedURL(),
                    path,
                    (KIO::RenameDlg_Mode) (KIO::M_OVERWRITE | KIO::M_SINGLE),
                    true, // well we assume that src is newer
                    newDest
                    );
                switch (res) {
                    case KIO::R_RENAME:
                        properties->updateUrl( newDest );
                        bOk = false; // we need to check this new dest...
                        break;
                    case KIO::R_OVERWRITE:
                        break; //just do it
                    case KIO::R_CANCEL:
                    default:
                        return;
                }
            }
        } while (!bOk);
    }
  }

  // If we launched a job, wait for it to finish
  // Otherwise, we can go on straight away
  if (!job)
    slotRenameFinished( 0L );

}

void FilePropsPage::slotRenameFinished( KIO::Job * job )
{
  if (job && job->error())
  {
    /*KMessageBox::sorry(this,
                     i18n("Could not rename file or directory to\n%1\n")
                     .arg(properties->kurl().url()));*/
    job->showErrorDialog();
    return;
  }

  assert( properties->item() );
  assert( !properties->item()->url().isEmpty() );

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

  // handle icon changes - only local files for now
  // TODO: Use KTempFile and KIO::file_copy with resume = true
  if (!iconArea->isA("QLabel") && properties->kurl().isLocalFile()) {
    KIconLoaderButton *iconButton = (KIconLoaderButton *) iconArea;
    QString path;

    if (S_ISDIR(properties->item()->mode()))
    {
      path = properties->kurl().path(1) + ".directory";
      // don't call updateUrl because the other tabs (i.e. permissions)
      // apply to the directory, not the .directory file.
    }
    else
      path = properties->kurl().path();

    kDebugInfo(1202,"**%s**",path.ascii());
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
    kdDebug(1203) << QString("sIcon = '%1'").arg(sIcon) << endl;
    kdDebug(1203) << QString("str = '%1'").arg(str) << endl;
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
  grpCombo = 0L; grpEdit = 0;
  usrEdit = 0L;
  QString path = properties->kurl().path(-1);
  QString fname = properties->kurl().filename();
  bool isLocal = properties->kurl().isLocalFile();

  bool IamRoot = (geteuid() == 0);
  bool isMyFile, isDir, isLink;

  isLink = properties->item()->isLink();
  isDir = S_ISDIR(properties->item()->mode());
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
    //isMyFile = false; // we have to assume remote files aren't ours.
    isMyFile = true; // how could we know?
    //KIO::chmod will tell, if we had no right to change permissions
  }

  QBoxLayout *box = new QVBoxLayout( this, KDialog::spacingHint() );

  QLabel *l, *cl[3];
  QGroupBox *gb;
  QGridLayout *gl;

  /* Group: Access Permissions */
  gb = new QGroupBox ( i18n("Access permissions"), this );
  box->addWidget (gb);

  gl = new QGridLayout (gb, 6, 6, 15);
  gl->addRowSpacing(0, 10);

  l = new QLabel(i18n("Class"), gb);
  gl->addWidget(l, 1, 0);

  if (isDir)
    l = new QLabel( i18n("Show\nEntries"), gb );
  else
    l = new QLabel( i18n("Read"), gb );
  gl->addWidget (l, 1, 1);

  if (isDir)
    l = new QLabel( i18n("Write\nEntries"), gb );
  else
    l = new QLabel( i18n("Write"), gb );
  gl->addWidget (l, 1, 2);

  if (isDir)
    l = new QLabel( i18n("Enter"), gb );
  else
    l = new QLabel( i18n("Exec"), gb );
  // GJ: Add space between normal and special modes
  QSize size = l->sizeHint();
  size.setWidth(size.width() + 15);
  l->setFixedSize(size);
  gl->addWidget (l, 1, 3);

  l = new QLabel( i18n("Special"), gb );
  gl->addMultiCellWidget(l, 1, 1, 4, 5);

  cl[0] = new QLabel( i18n("User"), gb );
  //l->setEnabled (IamRoot || isMyFile);
  gl->addWidget (cl[0], 2, 0);

  cl[1] = new QLabel( i18n("Group"), gb );
  gl->addWidget (cl[1], 3, 0);

  cl[2] = new QLabel( i18n("Others"), gb );
  gl->addWidget (cl[2], 4, 0);

  l = new QLabel(i18n("Set UID"), gb);
  gl->addWidget(l, 2, 5);

  l = new QLabel(i18n("Set GID"), gb);
  gl->addWidget(l, 3, 5);

  l = new QLabel(i18n("Sticky"), gb);
  gl->addWidget(l, 4, 5);

    /* Draw Checkboxes */
  for (int row = 0; row < 3 ; ++row) {
    for (int col = 0; col < 4; ++col) {
      QCheckBox *cb = new QCheckBox(gb);
      cb->setChecked(permissions & fperm[row][col]);
      cb->setEnabled ((isMyFile || IamRoot) && (!isLink));
      permBox[row][col] = cb;
      gl->addWidget (permBox[row][col], row+2, col+1);
    }
  }
  gl->setColStretch(6, 10);

    /**** Group: Ownership ****/
  gb = new QGroupBox ( i18n("Ownership"), this );
  box->addWidget (gb);

  gl = new QGridLayout (gb, 4, 3, 15);
  gl->addRowSpacing(0, 10);

  /*** Set Owner ***/
  l = new QLabel( i18n("User:"), gb );
  gl->addWidget (l, 1, 0);

  /* GJ: Don't autocomplete more than 1000 users. This is a kind of random
   * value. Huge sites having 10.000+ user have a fair chance of using NIS,
   * (possibly) making this unacceptably slow.
   * OTOH, it is nice to offer this functionality for the standard user.
   */
  int i, maxEntries = 1000;

   /* File owner: For root, offer a KLineEdit with autocompletion.
    * For a user, who can never chown() a file, offer a QLabel.
    */
  if (IamRoot && isLocal)
  {
    usrEdit = new KLineEdit( gb );
    KCompletion *compl = usrEdit->completionObject();
    compl->setSorted(true);
    setpwent();
    for (i=0; ((user = getpwent()) != 0L) && (i < maxEntries); i++)
      compl->addItem(user->pw_name);
    endpwent();
    usrEdit->setCompletionMode((i < maxEntries) ? KGlobalSettings::CompletionAuto :
	  KGlobalSettings::CompletionNone);
    usrEdit->setText(strOwner);
    gl->addWidget(usrEdit, 1, 1);
  }
  else
  {
    l = new QLabel(strOwner, gb);
    gl->addWidget(l, 1, 1);
  }

  /*** Set Group ***/

  QStringList groupList;
  QCString strUser;
  user = getpwuid(geteuid());
  if (user != 0L)
    strUser = user->pw_name;

  if (IamRoot || isMyFile)
  {
    setgrent();
    for (i=0; ((ge = getgrent()) != 0L) && (i < maxEntries); i++)
    {
      if (IamRoot)
	groupList += QString::fromLatin1(ge->gr_name);
      else
      {
	/* pick just the groups the user can change the file to */
	char ** members = ge->gr_mem;
	char * member;
	while ((member = *members) != 0L) {
	  if (strUser == member) {
	    groupList += QString::fromLatin1(ge->gr_name);
	    break;
	  }
	  ++members;
	}
      }
    }
    endgrent();
  }

  /* add the effective Group to the list .. */
  ge = getgrgid (getegid());
  if (ge) {
    QString name = QString::fromLatin1(ge->gr_name);
    if (name.isEmpty())
      name.setNum(ge->gr_gid);
    if (groupList.find(name) == groupList.end())
      groupList += name;
  }

  /* add the group the file currently belongs to ..
   * .. if its not there already
   */
  if (groupList.find(strGroup) == groupList.end())
    groupList += strGroup;

  l = new QLabel( i18n("Group:"), gb );
  gl->addWidget (l, 2, 0);

  /* Set group: if possible to change:
   * - Offer a KLineEdit for root, since he can change to any group.
   * - Offer a QComboBox for a normal user, since he can change to a fixed
   *   (small) set of groups only.
   * If not changable: offer a QLabel.
   */
  if (IamRoot && isLocal)
  {
    grpEdit = new KLineEdit(gb);
    KCompletion *compl = new KCompletion;
    compl->setItems(groupList);
    grpEdit->setCompletionObject(compl, true);
    grpEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
    grpEdit->setText(strGroup);
    gl->addWidget(grpEdit, 2, 1);
  }
  else if ((groupList.count() > 1) && isMyFile && isLocal)
  {
    grpCombo = new QComboBox(gb);
    grpCombo->insertStringList(groupList);
    grpCombo->setCurrentItem(groupList.findIndex(strGroup));
    gl->addWidget(grpCombo, 2, 1);
  }
  else
  {
    l = new QLabel(strGroup, gb);
    gl->addWidget(l, 2, 1);
  }

  gl->setColStretch(2, 10);
  box->addStretch (10);

  if (isMyFile)
    cl[0]->setText(i18n("<b>User</b>"));
  else if (groupList.contains(strGroup))
    cl[1]->setText(i18n("<b>Group</b>"));
  else
    cl[2]->setText(i18n("<b>Others</b>"));
}

bool FilePermissionsPropsPage::supports( KonqFileItemList /*_items*/ )
{
  return true;
}

void FilePermissionsPropsPage::applyChanges()
{
  mode_t p = 0L;
  for (int row = 0;row < 3; ++row)
    for (int col = 0; col < 4; ++col)
      if (permBox[row][col]->isChecked())
	p |= fperm[row][col];

  QString owner, group;
  if (usrEdit)
    owner = usrEdit->text();
  else
    owner = strOwner;
  if (grpEdit)
    group = grpEdit->text();
  else if (grpCombo)
    group = grpCombo->currentText();
  else
    group = strGroup;

  // First update group / owner
  // (permissions have to set after, in case of suid and sgid)
  if ((owner != strOwner) || (group != strGroup))
  {
    struct passwd* pw = getpwnam(owner.latin1());
    struct group* g = getgrnam(group.latin1());
    if ( pw == 0L ) {
      kDebugError(1202," ERROR: No user %s", owner.latin1());
      return;
    }
    if ( g == 0L ) {
      kDebugError(1202," ERROR: No group %s", group.latin1());
      return;
    }
    QString path = properties->kurl().path();
    if ( chown( path, pw->pw_uid, g->gr_gid ) != 0 )
      KMessageBox::sorry( 0, i18n( "Could not change owner/group\nPerhaps access denied." ));
  }

  kdDebug(1203) << "old permissions : " << permissions << endl;
  kdDebug(1203) << "new permissions : " << p << endl;
  kdDebug(1203) << "url : " << properties->kurl().url() << endl;
  if ( permissions != p )
  {
    KIO::Job * job = KIO::chmod( properties->kurl(), p );
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotChmodResult( KIO::Job * ) ) );
  }

}

void FilePermissionsPropsPage::slotChmodResult( KIO::Job * job )
{
  if (job->error())
    job->showErrorDialog();
  else
  {
    // Force refreshing information about that file if displayed.
    KDirWatch::self()->setFileDirty( properties->kurl().path() );
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

  // The groupbox about run with substituted uid.

  tmpQGroupBox = new QGroupBox(this, "GroupBox_3");
  tmpQGroupBox->setFrameStyle(QFrame::Sunken|QFrame::Box);
  tmpQGroupBox->setAlignment(Qt::AlignLeft);
  mainlayout->addWidget(tmpQGroupBox, 2);

  grouplayout = new QVBoxLayout(tmpQGroupBox, KDialog::spacingHint());
  suidCheck = new QCheckBox(tmpQGroupBox, "RadioButton_4");
  suidCheck->setText(i18n("Run as a different user"));
  grouplayout->addWidget(suidCheck, 0);

  hlayout = new QHBoxLayout(KDialog::spacingHint());
  grouplayout->addLayout(hlayout, 1);
  l = new QLabel(tmpQGroupBox, "Label_6");
  l->setText(i18n("Username"));
  hlayout->addWidget(l, 0);
  suidEdit = new KLineEdit(tmpQGroupBox, "LineEdit_6");
  hlayout->addWidget(suidEdit, 1);
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
  suidStr = config.readEntry( "X-KDE-SubstituteUID" );
  suidUserStr = config.readEntry( "X-KDE-Username" );

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

  suidCheck->setChecked( suidStr == "1" );
  suidEdit->setText( suidUserStr );
  enableSuidEdit();

  // Provide username completion up to 1000 users.
  KCompletion *compl = new KCompletion;
  compl->setSorted(true);
  struct passwd *pw;
  int i, maxEntries = 1000;
  setpwent();
  for (i=0; ((pw = getpwent()) != 0L) && (i < maxEntries); i++)
    compl->addItem(QString::fromLatin1(pw->pw_name));
  endpwent();
  if (i < maxEntries)
  {
    suidEdit->setCompletionObject(compl, true);
    suidEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
  }
  else
  {
    delete compl;
  }

  connect( execBrowse, SIGNAL( clicked() ), this, SLOT( slotBrowseExec() ) );
  connect( terminalCheck, SIGNAL( clicked() ), this,  SLOT( enableCheckedEdit() ) );
  connect( suidCheck, SIGNAL( clicked() ), this,  SLOT( enableSuidEdit() ) );

}


void ExecPropsPage::enableCheckedEdit()
{
  terminalEdit->setEnabled(terminalCheck->isChecked());
}

void ExecPropsPage::enableSuidEdit()
{
  suidEdit->setEnabled(suidCheck->isChecked());
}

bool ExecPropsPage::supports( KonqFileItemList _items )
{
  KonqFileItem * item = _items.first();
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
  config.writeEntry( "X-KDE-SubstituteUID", suidCheck->isChecked() ? "1" : "0" );
  config.writeEntry( "X-KDE-Username", suidEdit->text() );
}


void ExecPropsPage::slotBrowseExec()
{
    KURL f = KFileDialog::getOpenURL( QString::null,
				      QString::null, this );
    if ( f.isEmpty() )
	return;

    if ( !f.isLocalFile()) {
	KMessageBox::sorry(this, i18n("Sorry, but only executables of the local file systems are supported."));
	return;
    }

    execEdit->setText( f.path() );
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

bool URLPropsPage::supports( KonqFileItemList _items )
{
  KonqFileItem * item = _items.first();
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

bool ApplicationPropsPage::supports( KonqFileItemList _items )
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

bool BindingPropsPage::supports( KonqFileItemList _items )
{
  KonqFileItem * item = _items.first();
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
  unmounted->setIconType("devices");
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

bool DevicePropsPage::supports( KonqFileItemList _items )
{
  KonqFileItem * item = _items.first();
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
  kdDebug(1203) << "unmounted->icon() = " << unmounted->icon() << endl;

  config.writeEntry( "ReadOnly", readonly->isChecked() );

  config.sync();
}

#include "kpropsdlg.moc"
