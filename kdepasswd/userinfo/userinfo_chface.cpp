/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/
/**
 * @file UserInfo's Dialog for changing your face.
 * @author Braden MacDonald
 */
#include <qstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qpushbutton.h>
#include <qdir.h>
#include <qcheckbox.h>

#include <kdialogbase.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kiconview.h>
#include <kimagefilepreview.h>
#include <kimageio.h>
#include <kmessagebox.h>

#include <konq_operations.h>
#include <kurl.h>

#include "userinfo_chface.h"
#include "userinfo_constants.h"



KUserInfoChFaceDlg::KUserInfoChFaceDlg(const QString& picsdir, QWidget *parent, const char *name, bool modal)
  : KDialogBase( parent, name, modal, i18n("Change your Face"), Ok|Cancel, Ok, true )
{
  QWidget *page = new QWidget(this);
  setMainWidget( page );

  QVBoxLayout *top = new QVBoxLayout(page, 0, spacingHint());

  QLabel *header = new QLabel( i18n("Select a new face:"), page );
  top->addWidget( header );

  m_FacesWidget = new KIconView( page );
  m_FacesWidget->setSelectionMode( QIconView::Single );
  m_FacesWidget->setItemsMovable( false );
  m_FacesWidget->setMinimumSize( 400, 200 );

  connect( m_FacesWidget, SIGNAL( selectionChanged( QIconViewItem * ) ), SLOT( slotFaceWidgetSelectionChanged( QIconViewItem * ) ) );

  connect( m_FacesWidget, SIGNAL( doubleClicked( QIconViewItem *, const QPoint & ) ), SLOT( slotOk() ) );

  top->addWidget( m_FacesWidget );

  // Buttons to get more pics
  QHBoxLayout * morePics = new QHBoxLayout( 0, 0, spacingHint() );
  QPushButton *browseBtn = new QPushButton( i18n("Custom &Image..."), page );
  connect( browseBtn, SIGNAL( clicked() ), SLOT( slotGetCustomImage() ) );
  morePics->addWidget( browseBtn );
#if 0
  QPushButton *acquireBtn = new QPushButton( i18n("&Acquire Image..."), page );
  acquireBtn->setEnabled( false );
  morePics->addWidget( acquireBtn );
#endif
  morePics->addStretch();
  top->addLayout( morePics );

  // Filling the icon view
  QDir facesDir( picsdir );
  if ( facesDir.exists() )
  {
    QStringList picslist = facesDir.entryList( QDir::Files );
    for ( QStringList::Iterator it = picslist.begin(); it != picslist.end(); ++it )
      new QIconViewItem( m_FacesWidget, (*it).section(".",0,0), QPixmap( picsdir + *it ) );
  }
  facesDir.setPath( QDir::homeDirPath() + USER_FACES_DIR );
  if ( facesDir.exists() )
  {
    QStringList picslist = facesDir.entryList( QDir::Files );
    for ( QStringList::Iterator it = picslist.begin(); it != picslist.end(); ++it )
      new QIconViewItem( m_FacesWidget, "/"+(*it) == USER_CUSTOM_FILE ? i18n("(Custom)") : (*it).section(".",0,0),
                                                    QPixmap( QDir::homeDirPath() + USER_FACES_DIR + *it ) );
  }

  m_FacesWidget->setResizeMode( QIconView::Adjust );
  //m_FacesWidget->setGridX( FACE_PIX_SIZE - 10 );
  m_FacesWidget->arrangeItemsInGrid();

  enableButtonOK( false );
#if 0
  connect( this, SIGNAL( okClicked() ), SLOT( slotSaveCustomImage() ) );
#endif
  resize( 420, 400 );
}

void KUserInfoChFaceDlg::addCustomPixmap( QString imPath, bool saveCopy )
{
  QImage pix( imPath );
  // TODO: save pix to TMPDIR/userinfo-tmp,
  // then scale and copy *that* to ~/.faces

  if (pix.isNull())
  {
    KMessageBox::sorry( this, i18n("There was an error loading the image.") );
    return;
  }
  if ( (pix.width() > FACE_PIX_SIZE) || (pix.height() > FACE_PIX_SIZE) )
    pix = pix.scale( FACE_PIX_SIZE, FACE_PIX_SIZE, QImage::ScaleMin );// Should be no bigger than certain size.

  if ( saveCopy )
  {
    // If we should save a copy:
    QDir userfaces( QDir::homeDirPath() + USER_FACES_DIR );
    if ( !userfaces.exists( ) )
      userfaces.mkdir( userfaces.absPath() );

    pix.save( userfaces.absPath() + "/.userinfo-tmp" , "PNG" );
    KonqOperations::copy( this, KonqOperations::COPY, KURL::List( KURL( userfaces.absPath() + "/.userinfo-tmp" ) ), KURL( userfaces.absPath() + "/" + QFileInfo(imPath).fileName().section(".",0,0) ) );
#if 0
  if ( !pix.save( userfaces.absPath() + "/" + imPath , "PNG" ) )
    KMessageBox::sorry(this, i18n("There was an error saving the image:\n%1").arg( userfaces.absPath() ) );
#endif
  }

  QIconViewItem* newface = new QIconViewItem( m_FacesWidget, QFileInfo(imPath).fileName().section(".",0,0) , pix );
  newface->setKey( USER_CUSTOM_KEY );// Add custom items to end
  m_FacesWidget->ensureItemVisible( newface );
  m_FacesWidget->setCurrentItem( newface );
}

void KUserInfoChFaceDlg::slotGetCustomImage(  )
{
  QCheckBox* checkWidget = new QCheckBox( i18n("&Save copy in custom faces folder for future use"), 0 );

  KFileDialog *dlg = new KFileDialog( QDir::homeDirPath(), KImageIO::pattern( KImageIO::Reading ),
                  this, 0, true, checkWidget);

  dlg->setOperationMode( KFileDialog::Opening );
  dlg->setCaption( i18n("Choose Image") );
  dlg->setMode( KFile::File | KFile::LocalOnly );

  KImageFilePreview *ip = new KImageFilePreview( dlg );
  dlg->setPreviewWidget( ip );
  if (dlg->exec() == QDialog::Accepted)
      addCustomPixmap( dlg->selectedFile(), checkWidget->isChecked() );
  // Because we give it a parent we have to close it ourselves.
  dlg->close(true);
}

#if 0
void KUserInfoChFaceDlg::slotSaveCustomImage()
{
  if ( m_FacesWidget->currentItem()->key() ==  USER_CUSTOM_KEY)
  {
    QDir userfaces( QDir::homeDirPath() + USER_FACES_DIR );
    if ( !userfaces.exists( ) )
      userfaces.mkdir( userfaces.absPath() );

    if ( !m_FacesWidget->currentItem()->pixmap()->save( userfaces.absPath() + USER_CUSTOM_FILE , "PNG" ) )
      KMessageBox::sorry(this, i18n("There was an error saving the image:\n%1").arg( userfaces.absPath() ) );
  }
}
#endif

#include "userinfo_chface.moc"
