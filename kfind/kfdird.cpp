/***********************************************************************
 *
 *  KfDirD.cpp
 *
 **********************************************************************/

#include <qlistbox.h>
#include <qlined.h>
#include <qstring.h>
#include <qcombo.h>
#include <qlabel.h>
#include <qpushbt.h>
#include <qmsgbox.h>
#include <qapp.h>

#include "kfdird.h"

#include <klocale.h>
#define klocale KLocale::klocale()

KfDirDialog::KfDirDialog( QWidget *parent, const char *name, bool modal )
    : QDialog( parent, name, modal )
{
    init();
    d.convertToAbs();
    rereadDir();
    resize( 200, 300 );
};

KfDirDialog::KfDirDialog( const char *dirName,
			  QWidget *parent, const char *name, bool modal )
    : QDialog( parent, name, modal )
{
    init();
    if ( dirName )
	d.setPath( dirName );
    d.convertToAbs();
    rereadDir();
    resize( 200, 300 );
};

void KfDirDialog::init()
{
    pathBox    = new QComboBox(		     this, "pathBox"	  );
    dirs       = new QListBox(		     this, "dirList"	  );
    dirL       = new QLabel( klocale->translate("Directories:"), this, "dirLabel"	  );
    okB	       = new QPushButton( klocale->translate("OK"), this, "okButton"	  );
    cancelB    = new QPushButton( klocale->translate("Cancel") , this, "cancelButton" );

    pathBox->setAutoResize( TRUE );
    dirL   ->setAutoResize( TRUE );

    okB	   ->resize( 50, 25 );
    cancelB->resize( 50, 25 );

    connect( dirs,	SIGNAL(selected(int)),	 SLOT(dirSelected(int)) );
    connect( pathBox,	SIGNAL(activated(int)),	 SLOT(pathSelected(int)) );
    connect( okB,	SIGNAL(clicked()),	 SLOT(okClicked()) );
    connect( cancelB,	SIGNAL(clicked()),	 SLOT(cancelClicked()) );
    d.setMatchAllDirs( TRUE );
    d.setSorting( d.sorting() | QDir::DirsFirst );

    //    setMinimumSize(200,300);
};

KfDirDialog::~KfDirDialog()
{
    delete dirs;
    delete pathBox;
    delete dirL;
    delete okB;
    delete cancelB;
}

/*!
  Returns the active directory path string in the file dialog.
  \sa dir(), setDir()
*/

const char *KfDirDialog::dirPath() const
{
    return d.path();
}

/*!
  Sets a directory path string for the file dialog.
  \sa dir()
*/

void KfDirDialog::setDir( const char *pathstr )
{
    if ( strcmp(d.path(),pathstr) == 0 )
	return;
    d.setPath( pathstr );
    d.convertToAbs();
    rereadDir();
}

/*!
  Returns the active directory in the file dialog.
  \sa setDir()
*/

const QDir *KfDirDialog::dir() const
{
    return &d;
}

/*!
  Returns the selected dir name.
*/

QString KfDirDialog::selectedDir() const
{
    QString tmp;

    if (dirs->currentItem()!=-1)
        {
          tmp.sprintf("%s/%s",d.path(),dirs->text((dirs->currentItem())));
          tmp= d.cleanDirPath(tmp); 
        }
      else
        tmp = d.path();

    return tmp;
}
 
/*!
  Sets a directory path for the file dialog.
  \sa dir()
*/

void KfDirDialog::setDir( const QDir &dir )
{
    d = dir;
    d.convertToAbs();
    d.setMatchAllDirs( TRUE );
    d.setSorting( d.sorting() | QDir::DirsFirst );
    rereadDir();
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
    dirs ->setAutoUpdate( FALSE );
    dirs ->clear();

    const QFileInfoList	 *filist = d.entryInfoList();
    if ( filist ) {
	QFileInfoListIterator it( *filist );
	QFileInfo		 *fi = it.current();
	while ( fi && fi->isDir() ) {
	    dirs->insertItem( fi->fileName().data() );
	    fi = ++it;
	}
    } else {
	qApp->restoreOverrideCursor();
	QMessageBox::message( klocale->translate("Sorry"), 
			      klocale->translate("Cannot open or read directory.") );
	qApp ->setOverrideCursor( waitCursor );
    }
    dirs ->setAutoUpdate( TRUE );
    dirs ->repaint();
    updatePathBox( d.path() );
    qApp->restoreOverrideCursor();
}



/*!!!!!!!!!!!!!!!!!!!!!!!!!!****************!!!!!!!!!!
  \internal
  Activated when a file name in the file list has been highlighted.
*/

void KfDirDialog::dirHighlighted( int index )
  {
     QDir tmp = d;
     if ( d.cd( dirs->text(index) ) && d.isReadable() ) {
       //	 nameEdit->setText( "" );
	 emit dirEntered( d.path() );
	 rereadDir();
       } else {
	 QMessageBox::message( klocale->translate("Sorry"), 
			       klocale->translate("Cannot open or read directory.") );
	 d = tmp;
     };
}

/*!
  \internal
  Activated when a directory name in the directory list has been selected.
*/

void KfDirDialog::dirSelected( int index )
{
    QDir tmp = d;
    if ( d.cd( dirs->text(index) ) && d.isReadable() ) {
	emit dirEntered( d.path() );
	rereadDir();
    } else {
	QMessageBox::message( klocale->translate("Sorry"), 
			      klocale->translate("Cannot open or read directory.") );
	d = tmp;
    }
}

void KfDirDialog::pathSelected( int index )
{
    if ( index == 0 )				// current directory shown
	return;
    QString newPath;
    QDir tmp = d;
    for( int i = pathBox->count() - 1 ; i >= index ; i-- )
	newPath += pathBox->text( i );
    d.setPath( newPath );
    if ( d.isReadable() ) {
	rereadDir();
    } else {
	d = tmp;
    }
}


/*!
  \internal
  Activated when the "Ok" button is clicked.
*/

void KfDirDialog::okClicked()
  {
    emit dirSelected( d.path() );
    accept();
  };

/*!
  \internal
  Activated when the "Cancel" button is clicked.
*/

void KfDirDialog::cancelClicked()
{
    reject();
}


/*!
  Handles resize events for the file dialog.
*/

void KfDirDialog::resizeEvent( QResizeEvent * )
{
    int w = width();
    int h = height();
    int	  wTmp;
    QRect rTmp;

    rTmp = *(new QRect(10,0,0,0));
    wTmp = pathBox->width();
    pathBox->move( (w - wTmp)/2, rTmp.bottom() + 10 );

    rTmp = pathBox->geometry();
    dirL->move( 10, rTmp.bottom() + 5 );

    rTmp = dirL->geometry();
    dirs->setGeometry(10, rTmp.bottom() + 5,
		      w - 20, h - rTmp.bottom() - 10 - 25 - 10 - 20 - 10 );

    rTmp = dirs->geometry();
    okB->move( 10, rTmp.bottom() + 10 );

    rTmp = okB->geometry();
    wTmp = cancelB->width();
    cancelB->move( w - 10 - wTmp, rTmp.y() );

}

/*!
  \internal
  Updates the path box.	 Called from rereadDir().
*/
void KfDirDialog::updatePathBox( const char *s )
{
    QStrList l;
    QString tmp;
    QString safe = s;

    l.insert( 0, "/" );
    tmp = strtok( safe.data(), "/" );
    while ( TRUE ) {
	if ( tmp.isNull() )
	    break;
	l.insert( 0, tmp + "/" );
	tmp = strtok( 0, "/" );
    }
    pathBox->setUpdatesEnabled( FALSE );
    pathBox->clear();
    pathBox->insertStrList( &l );
    pathBox->setCurrentItem( 0 );
    pathBox->move( (width() - pathBox->width()) / 2, pathBox->geometry().y() );
    pathBox->setUpdatesEnabled( TRUE );
    pathBox->repaint();
}



