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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <assert.h>

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qwhatsthis.h>

#include <dcopclient.h>

#include <kapplication.h>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfontcombo.h>
#include <kfontdialog.h>
#include <klocale.h>
#include <konq_defaults.h> // include default values directly from konqueror

#include "fontopts.h"


//-----------------------------------------------------------------------------

KonqFontOptions::KonqFontOptions(KConfig *config, QString group, bool desktop, QWidget *parent, const char* /*name*/)
    : KCModule( parent, "kcmkonq" ), g_pConfig(config), groupname(group), m_bDesktop(desktop)
{
    QLabel *label;
    QString wtstr;
    int row = 0;

    int LASTLINE = m_bDesktop ? 8 : 10; // this can be different :)
#define LASTCOLUMN 2
    QGridLayout *lay = new QGridLayout(this,LASTLINE+1,LASTCOLUMN+1, 0,
        KDialog::spacingHint());
    lay->setRowStretch(LASTLINE,10);
    lay->setColStretch(LASTCOLUMN,10);

    row++;

    m_pStandard = new KFontCombo( this );
    label = new QLabel( m_pStandard, i18n("&Standard font:"), this );
    lay->addWidget(label,row,0);
    lay->addMultiCellWidget(m_pStandard,row,row,1,1);

    wtstr = i18n("This is the font used to display text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pStandard, wtstr );

    row++;
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT( slotStandardFont(const QString&) ) );
    connect( m_pStandard, SIGNAL( activated(const QString&) ),
             SLOT(changed() ) );
    connect( m_pStandard, SIGNAL( textChanged(const QString&) ),
             SLOT( slotStandardFont(const QString&) ) );
    connect( m_pStandard, SIGNAL( textChanged(const QString&) ),
             SLOT(changed() ) );

    m_pSize = new QSpinBox( 4,18,1,this );
    label = new QLabel( m_pSize, i18n("Font si&ze:"), this );
    lay->addWidget(label,row,0);
    lay->addMultiCellWidget(m_pSize,row,row,1,1);

    connect( m_pSize, SIGNAL( valueChanged(int) ),
             this, SLOT( slotFontSize(int) ) );
    row+=2;

    wtstr = i18n("This is the font size used to display text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pSize, wtstr );
    int hAlign = QApplication::reverseLayout() ? AlignRight : AlignLeft;

    //
#define COLOR_BUTTON_COL 1
    m_pNormalText = new KColorButton( normalTextColor, this );
    label = new QLabel( m_pNormalText, i18n("Normal te&xt color:"), this );
    lay->addWidget(label,row,0);
    lay->addWidget(m_pNormalText,row,COLOR_BUTTON_COL,hAlign);

    wtstr = i18n("This is the color used to display text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pNormalText, wtstr );

    connect( m_pNormalText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotNormalTextColorChanged( const QColor & ) ) );

    /*
    row++;
    label = new QLabel( i18n("Highlighted text color:"), this );
    lay->addWidget(label,row,0);

    m_pHighlightedText = new KColorButton( highlightedTextColor, this );
    lay->addWidget(m_pHighlightedText,row,COLOR_BUTTON_COL,hAlign);

    wtstr = i18n("This is the color used to display selected text in Konqueror windows.");
    QWhatsThis::add( label, wtstr );
    QWhatsThis::add( m_pHighlightedText, wtstr );

    connect( m_pHighlightedText, SIGNAL( changed( const QColor & ) ),
             SLOT( slotHighlightedTextColorChanged( const QColor & ) ) );
    */

    row++;

    if ( m_bDesktop )
    {
        m_cbTextBackground = new QCheckBox( i18n("&Text background color:"), this );
        lay->addWidget(m_cbTextBackground,row,0);
        connect( m_cbTextBackground, SIGNAL( clicked() ),
                 SLOT( slotTextBackgroundClicked() ) );

        m_pTextBackground = new KColorButton( textBackgroundColor, this );
        lay->addWidget(m_pTextBackground,row,COLOR_BUTTON_COL,hAlign);

        wtstr = i18n("This is the color used behind the text for the icons on the desktop.");
        QWhatsThis::add( label, wtstr );
        QWhatsThis::add( m_pTextBackground, wtstr );

        connect( m_pTextBackground, SIGNAL( changed( const QColor & ) ),
                 SLOT( slotTextBackgroundColorChanged( const QColor & ) ) );

        row++;
    }
    else
    {
        m_pNbLines = new QSpinBox( 1, 10, 1, this );
        m_pNbLines->setSuffix( i18n( " lines" ) );
        m_pNbLines->setSpecialValueText( i18n( "1 line" ) );
        QLabel* label = new QLabel( m_pNbLines, i18n("H&eight for icon text:"), this );
        lay->addWidget( label, row, 0 );
        lay->addWidget( m_pNbLines, row, 1 );
        connect( m_pNbLines, SIGNAL( valueChanged(int) ),
                 this, SLOT( changed() ) );

        QString thwt = i18n("This is the maximum number of lines that can be"
                            " used to draw icon text. Long file names are"
                            " truncated at the end of the last line.");
        QWhatsThis::add( label, thwt );
        QWhatsThis::add( m_pNbLines, thwt );

        row++;

        // width for the items in multicolumn icon view
        m_pNbWidth = new QSpinBox( 1, 100000, 1, this );
        m_pNbWidth->setSuffix( i18n( " pixels" ) );

        label = new QLabel( m_pNbWidth, i18n("&Width for icon text:"), this );
        lay->addWidget( label, row, 0 );
        lay->addWidget( m_pNbWidth, row, 1 );
        connect( m_pNbWidth, SIGNAL( valueChanged(int) ),
                 this, SLOT( changed() ) );

        thwt = i18n( "This is the maximum width for the icon text when konqueror "
                     "is used in multi column view mode." );
        QWhatsThis::add( label, thwt );
        QWhatsThis::add( m_pNbWidth, thwt );

        row++;
    }

    cbUnderline = new QCheckBox(i18n("&Underline filenames"), this);
    lay->addMultiCellWidget(cbUnderline,row,row,0,LASTCOLUMN,hAlign);
    connect(cbUnderline, SIGNAL(clicked()), this, SLOT(changed()));

    QWhatsThis::add( cbUnderline, i18n("Checking this option will result in filenames"
                                       " being underlined, so that they look like links on a web page. Note:"
                                       " to complete the analogy, make sure that single click activation is"
                                       " enabled in the mouse control module.") );

    if ( !m_bDesktop )
    {
        row++;

        m_pSizeInBytes = new QCheckBox(i18n("Display file sizes in b&ytes"), this);
        lay->addMultiCellWidget( m_pSizeInBytes,row,row,0,LASTCOLUMN,hAlign );
        connect( m_pSizeInBytes, SIGNAL(clicked()), this, SLOT(changed()) );

        QWhatsThis::add( m_pSizeInBytes, i18n("Checking this option will result in file sizes"
                                              " being displayed in bytes. Otherwise file sizes are"
                                              " being displayed in kilobytes or megabytes if appropriate.") );
    }
    row++;

    assert( row == LASTLINE-1 );
    // The last line is empty and grows if resized

    load();
}

void KonqFontOptions::slotFontSize(int i)
{
    m_fSize = i;
    changed();
}

void KonqFontOptions::slotStandardFont(const QString& n )
{
    m_stdName = n;
}

void KonqFontOptions::load()
{
    g_pConfig->setGroup(groupname);

    QFont stdFont = g_pConfig->readFontEntry( "StandardFont" );
    m_stdName = stdFont.family();
    m_fSize = stdFont.pointSize();
    // we have to use QFontInfo, in case the font was specified with a pixel size
    if ( m_fSize == -1 )
        m_fSize = QFontInfo(stdFont).pointSize();

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
        int n = g_pConfig->readNumEntry( "TextHeight", 0 );
        if ( n == 0 ) {
            if ( g_pConfig->readBoolEntry( "WordWrapText", true ) )
                n = DEFAULT_TEXTHEIGHT;
            else
                n = 1;
        }
        m_pNbLines->setValue( n );

        n = g_pConfig->readNumEntry( "TextWidth", DEFAULT_TEXTWIDTH_MULTICOLUMN );
        m_pNbWidth->setValue( n );

        m_pSizeInBytes->setChecked( g_pConfig->readBoolEntry( "DisplayFileSizeInBytes", DEFAULT_FILESIZEINBYTES ) );
    }
    cbUnderline->setChecked( g_pConfig->readBoolEntry("UnderlineLinks", DEFAULT_UNDERLINELINKS ) );

    KConfig cfg("kdeglobals");
    cfg.setGroup("DesktopIcons");

    updateGUI();
    emit KCModule::changed( false );
}

void KonqFontOptions::defaults()
{
    QFont stdFont = KGlobalSettings::generalFont();
    m_stdName = stdFont.family();
    m_fSize = stdFont.pointSize();
    // we have to use QFontInfo, in case the font was specified with a pixel size
    if ( m_fSize == -1 )
        m_fSize = QFontInfo(stdFont).pointSize();

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
        m_pNbLines->setValue( DEFAULT_TEXTHEIGHT );
        m_pNbWidth->setValue( DEFAULT_TEXTWIDTH_MULTICOLUMN );
        m_pSizeInBytes->setChecked( DEFAULT_FILESIZEINBYTES );
    }
    cbUnderline->setChecked( DEFAULT_UNDERLINELINKS );
    updateGUI();
}

void KonqFontOptions::updateGUI()
{
    if ( m_stdName.isEmpty() )
        m_stdName = KGlobalSettings::generalFont().family();

    m_pStandard->setCurrentFont( m_stdName );
    m_pSize->setValue( m_fSize );
}

void KonqFontOptions::save()
{
    g_pConfig->setGroup(groupname);

    QFont stdFont( m_stdName, m_fSize );
    g_pConfig->writeEntry( "StandardFont", stdFont );

    g_pConfig->writeEntry( "NormalTextColor", normalTextColor );
    //g_pConfig->writeEntry( "HighlightedTextColor", highlightedTextColor );
    if ( m_bDesktop )
        g_pConfig->writeEntry( "ItemTextBackground", m_cbTextBackground->isChecked() ? textBackgroundColor : QColor());
    else
    {
        g_pConfig->writeEntry( "TextHeight", m_pNbLines->value() );
        g_pConfig->writeEntry( "TextWidth", m_pNbWidth->value() );
        g_pConfig->writeEntry( "DisplayFileSizeInBytes", m_pSizeInBytes->isChecked() );
    }
    g_pConfig->writeEntry( "UnderlineLinks", cbUnderline->isChecked() );
    g_pConfig->sync();

    KConfig cfg("kdeglobals");
    cfg.setGroup("DesktopIcons");

    // Send signal to konqueror
    // Warning. In case something is added/changed here, keep kfmclient in sync
    QByteArray data;
    if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
    kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

    // Tell kdesktop about the new config file
    int konq_screen_number = KApplication::desktop()->primaryScreen();
    QCString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
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

QString KonqFontOptions::quickHelp() const
{
    return i18n("<h1>Appearance</h1> You can configure how Konqueror looks as a file manager here.");
}

#include "fontopts.moc"
