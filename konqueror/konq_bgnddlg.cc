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

#include "konq_factory.h"
#include "konq_bgnddlg.h"


KonqBgndDialog::KonqBgndDialog( const KURL & dirURL, KInstance *instance )
  : KDialogBase( 0L, // no parent,
                 "KonqBgndDialog",
                 true, //modal
                 i18n("Background image for %1").arg(dirURL.decodedURL()),
                 Ok|Apply|Cancel,
                 Ok,
                 true, // separator
                 i18n( "Set as default" )
                 )
{
  // Is it the trash bin ?
  //QString path = item->url().path( 1 ); // adds trailing slash
  //if ( item->url().isLocalFile() && path == KGlobalSettings::trashPath() )
  //  return false;

    m_propsPage = new DirPropsPage( this, dirURL, instance );
    connect( this, SIGNAL( okClicked() ), m_propsPage, SLOT( slotApply() ) );
    connect( this, SIGNAL( applyClicked() ), m_propsPage, SLOT( slotApply() ) );
    connect( this, SIGNAL( user1Clicked() ), m_propsPage, SLOT( slotApplyGlobal() ) );

    setMainWidget( m_propsPage );
}

KonqBgndDialog::~KonqBgndDialog()
{
}

DirPropsPage::DirPropsPage( QWidget * parent, const KURL & dirURL, KInstance *instance )
  : QWidget( parent, "DirPropsPage" ), m_url( dirURL )
{
    m_instance = instance; 
    m_fileitem = new KonqFileItem( -1, -1, dirURL );

    QLabel* tmpQLabel = new QLabel( this, "Label_1" );
    tmpQLabel->setText( i18n("Background") );
    tmpQLabel->move( KDialog::marginHint(), KDialog::marginHint() );
    tmpQLabel->adjustSize();

    m_wallBox = new QComboBox( false, this, "ComboBox_1" );

    QString tmp = dirURL.path();

    if ( tmp.at(tmp.length() - 1) != '/' )
	tmp += "/.directory";
    else
	tmp += ".directory";

    QString wallStr;
    QFile f( tmp );
    if ( f.open( IO_ReadOnly ) )
    {
	f.close();

	KSimpleConfig config( tmp );
	config.setGroup( "URL properties" );
	wallStr = config.readEntry( "BgImage" );
    }

    QStringList list = KGlobal::dirs()->findAllResources("wallpaper");
    m_wallBox->insertItem(  i18n("(Default)"), 0 );

    for (QStringList::ConstIterator it = list.begin(); it != list.end(); it++)
        m_wallBox->insertItem( ( (*it).at(0)=='/' ) ?        // if absolute path
                             KURL( *it ).filename() :    // then only filename
                             *it );

    showSettings( wallStr );

    m_wallBox->adjustSize();

    m_browseButton = new QPushButton( i18n("&Browse..."), this );
    m_browseButton->adjustSize();
    connect( m_browseButton, SIGNAL( clicked() ), SLOT( slotBrowse() ) );

    m_wallWidget = new QWidget( this );
    loadWallPaper();

    connect( m_wallBox, SIGNAL( activated( int ) ), this, SLOT( slotWallPaperChanged( int ) ) );

    setMinimumSize( QSize( 400, 300 ) );
}

DirPropsPage::~DirPropsPage()
{
}

void DirPropsPage::slotApply()
{
    QString tmp = m_url.path();
    if ( tmp.at( tmp.length() - 1 ) != '/' )
	tmp += "/.directory";
    else
	tmp += ".directory";

    QFile f( tmp );
    if ( !f.open( IO_ReadWrite ) )
    {
      KMessageBox::error( 0, i18n("Could not write to\n") + tmp);
	return;
    }
    f.close();

    KSimpleConfig config( tmp );
    config.setGroup( "URL properties" );

    int i = m_wallBox->currentItem();
    if ( i != -1 )
    {
	if ( strcmp( m_wallBox->text( i ),  i18n("(Default)") ) == 0 )
	    config.writeEntry( "BgImage", "" );
	else
	    config.writeEntry( "BgImage", m_wallBox->text( i ) );
    }

    config.sync();
}

void DirPropsPage::showSettings( QString filename )
{
  m_wallBox->setCurrentItem( 0 );
  for ( int i = 1; i < m_wallBox->count(); i++ )
    {
      if ( filename == m_wallBox->text( i ) )
        {
          m_wallBox->setCurrentItem( i );
          return;
        }
    }

  if ( !filename.isEmpty() )
    {
      m_wallBox->insertItem( filename );
      m_wallBox->setCurrentItem( m_wallBox->count()-1 );
    }
}

void DirPropsPage::slotBrowse( )
{
    KURL url = KFileDialog::getOpenURL( 0 );
    if (url.isEmpty())
      return;
    if (!url.isLocalFile()) {
      KMessageBox::sorry(this, i18n("Currently are only local wallpapers allowed."));
    }
    showSettings( url.path() );
    m_wallBox->adjustSize();
    loadWallPaper();
}

void DirPropsPage::slotWallPaperChanged( int )
{
    loadWallPaper();
}

void DirPropsPage::loadWallPaper()
{
    int i = m_wallBox->currentItem();
    if ( i == -1 )
        m_wallPixmap.resize(0,0);
    else
    {
        QString text = m_wallBox->text( i );
        if ( text == i18n( "(Default)" ) )
            m_wallPixmap.resize(0,0);
        else
        {
            QString file = locate("wallpaper", text);
            if ( file != m_wallFile )
            {
                m_wallFile = file;
                m_wallPixmap.load( file );
            }

            if ( m_wallPixmap.isNull() )
                warning("Could not load wallpaper %s\n",file.ascii());
        }
    }
    m_wallWidget->setBackgroundPixmap( m_wallPixmap );
}

void DirPropsPage::resizeEvent ( QResizeEvent *)
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
    //applyButton->move( 10, imageY+imageH+KDialog::spacingHint() );
    //globalButton->move( applyButton->x() + applyButton->width() + KDialog::spacingHint(), applyButton->y() );
}

void DirPropsPage::slotApplyGlobal()
{
    // Write the image setting to the configuration file shared by
    // the builtin views (iconview/treeview)
    KConfig *config = new KConfig( "konqbuiltinviewrc", false, false );

    config->setGroup( "Settings" );

    int i = m_wallBox->currentItem();
    if ( i != -1 )
    {
	if ( strcmp( m_wallBox->text( i ),  i18n("(None)") ) == 0 )
	    config->writeEntry( "BackgroundPixmap", "" );
	else
	    config->writeEntry( "BackgroundPixmap", m_wallBox->text( i ) );
    }

    config->sync();
}

#include "konq_bgnddlg.moc"
