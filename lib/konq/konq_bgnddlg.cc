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
#include <qdir.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>

#include <konqfileitem.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kglobalsettings.h>

#include "konq_bgnddlg.h"


KonqBgndDialog::KonqBgndDialog( const QString & pixmapFile, KInstance *instance )
  : KDialogBase( 0L, // no parent,
                 "KonqBgndDialog",
                 true, //modal
                 i18n("Select background image"),
                 Ok|Cancel,
                 Ok,
                 true, // separator
                 i18n( "Set as default" )
                 )
{
    m_propsPage = new KBgndDialogPage( this, pixmapFile, instance );
    setMainWidget( m_propsPage );
}

KonqBgndDialog::~KonqBgndDialog()
{
}

KBgndDialogPage::KBgndDialogPage( QWidget * parent, const QString & pixmapFile, KInstance *instance )
  : QWidget( parent, "KBgndDialogPage" )
{
    m_instance = instance; 

    QLabel* tmpQLabel = new QLabel( this, "Label_1" );
    tmpQLabel->setText( i18n("Background") );
    tmpQLabel->move( KDialog::marginHint(), KDialog::marginHint() );
    tmpQLabel->adjustSize();

    m_wallBox = new QComboBox( false, this, "ComboBox_1" );
    m_wallBox->insertItem( i18n("None") );

    QStringList list = KGlobal::dirs()->findAllResources("wallpaper");

    for (QStringList::ConstIterator it = list.begin(); it != list.end(); it++)
        m_wallBox->insertItem( ( (*it).at(0)=='/' ) ?        // if absolute path
                             KURL( *it ).filename() :    // then only filename
                             *it );

    m_wallBox->adjustSize();

    m_browseButton = new QPushButton( i18n("&Browse..."), this );
    m_browseButton->adjustSize();
    connect( m_browseButton, SIGNAL( clicked() ), SLOT( slotBrowse() ) );

    m_wallWidget = new QWidget( this );

    showSettings( pixmapFile );

    connect( m_wallBox, SIGNAL( activated( int ) ), this, SLOT( slotWallPaperChanged( int ) ) );

    setMinimumSize( QSize( 400, 300 ) );
}

KBgndDialogPage::~KBgndDialogPage()
{
}

void KBgndDialogPage::showSettings( QString filename )
{
  for ( int i = 1; i < m_wallBox->count(); i++ )
    {
      if ( filename == m_wallBox->text( i ) )
        {
          m_wallBox->setCurrentItem( i );
          loadWallPaper();
          return;
        }
    }

  if ( !filename.isEmpty() )
    {
      m_wallBox->insertItem( filename );
      m_wallBox->setCurrentItem( m_wallBox->count()-1 );
      m_wallBox->adjustSize();
    }
  else m_wallBox->setCurrentItem( 0 );

  loadWallPaper();
}

void KBgndDialogPage::slotBrowse( )
{
    KURL url = KFileDialog::getOpenURL( 0 );
    if (url.isEmpty())
      return;
    if (!url.isLocalFile()) {
      KMessageBox::sorry(this, i18n("Currently are only local wallpapers allowed."));
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
        QString file = locate("wallpaper", m_wallFile);
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

void KBgndDialogPage::resizeEvent ( QResizeEvent *)
{
    int fontHeight = 2*fontMetrics().height();
    m_wallBox->move( KDialog::marginHint(), KDialog::marginHint() + fontHeight );
    m_browseButton->move( KDialog::marginHint(),
                          m_wallBox->y()+m_wallBox->height()+KDialog::spacingHint() ); // under m_wallBox

    imageX = 10;
    imageY = m_browseButton->y()+m_browseButton->height()+KDialog::spacingHint(); // under the browse button
    imageW = width() - imageX - KDialog::marginHint();
    imageH = height() - imageY - KDialog::marginHint()*2;

    m_wallWidget->setGeometry( imageX, imageY, imageW, imageH );
}

#include "konq_bgnddlg.moc"
