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
#include <qspinbox.h>
#include <qlabel.h>
#include <qlayout.h>//CT - 12Nov1998
#include <qradiobutton.h>
#include <qwhatsthis.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kcolorbutton.h>
#include <X11/Xlib.h>

#include "fontopts.h"
#include <assert.h>

#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <kfontdialog.h>

//-----------------------------------------------------------------------------

KonqFontOptions::KonqFontOptions(KConfig *config, QString group, bool desktop, QWidget *parent, const char *name)
    : KCModule( parent, name ), g_pConfig(config), groupname(group), m_bDesktop(desktop)
{
    QLabel *label;
    QString wtstr;
    int row = 0;

    int LASTLINE = m_bDesktop ? 7 : 8; // this can be different :)
#define LASTCOLUMN 2
    QGridLayout *lay = new QGridLayout(this,LASTLINE+1,LASTCOLUMN+1,KDialog::marginHint(),
                                       KDialog::spacingHint());
    lay->setRowStretch(LASTLINE,10);
    lay->setColStretch(LASTCOLUMN,10);

    row++;
    label = new QLabel( i18n("Standard Font"), this );
    lay->addWidget(label,row,0);

    m_pStandard = new QComboBox( false, this );
    lay->addMultiCellWidget(m_pStandard,row,row,1,1);

    wtstr = i18n("This is the font used to display text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pStandard, wtstr );

    row++;
    KFontChooser::getFontList(standardFonts, false);
    m_pStandard->insertStringList( standardFonts );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT( slotStandardFont(const QString&) ) );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT(changed() ) );


    label = new QLabel( i18n("Font Size"), this );
    lay->addWidget(label,row,0);

    m_pSize = new QSpinBox( 4,18,1,this );
    lay->addMultiCellWidget(m_pSize,row,row,1,1);

    connect( m_pSize, SIGNAL( valueChanged(int) ),
             this, SLOT( slotFontSize(int) ) );
    row+=2;

    wtstr = i18n("This is the font size used to display text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pSize, wtstr );

    //
#define COLOR_BUTTON_COL 1
    label = new QLabel( i18n("Normal Text Color:"), this );
    lay->addWidget(label,row,0);

    m_pNormalText = new KColorButton( normalTextColor, this );
    lay->addWidget(m_pNormalText,row,COLOR_BUTTON_COL,Qt::AlignLeft);

    wtstr = i18n("This is the color used to display text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pNormalText, wtstr );

    connect( m_pNormalText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotNormalTextColorChanged( const QColor & ) ) );

    /*
    row++;
    label = new QLabel( i18n("Highlighted Text Color:"), this );
    lay->addWidget(label,row,0);

    m_pHighlightedText = new KColorButton( highlightedTextColor, this );
    lay->addWidget(m_pHighlightedText,row,COLOR_BUTTON_COL,Qt::AlignLeft);

    wtstr = i18n("This is the color used to display selected text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pHighlightedText, wtstr );

    connect( m_pHighlightedText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotHighlightedTextColorChanged( const QColor & ) ) );
    */

    row++;

    if ( m_bDesktop )
    {
        m_cbTextBackground = new QCheckBox( i18n("&Text Background Color:"), this );
        lay->addWidget(m_cbTextBackground,row,0);
        connect( m_cbTextBackground, SIGNAL( clicked() ),
                 SLOT( slotTextBackgroundClicked() ) );

        m_pTextBackground = new KColorButton( textBackgroundColor, this );
        lay->addWidget(m_pTextBackground,row,COLOR_BUTTON_COL,Qt::AlignLeft);

        wtstr = i18n("This is the color used behind the text for the icons on the desktop.");
        QWhatsThis::add( label, wtstr );
        QWhatsThis::add( m_pTextBackground, wtstr );

        connect( m_pTextBackground, SIGNAL( changed( const QColor & ) ),
                 SLOT( slotTextBackgroundColorChanged( const QColor & ) ) );

        row++;
    }
    else
    {
        m_pWordWrap = new QCheckBox( i18n("&Word-wrap icon text"), this );
        lay->addMultiCellWidget(m_pWordWrap,row,row,0,LASTCOLUMN);
        connect( m_pWordWrap, SIGNAL(clicked()), this, SLOT(changed()) );

        QWhatsThis::add( m_pWordWrap, i18n("Checking this option will wrap long filenames"
                                       " to multiple lines, rather than showing only the part of the filename"
                                       " that fits on a single line.<p>"
                                       " Hint: if you uncheck this option, you can still see the word-wrapped filename"
                                       " by pausing the mouse pointer over the icon.") );

        row++;
    }

    cbUnderline = new QCheckBox(i18n("&Underline filenames"), this);
    lay->addMultiCellWidget(cbUnderline,row,row,0,LASTCOLUMN,Qt::AlignLeft);
    connect(cbUnderline, SIGNAL(clicked()), this, SLOT(changed()));

    QWhatsThis::add( cbUnderline, i18n("Checking this option will result in filenames"
                                       " being underlined, so that they look like links on a web page. Note:"
                                       " to complete the analogy, make sure that single click activation is"
                                       " enabled in the mouse control module.") );

    if ( !m_bDesktop )
    {
        row++;

        m_pSizeInBytes = new QCheckBox(i18n("Display filesizes in b&ytes"), this);
        lay->addMultiCellWidget( m_pSizeInBytes,row,row,0,LASTCOLUMN,Qt::AlignLeft );
        connect( m_pSizeInBytes, SIGNAL(clicked()), this, SLOT(changed()) );

        QWhatsThis::add( m_pSizeInBytes, i18n("Checking this option will result in filesizes"
                                              " being displayed in bytes. Otherwise filesizes are"
                                              " converted to kilobytes or megabytes if appropriate.") );
    }

    assert( row == LASTLINE-1 );
    // The last line is empty and grows if resized

    load();
}

void KonqFontOptions::slotFontSize(int i)
{
    fSize = i;
    changed();
}

void KonqFontOptions::slotStandardFont(const QString& n )
{
    stdName = n;
}

void KonqFontOptions::load()
{
    g_pConfig->setGroup(groupname);

    m_stdFont = g_pConfig->readFontEntry( "StandardFont" );
    stdName = m_stdFont.family();
    fSize = m_stdFont.pointSize();

    normalTextColor = KGlobalSettings::textColor();
    normalTextColor = g_pConfig->readColorEntry( "NormalTextColor", &normalTextColor );
    m_pNormalText->setColor( normalTextColor );

    /* highlightedTextColor = KGlobalSettings::highlightedTextColor();
    highlightedTextColor = g_pConfig->readColorEntry( "HighlightedTextColor", &highlightedTextColor );
    m_pHighlightedText->setColor( highlightedTextColor );
    */

    if ( m_bDesktop )
    {
        textBackgroundColor = g_pConfig->readColorEntry( "ItemTextBackground" );
        m_cbTextBackground->setChecked(textBackgroundColor.isValid());
        m_pTextBackground->setEnabled(textBackgroundColor.isValid());
        m_pTextBackground->setColor( textBackgroundColor );
	// Don't keep an invalid color around, otherwise checking the checkbox still gives invalid.
	if ( !textBackgroundColor.isValid() )
            textBackgroundColor = Qt::black;
    }
    else
    {
        m_pWordWrap->setChecked( g_pConfig->readBoolEntry( "WordWrapText", DEFAULT_WORDWRAPTEXT ) );
        m_pSizeInBytes->setChecked( g_pConfig->readBoolEntry( "DisplayFileSizeInBytes", DEFAULT_FILESIZEINBYTES ) );
    }
    cbUnderline->setChecked( g_pConfig->readBoolEntry("UnderlineLinks", DEFAULT_UNDERLINELINKS ) );

    updateGUI();
}

void KonqFontOptions::defaults()
{
		// Should grab from the configurable location (Oliveri).
    fSize=8;

    stdName = KGlobalSettings::generalFont().family();
    m_stdFont = QFont(stdName, 12);

    normalTextColor = KGlobalSettings::textColor();
    m_pNormalText->setColor( normalTextColor );

    //highlightedTextColor = KGlobalSettings::highlightedTextColor();
    //m_pHighlightedText->setColor( highlightedTextColor );
    if ( m_bDesktop )
    {
        m_cbTextBackground->setChecked(false);
        m_pTextBackground->setEnabled(false);
    }
    else
    {
        m_pWordWrap->setChecked( DEFAULT_WORDWRAPTEXT );
        m_pSizeInBytes->setChecked( DEFAULT_FILESIZEINBYTES );
    }
    cbUnderline->setChecked( DEFAULT_UNDERLINELINKS );

    updateGUI();
}

void KonqFontOptions::updateGUI()
{
    if ( stdName.isEmpty() )
        stdName = KGlobalSettings::generalFont().family();

    QStringList::Iterator sit = standardFonts.begin();
    int i;
    for ( i = 0; sit != standardFonts.end(); ++sit, i++ )
    {
        if ( stdName == (*sit) )
            m_pStandard->setCurrentItem( i );
    }
    m_pSize->setValue( fSize );
}

void KonqFontOptions::save()
{
    g_pConfig->setGroup(groupname);

 		// fSize set via signal (Oliveri)
    m_stdFont.setPointSize(fSize);

    m_stdFont.setFamily( stdName );
    g_pConfig->writeEntry( "StandardFont", m_stdFont );

    g_pConfig->writeEntry( "NormalTextColor", normalTextColor );
    //g_pConfig->writeEntry( "HighlightedTextColor", highlightedTextColor );
    if ( m_bDesktop )
        g_pConfig->writeEntry( "ItemTextBackground", m_cbTextBackground->isChecked() ? textBackgroundColor : QColor());
    else
    {
        g_pConfig->writeEntry( "WordWrapText", m_pWordWrap->isChecked() );
        g_pConfig->writeEntry( "DisplayFileSizeInBytes", m_pSizeInBytes->isChecked() );
    }
    g_pConfig->writeEntry( "UnderlineLinks", cbUnderline->isChecked() );
    g_pConfig->sync();
}


void KonqFontOptions::changed()
{
  emit KCModule::changed(true);
}

void KonqFontOptions::slotTextBackgroundClicked()
{
    m_pTextBackground->setEnabled( m_cbTextBackground->isChecked() );
    changed();
}

void KonqFontOptions::slotNormalTextColorChanged( const QColor &col )
{
    if ( normalTextColor != col )
    {
        normalTextColor = col;
        changed();
    }
}

/*
void KonqFontOptions::slotHighlightedTextColorChanged( const QColor &col )
{
    if ( highlightedTextColor != col )
    {
        highlightedTextColor = col;
        changed();
    }
}
*/

void KonqFontOptions::slotTextBackgroundColorChanged( const QColor &col )
{
    if ( textBackgroundColor != col )
    {
        textBackgroundColor = col;
        changed();
    }
}

#include "fontopts.moc"
