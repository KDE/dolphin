/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>//CT - 12Nov1998
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <kglobal.h>
#include <kconfig.h>
#include <X11/Xlib.h>

#include "fontopts.h"

#include <konqdefaults.h> // include default values directly from konqueror
#include <klocale.h>

//-----------------------------------------------------------------------------

KonqFontOptions::KonqFontOptions(KConfig *config, QString group, QWidget *parent, const char *name)
    : KCModule( parent, name ), g_pConfig(config), groupname(group)
{
    QLabel *label;
    int row = 0;

    QGridLayout *lay = new QGridLayout(this,8/*rows*/,6 /*cols*/,10,5);
    lay->addRowSpacing(0,10);
    lay->addRowSpacing(4,10);
    lay->addRowSpacing(3,10);

    lay->setRowStretch(0,0);
    lay->setRowStretch(1,1);
    lay->setRowStretch(2,1);
    lay->setRowStretch(3,0);
    lay->setRowStretch(4,0);
    lay->setRowStretch(5,0);
    lay->setRowStretch(6,0);
    lay->setRowStretch(7,10);

    lay->setColStretch(0,0);
    lay->setColStretch(1,1);
    lay->setColStretch(2,2);
    lay->setColStretch(3,0);

    row++;
    QButtonGroup *bg = new QButtonGroup( i18n("Font Size"), this );
    QGridLayout *bgLay = new QGridLayout(bg,2,3,10,5);
    bgLay->addRowSpacing(0,10);
    bgLay->setRowStretch(0,0);
    bgLay->setRowStretch(1,1);
    bg->setExclusive( TRUE );
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));

    connect( bg, SIGNAL( clicked( int ) ), SLOT( slotFontSize( int ) ) );

    m_pSmall = new QRadioButton( i18n("Small"), bg );
    bgLay->addWidget(m_pSmall,1,0);

    m_pMedium = new QRadioButton( i18n("Medium"), bg );
    bgLay->addWidget(m_pMedium,1,1);

    m_pLarge = new QRadioButton( i18n("Large"), bg );
    bgLay->addWidget(m_pLarge,1,2);

    bgLay->activate();
#define COLUMNS 3
    lay->addMultiCellWidget(bg,row,row,1,COLUMNS);
    row += 2;


    label = new QLabel( i18n("Standard Font"), this );    label->adjustSize();
    lay->addWidget(label,row,1);

    m_pStandard = new QComboBox( false, this );
    lay->addMultiCellWidget(m_pStandard,row,row,2,COLUMNS);

    getFontList( standardFonts, "-*-*-*-*-*-*-*-*-*-*-p-*-*-*" );
    m_pStandard->insertStrList( &standardFonts );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT( slotStandardFont(const QString&) ) );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT(changed() ) );

    row++;

    //
#define COLOR_BUTTON_COL 3

    label = new QLabel( i18n("Background Color:"), this );
    lay->addWidget(label,row,1);

    m_pBg = new KColorButton( bgColor, this );
    lay->addWidget(m_pBg,row,COLOR_BUTTON_COL);
    connect( m_pBg, SIGNAL( changed( const QColor & ) ),
             SLOT( slotBgColorChanged( const QColor & ) ) );

    row++;
    label = new QLabel( i18n("Normal Text Color:"), this );
    lay->addWidget(label,row,1);

    m_pNormalText = new KColorButton( normalTextColor, this );
    lay->addWidget(m_pNormalText,row,COLOR_BUTTON_COL);
    connect( m_pNormalText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotNormalTextColorChanged( const QColor & ) ) );

    row++;
    label = new QLabel( i18n("Highlighted Text Color:"), this );
    lay->addWidget(label,row,1);

    m_pHighlightedText = new KColorButton( highlightedTextColor, this );
    lay->addWidget(m_pHighlightedText,row,COLOR_BUTTON_COL);
    connect( m_pHighlightedText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotHighlightedTextColorChanged( const QColor & ) ) );

    row++;

    m_pWordWrap = new QCheckBox( i18n("&Word-wrap icon text"), this );
    lay->addMultiCellWidget(m_pWordWrap,row,row,1,COLUMNS);
    connect( m_pWordWrap, SIGNAL(clicked()), this, SLOT(changed()) );


    load();
}

void KonqFontOptions::getFontList( QStrList &list, const char *pattern )
{
    int num;

    char **xFonts = XListFonts( qt_xdisplay(), pattern, 2000, &num );

    for ( int i = 0; i < num; i++ )
    {
        addFont( list, xFonts[i] );
    }

    XFreeFontNames( xFonts );
}

void KonqFontOptions::addFont( QStrList &list, const char *xfont )
{
    const char *ptr = strchr( xfont, '-' );
    if ( !ptr )
        return;
	
    ptr = strchr( ptr + 1, '-' );
    if ( !ptr )
        return;

    QString font = ptr + 1;

    int pos;
    if ( ( pos = font.find( '-' ) ) > 0 )
    {
        font.truncate( pos );

        if ( font.find( "open look", 0, false ) >= 0 )
            return;

        QStrListIterator it( list );

        for ( ; it.current(); ++it )
            if ( it.current() == font )
                return;

        list.append( font.ascii() );
    }
}

void KonqFontOptions::slotFontSize( int i )
{
    fSize = i+3;
}

void KonqFontOptions::slotStandardFont(const QString& n )
{
    stdName = n;
}

void KonqFontOptions::load()
{
    g_pConfig->setGroup(groupname);
    QString fs = g_pConfig->readEntry( "BaseFontSize" );
    if ( !fs.isEmpty() )
    {
        fSize = fs.toInt();
        if ( fSize < 3 )
            fSize = 3;
        else if ( fSize > 5 )
            fSize = 5;
    }
    else
        fSize = 3;

    stdName = g_pConfig->readEntry( "StandardFont" );

    bgColor = g_pConfig->readColorEntry( "BgColor", &FM_DEFAULT_BG_COLOR );
    normalTextColor = g_pConfig->readColorEntry( "NormalTextColor", &FM_DEFAULT_TXT_COLOR );
    highlightedTextColor = g_pConfig->readColorEntry( "HighlightedTextColor", &FM_DEFAULT_HIGHLIGHTED_TXT_COLOR );

    m_pBg->setColor( bgColor );
    m_pNormalText->setColor( normalTextColor );
    m_pHighlightedText->setColor( highlightedTextColor );

    m_pWordWrap->setChecked( g_pConfig->readBoolEntry( "WordWrapText", DEFAULT_WORDWRAPTEXT ) );

    updateGUI();
}

void KonqFontOptions::defaults()
{
    fSize=4;
    stdName = KGlobal::generalFont().family();
    bgColor = FM_DEFAULT_BG_COLOR;

    normalTextColor = FM_DEFAULT_TXT_COLOR;
    highlightedTextColor = FM_DEFAULT_HIGHLIGHTED_TXT_COLOR;

    m_pBg->setColor( bgColor );
    m_pNormalText->setColor( normalTextColor );
    m_pHighlightedText->setColor( highlightedTextColor );
    m_pWordWrap->setChecked( DEFAULT_WORDWRAPTEXT );

    updateGUI();
}

void KonqFontOptions::updateGUI()
{
    if ( stdName.isEmpty() )
        stdName = KGlobal::generalFont().family();

    QStrListIterator sit( standardFonts );
    int i;
    for ( i = 0; sit.current(); ++sit, i++ )
    {
        if ( stdName == sit.current() )
            m_pStandard->setCurrentItem( i );
    }

    m_pSmall->setChecked( fSize == 3 );
    m_pMedium->setChecked( fSize == 4 );
    m_pLarge->setChecked( fSize == 5 );
}

void KonqFontOptions::save()
{
    g_pConfig->setGroup(groupname);			
    g_pConfig->writeEntry( "BaseFontSize", fSize );
    g_pConfig->writeEntry( "StandardFont", stdName );
    g_pConfig->writeEntry( "BgColor", bgColor );
    g_pConfig->writeEntry( "NormalTextColor", normalTextColor );
    g_pConfig->writeEntry( "HighlightedTextColor", highlightedTextColor );
    g_pConfig->writeEntry( "WordWrapText", m_pWordWrap->isChecked() );
    g_pConfig->sync();
}


void KonqFontOptions::changed()
{
  emit KCModule::changed(true);
}

void KonqFontOptions::slotBgColorChanged( const QColor &col )
{
    if ( bgColor != col )
    {
        bgColor = col;
        changed();  
    }
}

void KonqFontOptions::slotNormalTextColorChanged( const QColor &col )
{
    if ( normalTextColor != col )
    {
        normalTextColor = col;
        changed();
    }
}

void KonqFontOptions::slotHighlightedTextColorChanged( const QColor &col )
{
    if ( highlightedTextColor != col )
    {
        highlightedTextColor = col;
        changed();
    }
}

#include "fontopts.moc"
