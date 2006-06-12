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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QLabel>
#include <QLayout>
#include <QRadioButton>
//Added by qt3to4:
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kimagefilepreview.h>
#include <klocale.h>
//#include <krecentdocument.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

#include "konq_bgnddlg.h"


KonqBgndDialog::KonqBgndDialog( QWidget* parent,
                                const QString& pixmapFile,
                                const QColor& theColor,
                                const QColor& defaultColor )
 : KDialog( parent, i18n("Background Settings"), Ok|Cancel, Qt::Dialog )
{
    setModal(false);

    QWidget* page = new QWidget( this );
    setMainWidget( page );
    QVBoxLayout* mainLayout = new QVBoxLayout( page );
    mainLayout->setMargin( 0 );
    mainLayout->setSpacing( KDialog::spacingHint() );

    m_buttonGroup = new QGroupBox( i18n("Background"), page );
    QGridLayout* groupLayout = new QGridLayout( m_buttonGroup );
    groupLayout->setAlignment( Qt::AlignTop );
    mainLayout->addWidget( m_buttonGroup );

    // color
    m_radioColor = new QRadioButton( i18n("Co&lor:"), m_buttonGroup );
    groupLayout->addWidget( m_radioColor, 0, 0 );
    m_buttonColor = new KColorButton( theColor, defaultColor, m_buttonGroup );
    m_buttonColor->setSizePolicy( QSizePolicy::Preferred,
                                QSizePolicy::Minimum );
    groupLayout->addWidget( m_buttonColor, 0, 1 );

    connect( m_buttonColor, SIGNAL( changed( const QColor& ) ),
             this, SLOT( slotColorChanged() ) );
    connect( m_radioColor, SIGNAL( toggled(bool) ),
             this, SLOT( slotBackgroundModeChanged() ) );

    // picture
    m_radioPicture = new QRadioButton( i18n("&Picture:"), m_buttonGroup );
    groupLayout->addWidget( m_radioPicture, 1, 0 );
    m_comboPicture = new KUrlComboRequester( m_buttonGroup );
    groupLayout->addWidget( m_comboPicture, 1, 1, 1, 2 );
    initPictures();

    connect( m_comboPicture->comboBox(), SIGNAL( activated( int ) ),
	     this, SLOT( slotPictureChanged() ) );
    connect( m_comboPicture, SIGNAL( urlSelected(const QString &) ),
             this, SLOT( slotPictureChanged() ) );

    QSpacerItem* spacer1 = new QSpacerItem( 0, 0, QSizePolicy::Expanding,
                                            QSizePolicy::Minimum );
    groupLayout->addItem( spacer1, 0, 2 );

    // preview title
    QHBoxLayout* hlay = new QHBoxLayout();
    hlay->setSpacing( KDialog::spacingHint() );
    //mainLayout->addLayout( hlay );
    QLabel* lbl = new QLabel( i18n("Preview"), page );
    hlay->addWidget( lbl );
    QFrame* frame = new QFrame( page );
    frame->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    frame->setFrameShape( QFrame::HLine );
    frame->setFrameShadow( QFrame::Sunken );
    hlay->addWidget( frame );

    mainLayout->addItem(hlay);

    // preview frame
    m_preview = new QFrame( page );
    m_preview->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_preview->setMinimumSize( 370, 180 );
    m_preview->setFrameShape( QFrame::Panel );
    m_preview->setFrameShadow( QFrame::Raised );
    m_preview->setAutoFillBackground( true );
    mainLayout->addWidget( m_preview );

    if ( !pixmapFile.isEmpty() ) {
        loadPicture( pixmapFile );
        m_buttonColor->setColor( defaultColor );
        m_radioPicture->setChecked( true );
    }
    else {
        m_buttonColor->setColor( theColor );
        m_comboPicture->comboBox()->setCurrentIndex( 0 );
        m_radioColor->setChecked( true );
    }
    slotBackgroundModeChanged();
}

KonqBgndDialog::~KonqBgndDialog()
{
}

QColor KonqBgndDialog::color() const
{
    if ( m_radioColor->isChecked() )
        return m_buttonColor->color();

    return QColor();
}

void KonqBgndDialog::initPictures()
{
    KGlobal::dirs()->addResourceType( "tiles",
        KGlobal::dirs()->kde_default("data") + "konqueror/tiles/");
    kDebug(1203) << KGlobal::dirs()->kde_default("data") + "konqueror/tiles/" << endl;

    QStringList list = KGlobal::dirs()->findAllResources("tiles");

    if ( list.isEmpty() )
        m_comboPicture->comboBox()->addItem( i18n("None") );
    else {
        QStringList::ConstIterator it;
        for ( it = list.begin(); it != list.end(); it++ )
            m_comboPicture->comboBox()->addItem(
                ( (*it).at(0) == '/' ) ?    // if absolute path
                KUrl::fromPath( *it ).fileName() :  // then only fileName
                *it );
    }
}

void KonqBgndDialog::loadPicture( const QString& fileName )
{
    int i ;
    for ( i = 0; i < m_comboPicture->comboBox()->count(); i++ ) {
        if ( fileName == m_comboPicture->comboBox()->itemText( i ) ) {
            m_comboPicture->comboBox()->setCurrentIndex( i );
            return;
        }
    }

    if ( !fileName.isEmpty() ) {
        m_comboPicture->comboBox()->addItem( fileName );
        m_comboPicture->comboBox()->setCurrentIndex( i );
    }
    else
        m_comboPicture->comboBox()->setCurrentIndex( 0 );
}

void KonqBgndDialog::slotPictureChanged()
{
    m_pixmapFile = m_comboPicture->comboBox()->currentText();
    QString file = locate( "tiles", m_pixmapFile );
    if ( file.isEmpty() )
        file = locate("wallpaper", m_pixmapFile); // add fallback for compatibility
    if ( file.isEmpty() ) {
        kWarning(1203) << "Couldn't locate wallpaper " << m_pixmapFile << endl;
        m_preview->setPalette(QPalette());
        m_pixmap = QPixmap();
        m_pixmapFile = "";
    }
    else {
        m_pixmap.load( file );

        if ( m_pixmap.isNull() )
            kWarning(1203) << "Could not load wallpaper " << file << endl;
    }
	QPalette palette;
	palette.setBrush( m_preview->backgroundRole(), QBrush( m_pixmap ) );
    m_preview->setPalette( palette );
}

void KonqBgndDialog::slotColorChanged()
{
    QPalette palette;
    palette.setColor( m_preview->backgroundRole(), m_buttonColor->color() );
    m_preview->setPalette(palette);
}

void KonqBgndDialog::slotBackgroundModeChanged()
{
    if ( m_radioColor->isChecked() ) {
        m_buttonColor->setEnabled( true );
        m_comboPicture->setEnabled( false );
        m_pixmapFile = "";
        slotColorChanged();
    }
    else {  // m_comboPicture->isChecked() == true
        m_comboPicture->setEnabled( true );
        m_buttonColor->setEnabled( false );
        slotPictureChanged();
    }
}


#include "konq_bgnddlg.moc"
