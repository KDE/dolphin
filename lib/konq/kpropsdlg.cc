/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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
 * View/Edit Properties of files (later : URLs)
 *
 * some FilePermissionsPropsPage-changes by 
 *  Henner Zeller <zeller@think.de>
 * some layout management by 
 *  Bertrand Leconte <B.Leconte@mail.dotcom.fr>
 * the rest of the layout management, bug fixes, adaptation to libkio by
 *  David Faure <faure@kde.org>
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
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qlist.h>
#include <qstrlist.h>
#include <qstringlist.h>
#include <qpainter.h>

#include <kdesktopfile.h>
#include <kiconloaderdialog.h>
#include <kiconloader.h>
#include <kurl.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kfiledialog.h>
#include <kmimetypes.h>
#include <kservices.h>
#include <kfileitem.h> // for encodeFileName

#include "kpropsdlg.h"
#include "userpaths.h"

#include <X11/Xlib.h> // for XSetTransientForHint

#define SEPARATION 10

mode_t FilePermissionsPropsPage::fperm[3][4] = {
        {S_IRUSR, S_IWUSR, S_IXUSR, S_ISUID},
        {S_IRGRP, S_IWGRP, S_IXGRP, S_ISGID},
        {S_IROTH, S_IWOTH, S_IXOTH, S_ISVTX}
    };

PropertiesDialog::PropertiesDialog( KFileItemList _items ) :
  // TODO : handle all items
  // Current HACK : only use the first item
  m_item( _items.first() ), m_items( _items ), m_bMustDestroyItem( false )
{
  init();
}

PropertiesDialog::PropertiesDialog( const QString& _url, mode_t _mode ) :
  m_bMustDestroyItem( true )
{
  // Create a KFileItem from the information we have
  KURL u( _url );
  m_item = new KFileItem( "unknown", _mode, u );
  m_items.append( m_item );
  init();
}

void PropertiesDialog::init()
{
  pageList.setAutoDelete( true );
    
  tab = new QTabDialog( 0L, 0L );

  // Matthias: let the dialog look like a modal dialog
  XSetTransientForHint(qt_xdisplay(), tab->winId(), tab->winId());

  tab->setGeometry( tab->x(), tab->y(), 400, 400 );

  insertPages();

  tab->setOKButton(i18n("OK")); 
  tab->setCancelButton(i18n("Cancel"));

  connect( tab, SIGNAL( applyButtonPressed() ), this, SLOT( slotApply() ) );
  connect( tab, SIGNAL( cancelButtonPressed() ), this, SLOT( slotCancel() ) );
    
  tab->show();
}


PropertiesDialog::~PropertiesDialog()
{
  pageList.clear();    
  if ( m_bMustDestroyItem ) delete m_item;
}

bool PropertiesDialog::canDisplay( KFileItemList _items )
{
  return FilePropsPage::supports( _items ) ||
         FilePermissionsPropsPage::supports( _items ) ||
         ExecPropsPage::supports( _items ) ||
         ApplicationPropsPage::supports( _items ) ||
         BindingPropsPage::supports( _items ) ||
         URLPropsPage::supports( _items ) ||
         DirPropsPage::supports( _items ) ||
         DevicePropsPage::supports( _items );
}

void PropertiesDialog::slotApply()
{
  PropsPage *page;
  for ( page = pageList.last(); page != 0L; page = pageList.prev() )
    page->applyChanges();

  delete this;
}

void PropertiesDialog::slotCancel()
{
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

    if ( DirPropsPage::supports( m_items ) )
    {
	PropsPage *p = new DirPropsPage( this );
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

FilePropsPage::FilePropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
    // Extract the directories name without path
    QString filename = properties->kurl().filename();
    /*
    QString tmp2 = properties->kurl().path();
    if ( tmp2.at(tmp2.length() - 1) == '/' )
	tmp2.truncate( tmp2.length() - 1 );
    int i = tmp2.findRev( "/" );
    // Is is not the root directory ?
    if ( i != -1 )
	filename = tmp2.mid( i + 1, tmp2.length() );
    else
	filename = '/';
    */
    
    // Make it human-readable (%2F => '/', ...)
    filename = KFileItem::decodeFileName( filename );

    QString tmp = properties->kurl().path( 1 );
    bool isTrash = false;
    // is it the trash bin ?
    if ( properties->kurl().isLocalFile() && tmp == UserPaths::trashPath())
      isTrash = true;
    
    QString path = properties->kurl().path();
    QLabel *l;
 
    layout = new QBoxLayout(this, QBoxLayout::TopToBottom, SEPARATION); 

    l = new QLabel( i18n("File Name"), this );
    l->setFixedSize(l->sizeHint());
    layout->addWidget(l, 0, AlignLeft);
    
    name = new QLineEdit( this );
    name->setMinimumSize(200, fontHeight);
    name->setMaximumSize(QLayout::unlimited, fontHeight);
    layout->addWidget(name, 0, AlignLeft);
    name->setText( filename );
    // Dont rename trash or root directory
    if ( isTrash || filename == "/" )
	name->setEnabled( false );
    oldName = filename;

    l = new QLabel( i18n("Full Name"), this );
    l->setFixedSize(l->sizeHint());
    layout->addWidget(l, 0, AlignLeft);
    
    QLabel *fname = new QLabel( this );
    fname->setText( path );
    fname->setMinimumSize(200, fontHeight);
    fname->setMaximumSize(QLayout::unlimited, fontHeight);
    fname->setLineWidth(1);
    fname->setFrameStyle(QFrame::Box | QFrame::Raised);
    layout->addWidget(fname, 0, AlignLeft);
    
    layout->addSpacing(SEPARATION);

    if ( isTrash )
    {
	l = new QLabel( i18n( "Is the Trash Bin"), this );
	l->setFixedSize(l->sizeHint());
	layout->addWidget(l, 0, AlignLeft);
    }
    else if ( S_ISDIR( properties->item()->mode() ) )
    {
	l = new QLabel( i18n("Is a Directory"), this );
	l->setFixedSize(l->sizeHint());
	layout->addWidget(l, 0, AlignLeft);
    }
    if ( properties->item()->isLink() )
    {
	l = new QLabel( i18n( "Points to" ), this );
	l->setFixedSize(l->sizeHint());
	layout->addWidget(l, 0, AlignLeft);
    
        QLabel *lname = new QLabel( this );
        lname->setMinimumSize(200, fontHeight);
        lname->setMaximumSize(QLayout::unlimited, fontHeight);
        lname->setLineWidth(1);
        lname->setFrameStyle(QFrame::Box | QFrame::Raised);
        layout->addWidget(lname, 0, AlignLeft);

        QString linkDest = properties->item()->linkDest();
        if ( linkDest.isNull() && properties->kurl().isLocalFile() )
        {
          char buffer[1024];
          int n = readlink( path.ascii(), buffer, 1022 );
          if ( n > 0 )
          {
	    buffer[ n ] = 0;
	    linkDest = buffer;
          }
        }
        lname->setText( linkDest );
    }
    else if ( S_ISREG( properties->item()->mode() ) )
    {
        QString tempstr = i18n("Size: %1").arg( properties->item()->size() );
	l = new QLabel( tempstr, this );
	l->setFixedSize(l->sizeHint());
	layout->addWidget(l, 0, AlignLeft);
    }
    
    QString tempstr = i18n("Mimetype: %1").arg( properties->item()->mimetype() );
    l = new QLabel( tempstr, this );
    l->setFixedSize(l->sizeHint());
    layout->addWidget(l, 0, AlignLeft);

    /*
    char buffer[1024];
    struct tm *t = localtime( &lbuff.st_atime );
    sprintf( buffer, "%s: %02i:%02i %02i.%02i.%04i", 
	     (const char*)i18n("Last Access"),
	     t->tm_hour,t->tm_min,
	     t->tm_mday,t->tm_mon + 1,t->tm_year + 1900 );             
    */
    QString buffer = i18n("Last Access: %1").arg(
      properties->item()->time( UDS_ACCESS_TIME ) );
    l = new QLabel( buffer, this );
    l->setFixedSize(l->sizeHint());
    layout->addWidget(l, 0, AlignLeft);

    /*
    t = localtime( &lbuff.st_mtime );
    sprintf( buffer, "%s: %02i:%02i %02i.%02i.%04i", 
	     (const char*)i18n("Last Modified"),
	     t->tm_hour,t->tm_min,
	     t->tm_mday,t->tm_mon + 1,t->tm_year + 1900 );          
    */
    buffer = i18n("Last Modified: %1").arg(
      properties->item()->time( UDS_MODIFICATION_TIME ) );
    l = new QLabel( buffer, this );
    l->setFixedSize(l->sizeHint());
    layout->addWidget(l, 0, AlignLeft);

    layout->addStretch(10);
    layout->activate();
}

bool FilePropsPage::supports( KFileItemList /*_items*/ )
{
  return true; /* was _kurl.isLocalFile(); */
}

void FilePropsPage::applyChanges()
{
    QString path = properties->kurl().path();
    QString fname = properties->kurl().filename();
    QString n = KFileItem::encodeFileName( name->text() );

    // Do we need to rename the file ?
    if ( oldName != n )
    {
	QString s = path;
	if ( s.at(s.length() - 1) == '/') 
	  // It's a directory, so strip the trailing slash first
	  s.truncate( s.length() - 1);

	int i = s.findRev( "/" );
	// Should never happen
	if ( i == -1 )
	    return;
	s.truncate( i );
	s += '/';
	s += n;
	if ( path.at(path.length() - 1) == '/') 
	  // It's a directory, so strip the trailing slash (in case it's a
          // symlink)
	  path.truncate( path.length() - 1);
	if ( rename( path, s ) != 0 ) {
            QString tmp;
            tmp.sprintf(i18n("Could not rename the file or directory\n%s\n"), strerror(errno));
            QMessageBox::warning( this, i18n( "Properties Dialog Error" ), tmp, i18n("OK"));
        }
	// properties->emitPropertiesChanged( n );
    }
}

FilePermissionsPropsPage::FilePermissionsPropsPage( PropertiesDialog *_props ) 
: PropsPage( _props )
{
    QString path = properties->kurl().path();
    QString fname = properties->kurl().filename();

    /* remove appended '/' .. see comment in FilePropsPage */
    if ( path.length() > 1 && path.at( path.length() - 1 ) == '/' )
	path.truncate( path.length() - 1 );

    struct stat buff;
    lstat( path.ascii(), &buff ); // display uid/gid of the link, if it's a link
    struct passwd * user = getpwuid( buff.st_uid );
    struct group * ge = getgrgid( buff.st_gid );

    bool IamRoot = (geteuid() == 0);
    bool IsMyFile  = (geteuid() == buff.st_uid);
    bool IsDir =  S_ISDIR (buff.st_mode);
    bool IsLink =  S_ISLNK( buff.st_mode );

    permissions = buff.st_mode & ( S_IRWXU | S_IRWXG | S_IRWXO |
				   S_ISUID | S_ISGID | S_ISVTX );
    strOwner = "";
    strGroup = "";
    if ( user != 0L )
    {
	strOwner = user->pw_name;
    }    
    if ( ge != 0L )
    {
	strGroup = ge->gr_name;
	if (strGroup.isEmpty())
	    strGroup.sprintf("%d",ge->gr_gid);
    } else 
	strGroup.sprintf("%d",buff.st_gid);

    QBoxLayout *box = new QVBoxLayout( this, SEPARATION );

    QLabel *l;
    QGroupBox *gb;
    QGridLayout *gl;

    /* Group: Access Permissions */
    gb = new QGroupBox ( i18n("Access permissions"), this );
    box->addWidget (gb);

    gl = new QGridLayout (gb, 4, 6, 15);

    if (IsDir)
	    l = new QLabel( i18n("Show\nEntries"), gb );
    else
	    l = new QLabel( i18n("Read"), gb );
    l->setMinimumSize( l->sizeHint() );
    gl->addWidget (l, 0, 1);

    if (IsDir) 
	    l = new QLabel( i18n("Write\nEntries"), gb );
    else
	    l = new QLabel( i18n("Write"), gb );
    l->setMinimumSize( l->sizeHint() );
    gl->addWidget (l, 0, 2);

    if (IsDir)
	    l = new QLabel( i18n("Change\nInto"), gb );
    else
	    l = new QLabel( i18n("Exec"), gb );
    l->setMinimumSize( l->sizeHint() );
    gl->addWidget (l, 0, 3);

    l = new QLabel( i18n("Special"), gb );
    l->setMinimumSize( l->sizeHint() );
    gl->addWidget (l, 0, 4);

    l = new QLabel( i18n("User"), gb );
    l->setMinimumSize( l->sizeHint() );
    l->setEnabled (IamRoot || IsMyFile);
    gl->addWidget (l, 1, 0);

    l = new QLabel( i18n("Group"), gb );
    l->setMinimumSize( l->sizeHint() );
    l->setEnabled (IamRoot || IsMyFile);
    gl->addWidget (l, 2, 0);

    l = new QLabel( i18n("Others"), gb );
    l->setMinimumSize( l->sizeHint() );
    l->setEnabled (IamRoot || IsMyFile);
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
		    cb->setChecked ((buff.st_mode & fperm[row][col]) 
				    == fperm[row][col]);
		    cb->setEnabled ((IsMyFile || IamRoot) && (!IsLink));
		    cb->setMinimumSize (cb->sizeHint());
		    permBox[row][col] = cb;
		    gl->addWidget (permBox[row][col], row+1, col+1);
	    }
    }
    gl->setColStretch(5, 10);
    gl->activate();
    gb->adjustSize();

    /**** Group: Ownership ****/
    gb = new QGroupBox ( i18n("Ownership"), this );
    box->addWidget (gb);

    gl = new QGridLayout (gb, 4, 3, 15);

    /*** Set Owner ***/
    l = new QLabel( i18n("Owner"), gb );
    l->setMinimumSize( l->sizeHint() );
    gl->addWidget (l, 1, 0);
    
    /* maybe this should be a combo-box, but this isn't handy
     * if there are 2000 Users (tiny scrollbar!)
     * anyone developing a combo-box with incremental search .. ?
     */
    owner = new QLineEdit( gb );
    owner->setMinimumSize( owner->sizeHint().width(), fontHeight );
    gl->addWidget (owner, 1, 1);
    owner->setText( strOwner );
    owner->setEnabled ( IamRoot ); // we can just change the user if we're root

    /*** Set Group ***/
    /* get possible groups .. */
    QStrList *groupList = new QStrList (true);

    if (IsMyFile || IamRoot) {  // build list just if we have to
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
    l->setMinimumSize( l->sizeHint() );
    gl->addWidget (l, 2, 0);

    if (groupList->count() > 1 && 
	(IamRoot || IsMyFile)) { 
      /* Combo-Box .. if there is anything to change */
      if (groupList->count() <= 10)
	/* Motif-Style looks _much_ nicer for few items */
	grp = new QComboBox( gb ); 
      else
	grp = new QComboBox( false, gb );
      
      grp->insertStrList ( groupList );
      grp->setCurrentItem (groupList->find ( strGroup ));
      grp->setMinimumSize( grp->sizeHint().width(), fontHeight );
      gl->addWidget (grp, 2, 1);
      grp->setEnabled (IamRoot || IsMyFile);
    }
    else {
      // no choice; just present the group in a disabled edit-field
      QLineEdit *e = new QLineEdit ( gb );
      e->setMinimumSize (e->sizeHint().width(), fontHeight);
      e->setText ( strGroup );
      e->setEnabled (false);
      gl->addWidget (e, 2, 1);
      grp = 0L;
    }
    
    delete groupList;

    gl->setColStretch(2, 10);
    gl->activate();
    gb->adjustSize();

    box->addStretch (10);
    box->activate();
}

bool FilePermissionsPropsPage::supports( KFileItemList _items )
{
  // TODO : return true for any URL and implement read-only permissions
  // for non local files
  return _items.first()->url().isLocalFile();
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
	 (grp && grp->text(grp->currentItem()) != strGroup ))
    {
	struct passwd* pw = getpwnam( owner->text() );
	struct group* g;
	if (grp)
	    g = getgrnam( grp->text(grp->currentItem()) );
	else 
	    g = 0L;
	
	if ( pw == 0L )
	{
	    warning(i18n(" ERROR: No user %s"),(const char*)owner->text() );
	    return;
	}
	if ( g == 0L )
	{
	    warning(i18n(" ERROR: No group %s"),
		    (const char*)grp->text(grp->currentItem()) ); // should never happen
	    return;
	}
	if ( chown( path, pw->pw_uid, g->gr_gid ) != 0 )
	    QMessageBox::warning( 0, i18n( "Properties Dialog Error" ),
				  i18n( "Could not change owner/group\nPerhaps access denied." ),
				  i18n( "OK") );
    }    

    if ( p != permissions )
    {
	if ( chmod( path, p ) != 0 )
	    QMessageBox::warning( 0, i18n( "Properties Dialog Error" ),
				  i18n( "Could not change permissions\nPerhaps access denied." ),
				  i18n( "OK") );
    }

}

ExecPropsPage::ExecPropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
    QVBoxLayout * mainlayout = new QVBoxLayout(this, SEPARATION);

    // Now the widgets in the top layout

    QLabel* tmpQLabel;
    tmpQLabel = new QLabel( this, "Label_1" );
    tmpQLabel->setText( i18n("Execute") );
    tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
    mainlayout->addWidget(tmpQLabel, 1);

    QHBoxLayout * hlayout;
    hlayout = new QHBoxLayout(SEPARATION);
    mainlayout->addLayout(hlayout, 1);

    execEdit = new QLineEdit( this, "LineEdit_1" );
    execEdit->setText( "" );
    execEdit->setMaxLength( 256 );
    execEdit->setFixedHeight(fontHeight);
    execEdit->setMinimumSize( execEdit->sizeHint() );
    hlayout->addWidget(execEdit, 0);

    execBrowse = new QPushButton( this, "Button_1" );
    execBrowse->setText( i18n("&Browse...") );
    execBrowse->setFixedSize(execBrowse->sizeHint());
    hlayout->addWidget(execBrowse, 0);

    hlayout = new QHBoxLayout(SEPARATION);
    mainlayout->addLayout(hlayout, 2); // double stretch, because two items

    /* instead, a label */
    hlayout->addStretch(1);

    // and the icon button, on the right of this vertical layout
    iconBox = new KIconLoaderButton( KGlobal::iconLoader(), this );
    iconBox->setFixedSize( 50, 50 );
    iconBox->setIconType("icon"); // Choose from app icons
    hlayout->addWidget(iconBox, 0, AlignCenter);

    // The groupbox about swallowing

    QGroupBox* tmpQGroupBox;
    tmpQGroupBox = new QGroupBox( i18n("Swallowing on panel"), this, "GroupBox_1" );
    tmpQGroupBox->setFrameStyle( 49 );
    mainlayout->addWidget(tmpQGroupBox, 3); // 3 vertical items -> stretch = 3

    QVBoxLayout * grouplayout;
    grouplayout = new QVBoxLayout(tmpQGroupBox, SEPARATION);

    grouplayout->addSpacing( fontMetrics().height() ); 

    hlayout = new QHBoxLayout(SEPARATION);
    grouplayout->addLayout(hlayout, 0);

    tmpQLabel = new QLabel( tmpQGroupBox, "Label_6" );
    tmpQLabel->setText( i18n("Execute") );
    tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
    hlayout->addWidget(tmpQLabel, 0);

    swallowExecEdit = new QLineEdit( tmpQGroupBox, "LineEdit_3" );
    swallowExecEdit->setText( "" );
    swallowExecEdit->setFixedHeight(fontHeight);
    swallowExecEdit->setMinimumSize( swallowExecEdit->sizeHint() );
    hlayout->addWidget(swallowExecEdit, 1);

    hlayout = new QHBoxLayout(SEPARATION);
    grouplayout->addLayout(hlayout, 0);

    tmpQLabel = new QLabel( tmpQGroupBox, "Label_7" );
    tmpQLabel->setText( i18n("Window Title") );
    tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
    hlayout->addWidget(tmpQLabel, 0);

    swallowTitleEdit = new QLineEdit( tmpQGroupBox, "LineEdit_4" );
    swallowTitleEdit->setText( "" );
    swallowTitleEdit->setFixedHeight(fontHeight);
    swallowTitleEdit->setMinimumSize( swallowTitleEdit->sizeHint() );
    hlayout->addWidget(swallowTitleEdit, 1);

    // The groupbox about run in terminal

    tmpQGroupBox = new QGroupBox( this, "GroupBox_2" );
    tmpQGroupBox->setFrameStyle( 49 );
    tmpQGroupBox->setAlignment( 1 );
    mainlayout->addWidget(tmpQGroupBox, 2);  // 2 vertical items -> stretch = 2

    grouplayout = new QVBoxLayout(tmpQGroupBox, SEPARATION);

    terminalCheck = new QCheckBox( tmpQGroupBox, "RadioButton_3" );
    terminalCheck->setText( i18n("Run in terminal") );
    terminalCheck->setMinimumSize( terminalCheck->sizeHint() );
    grouplayout->addWidget(terminalCheck, 0);

    hlayout = new QHBoxLayout(SEPARATION);
    grouplayout->addLayout(hlayout, 1);

    tmpQLabel = new QLabel( tmpQGroupBox, "Label_5" );
    tmpQLabel->setText( i18n("Terminal Options") );
    tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
    hlayout->addWidget(tmpQLabel, 0);

    terminalEdit = new QLineEdit( tmpQGroupBox, "LineEdit_5" );
    terminalEdit->setText( "" );
    terminalEdit->setFixedHeight(fontHeight);
    terminalEdit->setMinimumSize( terminalEdit->sizeHint() );
    hlayout->addWidget(terminalEdit, 1);

    mainlayout->activate();

    //

    QFile f( _props->kurl().path() );
    if ( !f.open( IO_ReadOnly ) )
	return;    
    f.close();

    KConfig config( _props->kurl().path() );
    config.setDollarExpansion( false );
    config.setDesktopGroup();
    execStr = config.readEntry( "Exec" );
    iconStr = config.readEntry( "Icon" );
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

    if ( iconStr.isNull() )
        iconStr = KMimeType::mimeType("")->KServiceType::icon(); // default icon
    
    iconBox->setIcon( iconStr ); 

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
#ifdef SVEN
// --- Sven's editable global settings changes start ---
    int i = 0;
    bool err = false;
// --- Sven's editable global settings changes end ---
#endif
    QString path = properties->kurl().path();

    QFile f( path );

#ifdef SVEN
    QDir lDir (kapp->localkdedir() + "/share/applnk/"); // I know it exists

    //debug (path.ascii());
    //debug (kapp->kde_appsdir().ascii());
#endif
    if ( !f.open( IO_ReadWrite ) )
    {
#ifdef SVEN
      // path = /usr/local/kde/share/applnk/network/netscape.kdelnk

      //Does path contain kde_appsdir?
      if (path.find(kapp->kde_appsdir()) == 0) // kde_appsdir on start of path
      {
	path.remove(0, strlen(kapp->kde_appsdir())); //remove kde_appsdir

	if (path[0] == '/')
	  path.remove(0, 1); // remove /

	while (path.contains('/'))
	{
	  i = path.find('/'); // find separator
	  if (!lDir.cd(path.left(i)))  // exists?
	  {
	    lDir.mkdir((path.left(i)));  // no, create
	    if (!lDir.cd((path.left(i)))) // can cd to?
	    {
	      err = true;                 // no flag it...
	      // debug ("Can't cd to  %s in %s", path.left(i).ascii(),
	      //	 lDir.absPath().ascii());
	      break;                      // and exit while
	    }
	  }
	  path.remove (0, i);           // cded to;
	  if (path[0] == '/')
	    path.remove(0, 1); // remove / from path
	}
      }
      else // path didn't contain kde_appsdir - this is an error
	err = true;

      // we created all subdirs or failed
      if (!err) // if we didn't fail try to make file just for check
      {
	path.prepend('/'); // be sure to have /netscape.kdelnk
	path.prepend(lDir.absPath());
	f.setName(path);
	//debug("path = %s", path.ascii());

	// we must not copy whole kdelnk to local dir
	//  because it was done in ApplicationPropsPage
	if (!f.open( IO_ReadWrite ) )
	  err = true;
      }
      if (err)
      {
#endif
      
	QMessageBox::warning( 0, i18n("Properties Dialog Error"),
			      i18n("Could not save properties\nPerhaps permissions denied"),
			      i18n("OK") );
	return;
#ifdef SVEN
      }
#endif
    }

    f.close();

    KConfig config( path );
    config.setDesktopGroup();
    config.writeEntry( "Exec", execEdit->text() );
    config.writeEntry( "Icon", iconBox->icon() );
    config.writeEntry( "SwallowExec", swallowExecEdit->text() );
    config.writeEntry( "SwallowTitle", swallowTitleEdit->text() );

    if ( terminalCheck->isChecked() )
    {
        config.writeEntry( "Terminal", "1" );
        config.writeEntry( "TerminalOptions", terminalEdit->text() );
    }
    else
        config.writeEntry( "Terminal", "0" );

    config.sync();
}


void ExecPropsPage::slotBrowseExec()
{
    QString f = KFileDialog::getOpenFileName( QString::null, QString::null, this );
    if ( f.isNull() )
	return;

    execEdit->setText( f );
}

URLPropsPage::URLPropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
    QVBoxLayout * layout = new QVBoxLayout(this, SEPARATION);
    URLEdit = new QLineEdit( this, "LineEdit_1" );
    iconBox = new KIconLoaderButton( KGlobal::iconLoader(), this );

    QLabel* tmpQLabel;
    tmpQLabel = new QLabel( this, "Label_1" );
    tmpQLabel->setText( i18n("URL") );
    tmpQLabel->adjustSize();
    tmpQLabel->setFixedHeight( fontHeight );
    layout->addWidget(tmpQLabel);

    URLEdit->setText( "" );
    URLEdit->setMaxLength( 256 );
    URLEdit->setMinimumSize( URLEdit->sizeHint() );
    URLEdit->setFixedHeight( fontHeight );
    layout->addWidget(URLEdit);
    
    iconBox->setFixedSize( 50, 50 );
    layout->addWidget(iconBox, 0, AlignLeft);

    QString path = _props->kurl().path();

    QFile f( path );
    if ( !f.open( IO_ReadOnly ) )
	return;
    f.close();

    KConfig config( path );
    config.setDesktopGroup();
   URLStr = config.readEntry(  "URL" );
    iconStr = config.readEntry( "Icon" );

    if ( !URLStr.isNull() )
	URLEdit->setText( URLStr );
    if ( iconStr.isNull() )
        iconStr = KMimeType::mimeType("")->KServiceType::icon(); // default icon

    iconBox->setIcon( iconStr );

    layout->addStretch (10);
    layout->activate();

}

bool URLPropsPage::supports( KFileItemList _items )
{
  KFileItem * item = _items.first();
  // check if desktop file
  if ( !PropsPage::isDesktopFile( item ) )
    return false;

  // open file and check type
  KDesktopFile config( item->url().path(), true /* readonly */ );
  return config.hasURLType();
}

void URLPropsPage::applyChanges()
{
    QString path = properties->kurl().path();

    QFile f( path );
    if ( !f.open( IO_ReadWrite ) )
    {
	QMessageBox::warning( 0, i18n("Properties Dialog Error"), 
			        i18n("Could not save properties\nPerhaps permissions denied"),
				i18n("OK") );
	return;
    }
    f.close();

    KConfig config( path );
    config.setDesktopGroup();
    config.writeEntry( "URL", URLEdit->text() );
    config.writeEntry( "Icon", iconBox->icon() );
    config.writeEntry( "MiniIcon", iconBox->icon() );
    config.sync();
}

DirPropsPage::DirPropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
    // This is strange, but if we try to use a layout in this page, it centers
    // the other pages of the dialog (!?). Ok, no layout ! DF.

    // See resize event for widgets placement
    iconBox = new KIconLoaderButton( KGlobal::iconLoader(), this );

    QLabel* tmpQLabel = new QLabel( this, "Label_1" );
    tmpQLabel->setText( i18n("Background") );
    tmpQLabel->move( 10, 90 );
    tmpQLabel->adjustSize();

    wallBox = new QComboBox( false, this, "ComboBox_1" );

    applyButton = new QPushButton( i18n("Apply") , this );
    applyButton->adjustSize();
    connect( applyButton, SIGNAL( clicked() ), this, SLOT( slotApply() ) );
    
    globalButton = new QPushButton( i18n("Apply global"), this );
    globalButton->adjustSize();
    connect( globalButton, SIGNAL( clicked() ), this, SLOT( slotApplyGlobal() ) );

    QString tmp = _props->kurl().path();
    
    if ( tmp.at(tmp.length() - 1) != '/' )
	tmp += "/.directory";
    else
	tmp += ".directory";

    QFile f( tmp );
    if ( f.open( IO_ReadOnly ) )
    {
	f.close();

	KConfig config( tmp );
	config.setDesktopGroup();
	wallStr = config.readEntry( "BgImage" );
	iconStr = config.readEntry( "Icon" );
    }

    if ( iconStr.isEmpty() )
    {
	QString str( KMimeType::findByURL( properties->kurl(), properties->item()->mode(), true )->KServiceType::icon() );
	KURL u( str );
	iconStr = u.filename();
    }
    
    iconBox->setIcon( iconStr );

    QStringList list = KGlobal::dirs()->findAllResources("wallpaper");
    wallBox->insertItem(  i18n("(Default)"), 0 );
    
    for (QStringList::ConstIterator it = list.begin(); it != list.end(); it++)
	wallBox->insertItem( *it );
    
    showSettings( wallStr );

    browseButton = new QPushButton( i18n("&Browse..."), this );
    browseButton->adjustSize();
    connect( browseButton, SIGNAL( clicked() ), SLOT( slotBrowse() ) );

    drawWallPaper();

    connect( wallBox, SIGNAL( activated( int ) ), this, SLOT( slotWallPaperChanged( int ) ) );
}

bool DirPropsPage::supports( KFileItemList _items )
{
  KFileItem * item = _items.first();

  if ( !S_ISDIR( item->mode() ) )
    return false;

  // Is it the trash bin ?
  QString path = item->url().path( 1 ); // adds trailing slash
    
  if ( item->url().isLocalFile() && path == UserPaths::trashPath() )
    return false;

  return true;
}

void DirPropsPage::applyChanges()
{
    QString tmp = properties->kurl().path();
    if ( tmp.at( tmp.length() - 1 ) != '/' )
	tmp += "/.directory";
    else
	tmp += ".directory";

    QFile f( tmp );
    if ( !f.open( IO_ReadWrite ) )
    {
      QMessageBox::warning( 0, i18n("Properties Dialog Error"), 
			     i18n("Could not write to\n") + tmp,
			     i18n("OK") );
	return;
    }
    f.close();

    KConfig config( tmp );
    config.setDesktopGroup();

    int i = wallBox->currentItem();
    if ( i != -1 )
    {
	if ( strcmp( wallBox->text( i ),  i18n("(Default)") ) == 0 )
	    config.writeEntry( "BgImage", "" );
	else
	    config.writeEntry( "BgImage", wallBox->text( i ) );
    }

    // Get the default image
    QString str( KMimeType::findByURL( properties->kurl(),
				       properties->item()->mode(), true )->KServiceType::icon() );
    KURL u( str );
    QString str2 = u.filename();
    QString sIcon;
    // Is it another one than the default ?
    if ( str2 != iconBox->icon() )
        sIcon = iconBox->icon();
    config.writeEntry( "Icon", sIcon );
    config.writeEntry( "MiniIcon", sIcon );
    
    config.sync();

    // Send a notify to the parent directory since the
    // icon may have changed

    tmp = properties->kurl().url();
    
    if ( tmp.at( tmp.length() - 1) == '/' )
	tmp.truncate( tmp.length() - 1 );
    
    i = tmp.findRev( "/" );
    // Should never happen
    if ( i == -1 )
	return;
    tmp.truncate ( i + 1 );
}

void DirPropsPage::showSettings( QString filename )
{
  wallBox->setCurrentItem( 0 );
  for ( int i = 1; i < wallBox->count(); i++ )
    {
      if ( filename == wallBox->text( i ) )
        {
          wallBox->setCurrentItem( i );
          return;
        }
    }
 
  if ( !filename.isEmpty() )
    {
      wallBox->insertItem( filename );
      wallBox->setCurrentItem( wallBox->count()-1 );
    }
}

void DirPropsPage::slotBrowse( )
{
    QString filename = KFileDialog::getOpenFileName( 0 );
    showSettings( filename );
    drawWallPaper( );
}

void DirPropsPage::slotWallPaperChanged( int )
{
    drawWallPaper();
}

void DirPropsPage::paintEvent( QPaintEvent *_ev )
{
    PropsPage::paintEvent( _ev );
    drawWallPaper();
}

void DirPropsPage::drawWallPaper()
{
    int i = wallBox->currentItem();
    if ( i == -1 )
    {
	erase( imageX, imageY, imageW, imageH );
	return;
    }
    
    QString text = wallBox->text( i );
    if ( text == i18n( "(Default)" ) )
    {
	erase( imageX, imageY, imageW, imageH );
	return;
    }

    QString file = locate("wallpaper", text);

    if ( file != wallFile )
    {
	// debugT("Loading WallPaper '%s'\n",file.ascii());
	wallFile = file;
	wallPixmap.load( file );
    }
    
    if ( wallPixmap.isNull() )
	warning("Could not load wallpaper %s\n",file.ascii());
    
    erase( imageX, imageY, imageW, imageH );
    QPainter painter;
    painter.begin( this );
    painter.setClipRect( imageX, imageY, imageW, imageH );
    painter.drawPixmap( QPoint( imageX, imageY ), wallPixmap );
    painter.end();
}

void DirPropsPage::resizeEvent ( QResizeEvent *)
{
    imageX = 180; // X of the image (10 + width of the combobox + 10)
    // could be calculated in the future, depending on the length of the items
    // in the list.
    imageY = 90 + fontHeight; // so that combo & image are under the label
    imageW = width() - imageX - SEPARATION;
    imageH = height() - imageY - applyButton->height() - SEPARATION*2;

    iconBox->setGeometry( 10, 20, 50, 50 );
    wallBox->setGeometry( 10, imageY, imageX-20, 30 );
    browseButton->move( 10, wallBox->y()+wallBox->height()+SEPARATION );
    applyButton->move( 10, imageY+imageH+SEPARATION );
    globalButton->move( applyButton->x() + applyButton->width() + SEPARATION, applyButton->y() );
    
}

void DirPropsPage::slotApply()
{
    applyChanges();
}

void DirPropsPage::slotApplyGlobal()
{
    KConfig *config = KApplication::getKApplication()->getConfig();
    
    config->setGroup( "KFM HTML Defaults" );

    int i = wallBox->currentItem();
    if ( i != -1 )
    {
	if ( strcmp( wallBox->text( i ),  i18n("(None)") ) == 0 )
	    config->writeEntry( "BgImage", "" );
	else
	    config->writeEntry( "BgImage", wallBox->text( i ) );
    }

    config->sync();
}

/* ----------------------------------------------------
 *
 * ApplicationPropsPage
 *
 * -------------------------------------------------- */

ApplicationPropsPage::ApplicationPropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
    layout = new QBoxLayout(this, QBoxLayout::TopToBottom, SEPARATION);
    binaryPatternEdit = new QLineEdit( this, "LineEdit_1" );
    commentEdit = new QLineEdit( this, "LineEdit_2" );
    nameEdit = new QLineEdit( this, "LineEdit_3" );

    extensionsList = new QListBox( this );
    availableExtensionsList = new QListBox( this );
    addExtensionButton = new QPushButton( "<-", this );
    delExtensionButton = new QPushButton( "->", this );

    binaryPatternEdit->raise();
    binaryPatternEdit->setMinimumSize(210, fontHeight);
    binaryPatternEdit->setMaximumSize(QLayout::unlimited, fontHeight);
    binaryPatternEdit->setText( "" );
    binaryPatternEdit->setMaxLength( 512 );

    QLabel* tmpQLabel;
    tmpQLabel = new QLabel( this, "Label_1" );
    tmpQLabel->setText(  i18n("Binary Pattern (netscape;Netscape;)") );
    tmpQLabel->setFixedSize(tmpQLabel->sizeHint());
    layout->addWidget(tmpQLabel, 0, AlignLeft);

    layout->addWidget(binaryPatternEdit, 0, AlignLeft);

    tmpQLabel = new QLabel( this, "Label_3" );
    tmpQLabel->setText(  i18n("Comment") );
    tmpQLabel->setFixedSize(tmpQLabel->sizeHint());
    layout->addWidget(tmpQLabel, 0, AlignLeft);

    commentEdit->raise();
    commentEdit->setMinimumSize(210, fontHeight);
    commentEdit->setMaximumSize(QLayout::unlimited, fontHeight);
    commentEdit->setMaxLength( 256 );
    layout->addWidget(commentEdit, 0, AlignLeft);

    tmpQLabel = new QLabel( this, "Label_4" );
    tmpQLabel->setText(  i18n("Name ( in your language )") );
    tmpQLabel->setFixedSize(tmpQLabel->sizeHint());
    layout->addWidget(tmpQLabel, 0, AlignLeft);

    nameEdit->raise();
    nameEdit->setMaxLength( 256 );
    nameEdit->setMinimumSize(210, fontHeight);
    nameEdit->setMaximumSize(QLayout::unlimited, fontHeight);
    layout->addWidget(nameEdit, 0, AlignLeft);

    layoutH = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->addLayout(layoutH, 10);
    
    layoutH->addWidget(extensionsList, 10);

    layoutV = new QBoxLayout(QBoxLayout::TopToBottom);
    layoutH->addLayout(layoutV, 0);
    layoutV->addStretch(3);
    addExtensionButton->setFixedSize(40, 40);
    layoutV->addWidget(addExtensionButton, 0);
    layoutV->addStretch(3);
    connect( addExtensionButton, SIGNAL( pressed() ), 
	     this, SLOT( slotAddExtension() ) );
    delExtensionButton->setFixedSize(40, 40);
    layoutV->addWidget(delExtensionButton, 0);
    layoutV->addStretch(3);
    connect( delExtensionButton, SIGNAL( pressed() ), 
	     this, SLOT( slotDelExtension() ) );
    layoutH->addWidget(availableExtensionsList, 10);

    layout->activate();

    QString path = _props->kurl().path() ;
    //KURL::decodeURL( path );	    
    QFile f( path );
    if ( !f.open( IO_ReadOnly ) )
	return;
    f.close();

    KConfig config( path );
    config.setDesktopGroup();
    commentStr = config.readEntry( "Comment" );
    binaryPatternStr = config.readEntry( "BinaryPattern" );
    extensionsStr = config.readEntry( "MimeType" );
    nameStr = config.readEntry( "Name" );
    // Use the file name if no name is specified
    if ( nameStr.isEmpty() )
    {
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
    if ( !binaryPatternStr.isNull() )
	binaryPatternEdit->setText( binaryPatternStr );
    if ( !extensionsStr.isNull() )
    {
	int pos = 0;
	int pos2 = 0;
	while ( ( pos2 = extensionsStr.find( ';', pos ) ) != -1 )
	{
	    extensionsList->inSort( extensionsStr.mid( pos, pos2 - pos ) );
	    pos = pos2 + 1;
	}
    }

    addMimeType( "all" );
    addMimeType( "alldirs" );
    addMimeType( "allfiles" );

    QDictIterator<KMimeType> it ( KMimeType::mimeTypes() );
    for ( ; it.current(); ++it )
        addMimeType ( it.currentKey() );
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
#ifdef SVEN
// --- Sven's editable global settings changes start ---
    int i = 0;
    bool err = false;
// --- Sven's editable global settings changes end ---
#endif
    
    QString path = properties->kurl().path();

    QFile f( path );
    
#ifdef SVEN
    QDir lDir (kapp->localkdedir() + "/share/applnk/"); // I know it exists

    //debug (path.ascii());
    //debug (kapp->kde_appsdir().ascii());
#endif
    
    if ( !f.open( IO_ReadWrite ) )
    {
#ifdef SVEN
      // path = /usr/local/kde/share/applnk/network/netscape.kdelnk

      //Does path contain kde_appsdir?
      if (path.find(kapp->kde_appsdir()) == 0) // kde_appsdir on start of path
      {
	path.remove(0, strlen(kapp->kde_appsdir())); //remove kde_appsdir

	if (path[0] == '/')
	  path.remove(0, 1); // remove /

	while (path.contains('/'))
	{
	  i = path.find('/'); // find separator
	  if (!lDir.cd(path.left(i)))  // exists?
	  {
	    lDir.mkdir((path.left(i)));  // no, create
	    if (!lDir.cd((path.left(i)))) // can cd to?
	    {
	      err = true;                 // no flag it...
	      // debug ("Can't cd to  %s in %s", path.left(i).ascii(),
	      //	 lDir.absPath().ascii());
	      break;                      // and exit while
	    }
	    // Begin copy .directory if exists here.
	    // This block can be commented out without problems
	    // in case of problems.
	    {
	      QFile tmp(kapp->kde_appsdir() +
			'/' + path.left(i) + "/.directory");
	      //debug ("---- looking for: %s", tmp.name());
	      if (tmp.open( IO_ReadOnly))
	      {
		//debug ("--- opened RO");
		char *buff = new char[tmp.size()+10];
		if (buff != 0)
		{
		  if (tmp.readBlock(buff, tmp.size()) != -1)
		  {
		    size_t tmpsize = tmp.size();
		    //debug ("--- read");
		    tmp.close();
		    tmp.setName(lDir.absPath() + "/.directory");
		    //debug ("---- copying to: %s", tmp.name());
		    if (tmp.open(IO_ReadWrite))
		    {
		      //debug ("--- opened RW");
		      if (tmp.writeBlock(buff, tmpsize) != -1)
		      {
			//debug ("--- wrote");
			tmp.close();
		      }
		      else
		      {
                        //debug ("--- removed");
			tmp.remove();
		      }
		    }                 // endif can open to write
		  }                   // endif can read
		  else     //coulnd't read
		    tmp.close();

		  delete[] buff;
		}                     // endif is alocated
	      }                       // can open to write
	    }
	    // End coping .directory file
	    
	  }
	  path.remove (0, i);           // cded to;
	  if (path[0] == '/')
	    path.remove(0, 1); // remove / from path
	}
      }
      else // path didn't contain kde_appsdir - this is an error
	err = true;

      // we created all subdirs or failed
      if (!err) // if we didn't fail try to make file just for check
      {
	path.prepend('/'); // be sure to have /netscape.kdelnk
	path.prepend(lDir.absPath());
	f.setName(path);
	//debug("path = %s", path.ascii());
	if ( f.open( IO_ReadWrite ) )
	{
	  // we must first copy whole kdelnk to local dir
	  // and then change it. Trust me.
	  QFile s(properties->kurl().path());
	  s.open(IO_ReadOnly);
	  //char *buff = (char *) malloc (s.size()+10);   CHANGE TO NEW!
	  char *buff = new char[s.size()+10];           // Done.
	  if (buff != 0)
	  {
	    //debug ("********About to copy");
	    if (s.readBlock(buff, s.size()) != -1 &&
		f.writeBlock(buff, s.size()) != -1)
	      ; // ok
	    else
	    {
	      err = true;
	      f.remove();
	    }
	    //free ((void *) buff);                      CHANGE TO DELETE!
	    delete[] buff;                            // Done.
	  }
	  else
	    err = true;
	}
	else
	  err = true;
      }
      if (err)
      {
	//debug ("************Cannot save");
#endif

	QMessageBox::warning( 0, i18n("Properties Dialog Error"),
			        i18n("Could not save properties\nPerhaps permissions denied"),
				i18n("OK") );
	return;
#ifdef SVEN
      }
#endif
    }
    
    f.close(); // kalle
    KConfig config( path ); // kalle
    config.setDesktopGroup();
    config.writeEntry( "Comment", commentEdit->text(), true, false, true );

    QString tmp = binaryPatternEdit->text();
    if ( tmp.length() > 0 )
	if ( tmp.at(tmp.length() - 1) != ';' )
	    tmp += ';';
    config.writeEntry( "BinaryPattern", tmp );

    extensionsStr = "";
    for ( uint i = 0; i < extensionsList->count(); i++ )
    {
	extensionsStr += extensionsList->text( i );
	extensionsStr += ';';
    }
    config.writeEntry( "MimeType", extensionsStr );
    config.writeEntry( "Name", nameEdit->text(), true, false, true );
    
    config.sync();
    f.close();

    /*
    KMimeType::clearAll();
    KMimeType::init();
    if ( KRootWidget::getKRootWidget() )
	KRootWidget::getKRootWidget()->update();

    KfmGui *win;
    for ( win = KfmGui::getWindowList().first(); win != 0L; win = KfmGui::getWindowList().next() )
	win->updateView();
    */
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
    patternEdit = new QLineEdit( this, "LineEdit_1" );
    commentEdit = new QLineEdit( this, "LineEdit_3" );
    mimeEdit = new QLineEdit( this, "LineEdit_3" );
    iconBox = new KIconLoaderButton( KGlobal::iconLoader(), this );
    appBox = new QComboBox( false, this, "ComboBox_2" );

    QBoxLayout * mainlayout = new QVBoxLayout(this, SEPARATION);
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

    QHBoxLayout * hlayout = new QHBoxLayout(SEPARATION);
    mainlayout->addLayout(hlayout, 2); // double stretch, because two items

    QVBoxLayout * vlayout; // a vertical layout for the two following items
    vlayout = new QVBoxLayout(SEPARATION);
    hlayout->addLayout(vlayout, 1);

    tmpQLabel = new QLabel( this, "Label_2" );
    tmpQLabel->setText(  i18n("Default Application") );
    tmpQLabel->setMinimumSize(tmpQLabel->sizeHint());
    vlayout->addWidget(tmpQLabel, 1);

    appBox->setFixedHeight( fontHeight );
    vlayout->addWidget(appBox, 1);

    iconBox->setFixedSize( 50, 50 );
    hlayout->addWidget(iconBox, 0);

    mainlayout->addSpacing(fontMetrics().height());
    mainlayout->activate();

    QFile f( _props->kurl().path() );
    if ( !f.open( IO_ReadOnly ) )
	return;
    f.close();

    KConfig config( _props->kurl().path() );
    config.setDesktopGroup();
    QString patternStr = config.readEntry( "Patterns" );
    QString appStr = config.readEntry( "DefaultApp" );
    QString iconStr = config.readEntry( "Icon" );
    QString commentStr = config.readEntry( "Comment" );
    QString mimeStr = config.readEntry( "MimeType" );

    if ( !patternStr.isEmpty() )
	patternEdit->setText( patternStr );
    if ( !commentStr.isEmpty() )
	commentEdit->setText( commentStr );
    if ( iconStr.isEmpty() )
        iconStr = KMimeType::mimeType("")->KServiceType::icon(); // default icon
    if ( !mimeStr.isEmpty() )
	mimeEdit->setText( mimeStr );
    
    iconBox->setIcon( iconStr );
    
    // Get list of all applications
    QStringList applist;
    QString currApp;
    appBox->insertItem( i18n("<none>") );
    QListIterator<KService> it ( KService::services() );
    for ( ; it.current(); ++it )
    {
	currApp = it.current()->name();

	// list every app only once
	if ( applist.find( currApp ) == applist.end() ) { 
	    appBox->insertItem( currApp );
	    applist.append( currApp );
	}
    }
    
    // TODO: Torben: No default app any more here.
    // Set the default app (DefaultApp=... is the kdelnk name)
    // QStringList::Iterator it = applist.find( appStr );
    // debug ("appStr = %s\n\n",appStr.ascii());
    // if ( it == applist.end() )
    // appBox->setCurrentItem( 0 );
    // index = 0;
    // appBox->setCurrentItem( index );
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
#ifdef SVEN
// --- Sven's editable global settings changes start ---
    int i = 0;
    bool err = false;
    // --- Sven's editable global settings changes end ---
#endif
    
    QString path = properties->kurl().path();

    QFile f( path );

#ifdef SVEN
    QDir lDir (kapp->localkdedir() + "/share/mimelnk/"); // I know it exists

    //debug (path.ascii());
    //debug (kapp->kde_mimedir().ascii());
#endif
    
    if ( !f.open( IO_ReadWrite ) )
    {
#ifdef SVEN
      // path = /usr/local/kde/share/mimelnk/image/jpeg.kdelnk
      
      //Does path contain kde_mimedir?
      if (path.find(kapp->kde_mimedir()) == 0) // kde_mimedir on start of path
      {
	path.remove(0, strlen(kapp->kde_mimedir())); //remove kde_mimedir

	if (path[0] == '/')
	  path.remove(0, 1); // remove /
	
	while (path.contains('/'))
	{
	  i = path.find('/'); // find separator
	  if (!lDir.cd(path.left(i)))  // exists?
	  {
	    lDir.mkdir((path.left(i)));  // no, create
	    if (!lDir.cd((path.left(i)))) // can cd to?
	    {
	      err = true;                 // no flag it...
	      // debug ("Can't cd to  %s in %s", path.left(i).ascii(),
	      //	 lDir.absPath().ascii());
	      break;                      // and exit while
	    }
	    // Begin copy .directory if exists here.
	    // This block can be commented out without problems
	    // in case of problems.
	    {
	      QFile tmp(kapp->kde_mimedir() +
			'/' + path.left(i) + "/.directory");
	      //debug ("---- looking for: %s", tmp.name());
	      if (tmp.open( IO_ReadOnly))
	      {
		//debug ("--- opened RO");
		char *buff = new char[tmp.size()+10];
		if (buff != 0)
		{
		  if (tmp.readBlock(buff, tmp.size()) != -1)
		  {
		    size_t tmpsize = tmp.size();
		    //debug ("--- read");
		    tmp.close();
		    tmp.setName(lDir.absPath() + "/.directory");
		    //debug ("---- copying to: %s", tmp.name());
		    if (tmp.open(IO_ReadWrite))
		    {
		      //debug ("--- opened RW");
		      if (tmp.writeBlock(buff, tmpsize) != -1)
		      {
			//debug ("--- wrote");
			tmp.close();
		      }
		      else
		      {
                        //debug ("--- removed");
			tmp.remove();
		      }
		    }                 // endif can open to write
		  }                   // endif can read
		  else     //coulnd't read
		    tmp.close();

		  delete[] buff;
		}                     // endif is alocated
	      }                       // can open to write
	    }
	    // End coping .directory file
	  }
	  path.remove (0, i);           // cded to;
	  if (path[0] == '/')
	    path.remove(0, 1); // remove / from path
	}
      }
      else // path didn't contain kde_mimdir this is an error
	err = true;
      
      // we created all subdirs or failed
      if (!err) // if we didn't fail try to make file just for check
      {
	path.prepend('/'); // be sure to have /jpeg.kdelnk
	path.prepend(lDir.absPath());
	f.setName(path);
	//debug("path = %s", path.ascii());
	if ( f.open( IO_ReadWrite ) )
	{
	  // we must first copy whole kdelnk to local dir
	  // and then change it. Trust me.
	  QFile s(properties->kurl().path());
	  s.open(IO_ReadOnly);
          char *buff = new char[s.size()+10];
	  if (buff != 0)
	  {
	    if (s.readBlock(buff, s.size()) != -1 &&
		f.writeBlock(buff, s.size()) != -1)
	      ; // ok
	    else
	    {
	      err = true;
	      f.remove();
	    }
	    delete[] buff;
	  }
	  else
	    err = true;
	}
	else
	  err = true;
      }
      if (err)
      {
#endif
	QMessageBox::warning( 0, i18n("Properties Dialog Error"),
			        i18n("Could not save properties\nPerhaps permissions denied"),
				i18n("OK") );
	return;
#ifdef SVEN
      }
#endif
    }
    f.close();

    KConfig config( path );
    config.setDesktopGroup();
    
    QString tmp = patternEdit->text();
    if ( tmp.length() > 1 )
	if ( tmp.at(tmp.length() - 1) != ';' )
	    tmp += ';';
    config.writeEntry( "Patterns", tmp );
    config.writeEntry( "Comment", commentEdit->text(), true, false, true );
    config.writeEntry( "MimeType", mimeEdit->text() );
    config.writeEntry( "Icon", iconBox->icon() );
    // Torben: TODO: Now default app any more
    // config.writeEntry( "DefaultApp", kdelnklist.at( appBox->currentItem() ) );

    config.sync();

/*
    KMimeType::clearAll();
    KMimeType::init();
    if ( KRootWidget::getKRootWidget() )
	KRootWidget::getKRootWidget()->update();

    KfmGui *win;
    for ( win = KfmGui::getWindowList().first(); win != 0L; win = KfmGui::getWindowList().next() )
	win->updateView();
*/
}

/* ----------------------------------------------------
 *
 * DevicePropsPage
 *
 * -------------------------------------------------- */

DevicePropsPage::DevicePropsPage( PropertiesDialog *_props ) : PropsPage( _props )
{
    bool IamRoot = (geteuid() == 0);

    QLabel* tmpQLabel;
    tmpQLabel = new QLabel( this, "Label_1" );
    tmpQLabel->move( 10, 10 );
    tmpQLabel->setText(  i18n("Device ( /dev/fd0 )") );
    tmpQLabel->adjustSize();
    
    device = new QLineEdit( this, "LineEdit_1" );
    device->setGeometry( 10, 40, 180, 30 );
    device->setText( "" );
    
    tmpQLabel = new QLabel( this, "Label_2" );
    tmpQLabel->move( 10, 80 );
    tmpQLabel->setText(  i18n("Mount Point ( /floppy )") );
    tmpQLabel->adjustSize();

    mountpoint = new QLineEdit( this, "LineEdit_2" );
    mountpoint->setGeometry( 10, 110, 180, 30 );
    mountpoint->setText( "" );
    if ( !IamRoot )
	mountpoint->setEnabled( false );
    
    readonly = new QCheckBox( this, "CheckBox_1" );
    readonly->setGeometry( 220, 40, 130, 30 );
    readonly->setText(  i18n("Readonly") );
    if ( !IamRoot )
	readonly->setEnabled( false );
    
    tmpQLabel = new QLabel( this, "Label_4" );
    tmpQLabel->move( 10, 150 );
    tmpQLabel->setText(  i18n("Filesystems ( iso9660,msdos,minix,default )") );
    tmpQLabel->adjustSize();

    fstype = new QLineEdit( this, "LineEdit_3" );
    fstype->setGeometry( 10, 180, 280, 30 );
    fstype->setText( "" );
    if ( !IamRoot )
	fstype->setEnabled( false );

    tmpQLabel = new QLabel( this, "Label_5" );
    tmpQLabel->move( 10, 220 );
    tmpQLabel->setText(  i18n("Mounted Icon") );
    tmpQLabel->adjustSize();

    tmpQLabel = new QLabel( this, "Label_6" );
    tmpQLabel->move( 170, 220 );
    tmpQLabel->setText(  i18n("Unmounted Icon") );
    tmpQLabel->adjustSize();
    
    mounted = new KIconLoaderButton( KGlobal::iconLoader(), this );
    mounted->setGeometry( 10, 250, 50, 50 );
    
    unmounted = new KIconLoaderButton( KGlobal::iconLoader(), this );
    unmounted->setGeometry( 170, 250, 50, 50 );

    QString path( _props->kurl().path() );
    
    QFile f( path );
    if ( !f.open( IO_ReadOnly ) )
	return;
    f.close();

    KConfig config( path );
    config.setDesktopGroup();
    deviceStr = config.readEntry( "Dev" );
    mountPointStr = config.readEntry( "MountPoint" );
    readonlyStr = config.readEntry( "ReadOnly" );
    fstypeStr = config.readEntry( "FSType" );
    mountedStr = config.readEntry( "Icon" );
    unmountedStr = config.readEntry( "UnmountIcon" );

    if ( !deviceStr.isNull() )
	device->setText( deviceStr );
    if ( !mountPointStr.isNull() )
	mountpoint->setText( mountPointStr );
    if ( !fstypeStr.isNull() )
	fstype->setText( fstypeStr );
    if ( readonlyStr == "0" )
	readonly->setChecked( false );
    else
	readonly->setChecked( true );
    if ( mountedStr.isEmpty() )
        mountedStr = KMimeType::mimeType("")->KServiceType::icon(); // default icon
    if ( unmountedStr.isEmpty() )
        unmountedStr = KMimeType::mimeType("")->KServiceType::icon(); // default icon

    mounted->setIcon( mountedStr ); 
    unmounted->setIcon( unmountedStr ); 
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
	QMessageBox::warning( 0, i18n("Properties Dialog Error"), 
			        i18n("Could not save properties\nPerhaps permissions denied"),
				i18n("OK") );
	return;
    }
    f.close();

    KConfig config( path );
    config.setDesktopGroup();
    
    config.writeEntry( "Dev", device->text() );
    config.writeEntry( "MountPoint", mountpoint->text() );
    config.writeEntry( "FSType", fstype->text() );
    
    config.writeEntry( "Icon", mounted->icon() );
    config.writeEntry( "UnmountIcon", unmounted->icon() );
    
    if ( readonly->isChecked() )
	config.writeEntry( "ReadOnly", "1" );
    else
	config.writeEntry( "ReadOnly", "0" );

    config.sync();
}

#include "kpropsdlg.moc"
