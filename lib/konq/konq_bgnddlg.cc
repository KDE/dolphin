/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (c) 1999 David Faure <faure@kde.org>

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

#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>

#include <kcolorbutton.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kimagefilepreview.h>
#include <krecentdocument.h>

#include "konq_bgnddlg.h"


KonqBgndDialog::KonqBgndDialog( const QString & pixmapFile, KInstance *instance,
                                QColor &theColor, const QColor& defaultColor )
  : KDialogBase( Plain,
		 i18n("Configure Background Settings"),
                 Ok|Cancel,
                 Ok,
		 0L, // no parent,
                 "KonqBgndDialog",
                 true, //modal
                 false, // no separator
                 i18n( "Set as default" )
    )
{
    KGlobal::dirs()->addResourceType("tiles",
                                     KGlobal::dirs()->kde_default("data") + "konqueror/tiles/");
    kdDebug(1203) << KGlobal::dirs()->kde_default("data") + "konqueror/tiles/" << endl;
    QFrame *page = plainPage();
    QLayout *layout = new QVBoxLayout( page, 0, KDialog::spacingHint() );
    layout->setAutoAdd( true );
    
    QHBox *hbox =  new QHBox(page);
    QLabel *label = new QLabel(i18n("What shall define the background:")+" ", hbox);
    combobox = new QComboBox(false, hbox);
    combobox->insertItem(i18n("Color"));
    combobox->insertItem(i18n("Image"));

    colorbox =  new QHBox(page);
    new QLabel(i18n("Background color:"), colorbox);
    colorbutton = new KColorButton(theColor, defaultColor, colorbox);
    
    m_propsPage = new KBgndDialogPage( page, pixmapFile, instance, "tiles" );
    
    if (pixmapFile.isEmpty()) {
      combobox->setCurrentItem(0);
      m_propsPage->setEnabled(false);
    } 
    else {
      combobox->setCurrentItem(1);
      colorbox->setEnabled(false);
    }
    
    connect(combobox, SIGNAL(activated(int)), this, SLOT(comboboxChange()));    
}

void KonqBgndDialog::comboboxChange()
{
    if (combobox->currentItem()) {
      colorbox->setEnabled(false);
      m_propsPage->setEnabled(true);
    }
    else {   
      colorbox->setEnabled(true);
      m_propsPage->setEnabled(false);
    }
}

QColor KonqBgndDialog::color()
{
     if (combobox->currentItem())
       return QColor();
     else
       return colorbutton->color();
}

KonqBgndDialog::~KonqBgndDialog()
{
}

KBgndDialogPage::KBgndDialogPage( QWidget * parent, const QString & pixmapFile,
				  KInstance *instance, const char * resource )
  : QGroupBox( parent, "KBgndDialogPage" ),
    m_resource( resource )
{
    setTitle( i18n("Background Image") );
    m_instance = instance;

    m_wallBox = new QComboBox( false, this, "ComboBox_1" );
    m_wallBox->insertItem( i18n("None") );

    QStringList list = KGlobal::dirs()->findAllResources(resource);

    for (QStringList::ConstIterator it = list.begin(); it != list.end(); it++)
        m_wallBox->insertItem( ( (*it).at(0)=='/' ) ?    // if absolute path
			       KURL( *it ).fileName() :  // then only fileName
			       *it );

    m_wallBox->adjustSize();

    m_browseButton = new QPushButton( i18n("&Browse..."), this );
    m_browseButton->adjustSize();
    connect( m_browseButton, SIGNAL( clicked() ), SLOT( slotBrowse() ) );

    m_wallWidget = new QFrame( this );
    m_wallWidget->setLineWidth( 2 );
    m_wallWidget->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

    showSettings( pixmapFile );

    connect( m_wallBox, SIGNAL( activated( int ) ),
	     this, SLOT( slotWallPaperChanged( int ) ) );

    setMinimumSize( QSize( 400, 300 ) );
}

KBgndDialogPage::~KBgndDialogPage()
{
}

void KBgndDialogPage::showSettings( const QString& fileName )
{
  for ( int i = 1; i < m_wallBox->count(); i++ )
    {
      if ( fileName == m_wallBox->text( i ) )
        {
          m_wallBox->setCurrentItem( i );
          loadWallPaper();
          return;
        }
    }

  if ( !fileName.isEmpty() )
    {
      m_wallBox->insertItem( fileName );
      m_wallBox->setCurrentItem( m_wallBox->count()-1 );
      m_wallBox->adjustSize();
    }
  else m_wallBox->setCurrentItem( 0 );

  loadWallPaper();
}

void KBgndDialogPage::slotBrowse( )
{
    KURL url = KFileDialog::getImageOpenURL( QString::null, this, 
                                             i18n("Select Image" ) );

    if (!url.isValid())
      return;

    if (!url.isLocalFile()) {
      KMessageBox::sorry(this, i18n("Currently only local wallpapers are allowed."));
    } else
      showSettings( url.path() );
}

void KBgndDialogPage::slotWallPaperChanged( int )
{
    loadWallPaper();
}

void KBgndDialogPage::loadWallPaper()
{
    int i = m_wallBox->currentItem();
    if ( i == -1 || i == 0 )  // 0 is 'None'
    {
        m_wallPixmap.resize(0,0);
        m_wallFile = "";
    }
    else
    {
        m_wallFile = m_wallBox->text( i );
        QString file = locate(m_resource.data(), m_wallFile);
        if ( file.isEmpty() && m_resource != "wallpaper") // add fallback for compatibility
            file = locate("wallpaper", m_wallFile);
        if ( file.isEmpty() )
        {
          kdWarning(1203) << "Couldn't locate wallpaper " << m_wallFile << endl;
          m_wallPixmap.resize(0,0);
          m_wallFile = "";
        }
        else
        {
          m_wallPixmap.load( file );

          if ( m_wallPixmap.isNull() )
              kdWarning(1203) << "Could not load wallpaper " << file << endl;
        }
    }
    m_wallWidget->setBackgroundPixmap( m_wallPixmap );
}

void KBgndDialogPage::resizeEvent ( QResizeEvent *e )
{
    QGroupBox::resizeEvent( e );
    int fontHeight = fontMetrics().height();
    m_wallBox->move( KDialog::marginHint(), KDialog::marginHint() + fontHeight );
    int x = m_wallBox->x() + m_wallBox->width() + KDialog::spacingHint();
    int y = m_wallBox->y() + (m_wallBox->height() - m_browseButton->height()) / 2;
    m_browseButton->move( x, y );

    imageX = m_wallBox->x();
    imageY = m_browseButton->y()+m_browseButton->height()+KDialog::spacingHint(); // under the browse button
    imageW = width() - imageX - KDialog::marginHint();
    imageH = height() - imageY - KDialog::marginHint()*2;

    m_wallWidget->setGeometry( imageX, imageY, imageW, imageH );
}

#include "konq_bgnddlg.moc"
