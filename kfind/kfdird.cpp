/***********************************************************************
 *
 *  KfDirD.cpp
 *
 **********************************************************************/

#include <qapplication.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qstring.h>

#include <kapp.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kfdird.h"


KfDirDialog::KfDirDialog( const QString& dirName,
			  QWidget *parent, const char *name, bool modal )
  : KDialogBase( parent, name, modal, i18n("Select directory"), Ok|Cancel, Ok )
{
  init();
  if ( !dirName.isNull() )
    d.setPath( dirName );
  d.convertToAbs();
  rereadDir();
  setInitialSize( QSize(250, 300) );
}



void KfDirDialog::init( void )
{
  QWidget *page = new QWidget( this );
  setMainWidget(page);
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  pathL   = new QLabel( i18n("Current directory:"), page, "pathLabel" );
  path    = new QLineEdit( page, "path" );
  dirL    = new QLabel( i18n("Directories:"), page, "dirLabel" );
  dirs    = new QListBox( page, "dirList" );

  topLayout->addWidget(pathL);
  topLayout->addWidget(path);
  topLayout->addSpacing( spacingHint() );
  topLayout->addWidget(dirL);
  topLayout->addWidget(dirs);

  connect( dirs, SIGNAL(selected(int)),	
	   this, SLOT(dirSelected(int)) );
  connect( path, SIGNAL(returnPressed()),
	   this, SLOT(pathSelected()));
  d.setMatchAllDirs( TRUE );
  d.setSorting( d.sorting() | QDir::DirsFirst );
}






#if 0
KfDirDialog::KfDirDialog( const QString& dirName,
			  QWidget *parent, const char *name, bool modal )
  : QDialog( parent, name, modal )
{
  init();
  if ( !dirName.isNull() )
    d.setPath( dirName );
  d.convertToAbs();
  rereadDir();
  resize( 200, 300 );
}



void KfDirDialog::init()
{
  setCaption(i18n("Select directory"));

  pathL   = new QLabel( i18n("Current directory:"), this, "pathLabel" );
  path    = new QLineEdit( this, "path" );
  dirL    = new QLabel( i18n("Directories:"), this, "dirLabel" );
  dirs    = new QListBox( this, "dirList" );
  okB	  = new QPushButton( i18n("OK"), this, "okButton" );
  cancelB = new QPushButton( i18n("Cancel") , this, "cancelButton" );

  QVBoxLayout *vbox = new QVBoxLayout(this, 5);
  vbox->addWidget(pathL);
  vbox->addWidget(path);
  vbox->addSpacing(5);
  vbox->addWidget(dirL);
  vbox->addWidget(dirs);
  QHBoxLayout *hbox = new QHBoxLayout(vbox);
  hbox->addWidget(okB);
  hbox->addStretch(1);
  hbox->addWidget(cancelB);
  vbox->activate();

  connect( dirs, SIGNAL(selected(int)),	
	   this, SLOT(dirSelected(int)) );
  connect( path, SIGNAL(returnPressed()),
	   this, SLOT(pathSelected()));
  connect( okB,	SIGNAL(clicked()),	
	   this, SLOT(accept()) );
  connect( cancelB, SIGNAL(clicked()),
	   this, SLOT(reject()) );
  d.setMatchAllDirs( TRUE );
  d.setSorting( d.sorting() | QDir::DirsFirst );
}
#endif


KfDirDialog::~KfDirDialog()
{}

/*!
  Returns the selected dir name.
*/

QString KfDirDialog::selectedDir() const
{
  QString tmp;

  if (dirs->currentItem()!=-1)
    {
      tmp = QString("%1/%2")
	.arg(d.path())
	.arg(dirs->text((dirs->currentItem())));
      tmp= d.cleanDirPath(tmp);
    }
  else
    tmp = d.path();

  return tmp;
}

/*!
  Re-reads the active directory in the file dialog.

  It is seldom necessary to call this function.	 It is provided in
  case the directory contents change and you want to refresh the
  directory list box.
*/

void KfDirDialog::rereadDir()
{
  qApp ->setOverrideCursor( waitCursor );
  dirs ->clear();

  const QFileInfoList	 *filist = d.entryInfoList();
  if ( filist ) {
    QFileInfoListIterator it( *filist );
    QFileInfo		 *fi = it.current();
    while ( fi && fi->isDir() ) {
      dirs->insertItem( fi->fileName() );
      fi = ++it;
    }
  } else {
    qApp->restoreOverrideCursor();
    KMessageBox::sorry( this, i18n("Cannot open or read directory."));
    qApp ->setOverrideCursor( waitCursor );
  }
  dirs ->repaint();
  path->setText( d.path() );
  qApp->restoreOverrideCursor();
}

/*!
  \internal
  Activated when a directory name in the directory list has been selected.
*/

void KfDirDialog::dirSelected( int index )
{
  checkDir(dirs->text(index), false);
}

void KfDirDialog::pathSelected()
{
  checkDir(path->text(), true);
}

/*!
  \internal
  Checks dir validity
*/

void KfDirDialog::checkDir(const QString& subdir, bool abs)
{
  QDir tmp = d;
  if ( tmp.cd( subdir, abs) && tmp.isReadable()) {
    d = tmp;
    rereadDir();
    return;
  }

  KMessageBox::sorry(this, i18n("Cannot open or read directory."));
}
