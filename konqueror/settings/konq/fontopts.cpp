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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <assert.h>

#include <QCheckBox>
#include <QLabel>
#include <QLayout>

//Added by qt3to4:
#include <QGridLayout>
#include <QDesktopWidget>
#include <QFontComboBox>

#include <QtDBus/QtDBus>

#include <kapplication.h>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfontdialog.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <konq_defaults.h> // include default values directly from konqueror

#include "fontopts.h"
#include "konqkcmfactory.h"

class KonqFontOptionsDesktop : public KonqFontOptions
{
    public:
        KonqFontOptionsDesktop(QWidget *parent, const QStringList &args)
            : KonqFontOptions(parent, args, true)
        {}
};

typedef KonqKcmFactory<KonqFontOptions> KonqFontOptionsFactory;
K_EXPORT_COMPONENT_FACTORY(appearance, KonqFontOptionsFactory)

typedef KonqKcmFactory<KonqFontOptionsDesktop> KonqFontOptionsDesktopFactory;
K_EXPORT_COMPONENT_FACTORY(dappearance, KonqFontOptionsDesktopFactory)

//-----------------------------------------------------------------------------

KonqFontOptions::KonqFontOptions(QWidget *parent, const QStringList &, bool desktop)
    : KCModule( _globalInstance(), parent )
    , groupname("FMSettings")
    , m_bDesktop(desktop)
{
    if (m_bDesktop) {
        g_pConfig = KSharedConfig::openConfig(_desktopConfigName(), false, false);
    } else {
        g_pConfig = KSharedConfig::openConfig("konquerorrc", false, true);
    }
    QLabel *label;
    QString wtstr;
    int row = 0;

    int LASTLINE = m_bDesktop ? 8 : 10; // this can be different :)
#define LASTCOLUMN 2
    QGridLayout *lay = new QGridLayout(this );
    lay->setSpacing( KDialog::spacingHint() );
    lay->setRowStretch(LASTLINE,10);
    lay->setColumnStretch(LASTCOLUMN,10);

    row++;

    m_pStandard = new QFontComboBox( this );
    label = new QLabel( i18n("&Standard font:"), this );
    label->setBuddy( m_pStandard );
    lay->addWidget(label,row,0);
    lay->addWidget(m_pStandard,row,1, 1, 1);

    wtstr = i18n("This is the font used to display text in Konqueror windows.");
    label->setWhatsThis( wtstr );
    m_pStandard->setWhatsThis( wtstr );

    row++;
    connect( m_pStandard, SIGNAL( currentFontChanged(QFont) ),
             SLOT( slotStandardFont(QFont) ) );
    connect( m_pStandard, SIGNAL( currentFontChanged(const QFont&) ), SLOT(changed() ) );

    m_pSize = new QSpinBox( 4,18,1,this );
    label = new QLabel( i18n("Font si&ze:"), this );
    label->setBuddy( m_pSize );
    lay->addWidget(label,row,0);
    lay->addWidget(m_pSize,row,1, 1, 1);

    connect( m_pSize, SIGNAL( valueChanged(int) ),
             this, SLOT( slotFontSize(int) ) );
    row+=2;

    wtstr = i18n("This is the font size used to display text in Konqueror windows.");
    label->setWhatsThis( wtstr );
    m_pSize->setWhatsThis( wtstr );
    Qt::AlignmentFlag hAlign = QApplication::isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft;

    //
#define COLOR_BUTTON_COL 1
    m_pNormalText = new KColorButton( normalTextColor, this );
    label = new QLabel( i18n("Normal te&xt color:"), this );
    label->setBuddy( m_pNormalText );
    lay->addWidget(label,row,0);
    lay->addWidget(m_pNormalText,row,COLOR_BUTTON_COL,hAlign);

    wtstr = i18n("This is the color used to display text in Konqueror windows.");
    label->setWhatsThis( wtstr );
    m_pNormalText->setWhatsThis( wtstr );

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
        label->setWhatsThis( wtstr );
        m_pTextBackground->setWhatsThis( wtstr );

        connect( m_pTextBackground, SIGNAL( changed( const QColor & ) ),
                 SLOT( slotTextBackgroundColorChanged( const QColor & ) ) );

        row++;
    }
    else
    {
        m_pNbLines = new QSpinBox( 1, 10, 1, this );
        QLabel* label = new QLabel( i18n("H&eight for icon text:"), this );
        label->setBuddy( m_pNbLines );
        lay->addWidget( label, row, 0 );
        lay->addWidget( m_pNbLines, row, 1 );
        connect( m_pNbLines, SIGNAL( valueChanged(int) ),
                 this, SLOT( changed() ) );
        connect( m_pNbLines, SIGNAL( valueChanged(int) ),
                 SLOT( slotPNbLinesChanged(int)) );

        QString thwt = i18n("This is the maximum number of lines that can be"
                            " used to draw icon text. Long file names are"
                            " truncated at the end of the last line.");
        label->setWhatsThis( thwt );
        m_pNbLines->setWhatsThis( thwt );

        row++;

        // width for the items in multicolumn icon view
        m_pNbWidth = new QSpinBox( 1, 100000, 1, this );

        label = new QLabel( i18n("&Width for icon text:"), this );
        label->setBuddy( m_pNbWidth );
        lay->addWidget( label, row, 0 );
        lay->addWidget( m_pNbWidth, row, 1 );
        connect( m_pNbWidth, SIGNAL( valueChanged(int) ),
                 this, SLOT( changed() ) );
        connect( m_pNbWidth, SIGNAL( valueChanged(int) ),
                 SLOT( slotPNbWidthChanged(int)) );

        thwt = i18n( "This is the maximum width for the icon text when konqueror "
                     "is used in multi column view mode." );
        label->setWhatsThis( thwt );
        m_pNbWidth->setWhatsThis( thwt );

        row++;
    }

    cbUnderline = new QCheckBox(i18n("&Underline filenames"), this);
    lay->addWidget(cbUnderline,row,0, 1,LASTCOLUMN+1,hAlign);
    connect(cbUnderline, SIGNAL(clicked()), this, SLOT(changed()));

    cbUnderline->setWhatsThis( i18n("Checking this option will result in filenames"
                                       " being underlined, so that they look like links on a web page. Note:"
                                       " to complete the analogy, make sure that single click activation is"
                                       " enabled in the mouse control module.") );

    if ( !m_bDesktop )
    {
        row++;

        m_pSizeInBytes = new QCheckBox(i18n("Display file sizes in b&ytes"), this);
        lay->addWidget( m_pSizeInBytes,row,0, 1,LASTCOLUMN+1,hAlign );
        connect( m_pSizeInBytes, SIGNAL(clicked()), this, SLOT(changed()) );

        m_pSizeInBytes->setWhatsThis( i18n("Checking this option will result in file sizes"
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

void KonqFontOptions::slotStandardFont(const QFont& f )
{
    m_stdName = f.family();
}

void KonqFontOptions::slotPNbLinesChanged(int value)
{
    m_pNbLines->setSuffix( i18np( " line", " lines", value ) );
}

void KonqFontOptions::slotPNbWidthChanged(int value)
{
    m_pNbWidth->setSuffix( i18np( " pixel", " pixels", value ) );
}

void KonqFontOptions::load()
{
    g_pConfig->setGroup(groupname);

    QFont stdFont = g_pConfig->readEntry( "StandardFont", QFont() );
    m_stdName = stdFont.family();
    m_fSize = stdFont.pointSize();
    // we have to use QFontInfo, in case the font was specified with a pixel size
    if ( m_fSize == -1 )
        m_fSize = QFontInfo(stdFont).pointSize();

    normalTextColor = KGlobalSettings::textColor();
    normalTextColor = g_pConfig->readEntry( "NormalTextColor", normalTextColor );
    m_pNormalText->setColor( normalTextColor );

    /* highlightedTextColor = KGlobalSettings::highlightedTextColor();
    highlightedTextColor = g_pConfig->readEntry( "HighlightedTextColor", &highlightedTextColor );
    m_pHighlightedText->setColor( highlightedTextColor );
    */

    if ( m_bDesktop )
    {
        textBackgroundColor = g_pConfig->readEntry( "ItemTextBackground" );
        m_cbTextBackground->setChecked(textBackgroundColor.isValid());
        m_pTextBackground->setEnabled(textBackgroundColor.isValid());
        m_pTextBackground->setColor( textBackgroundColor );
	// Don't keep an invalid color around, otherwise checking the checkbox still gives invalid.
	if ( !textBackgroundColor.isValid() )
            textBackgroundColor = Qt::black;
    }
    else
    {
        int n = g_pConfig->readEntry( "TextHeight", 0 );
        if ( n == 0 ) {
            if ( g_pConfig->readEntry( "WordWrapText", QVariant(true )).toBool() )
                n = DEFAULT_TEXTHEIGHT;
            else
                n = 1;
        }
        m_pNbLines->setValue( n );

        n = g_pConfig->readEntry( "TextWidth", DEFAULT_TEXTWIDTH_MULTICOLUMN );
        m_pNbWidth->setValue( n );

        m_pSizeInBytes->setChecked( g_pConfig->readEntry( "DisplayFileSizeInBytes", QVariant(DEFAULT_FILESIZEINBYTES )).toBool() );
    }
    cbUnderline->setChecked( g_pConfig->readEntry("UnderlineLinks", QVariant(DEFAULT_UNDERLINELINKS )).toBool() );

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig("kdeglobals");
    cfg->setGroup("DesktopIcons");

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

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig("kdeglobals");
    cfg->setGroup("DesktopIcons");

    // Send signal to konqueror
    // Warning. In case something is added/changed here, keep kfmclient in sync
    QDBusMessage message =
        QDBusMessage::signal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration", QDBus::sessionBus());
    QDBus::sessionBus().send(message);

    // Tell kdesktop about the new config file
    int konq_screen_number = KApplication::desktop()->primaryScreen();
    QByteArray appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname = "kdesktop-screen-" + QByteArray::number( konq_screen_number);
#ifdef __GNUC__
#warning TODO Port to kdesktop DBus interface
#endif
//    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
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
