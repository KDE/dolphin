#include <kfiledialog.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qmessagebox.h>
#include <qwhatsthis.h>
#include <qlineedit.h>
#include <qvgroupbox.h>
#include <qhbox.h>
#include <qvbox.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <qlabel.h>
#include <kcolorbutton.h>
#include <kcharsets.h>
#include <qspinbox.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <X11/Xlib.h>
#include <klineedit.h>
#include <qspinbox.h>
#include <kfontcombo.h>

#include "htmlopts.h"
#include "policydlg.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <khtmldefaults.h>

#include "appearance.moc"

KAppearanceOptions::KAppearanceOptions(KConfig *config, QString group, QWidget *parent, const char *name)
    : KCModule( parent, name ), m_pConfig(config), m_groupname(group)
{
  QString wtstr;

  QGridLayout *lay = new QGridLayout(this, 1 ,1 , 10, 5);
  int r = 0;
  int E = 0, M = 2, W = 4; //CT 3 (instead 2) allows smaller color buttons

  QButtonGroup *bg = new QButtonGroup( 1, QGroupBox::Vertical,
                                       i18n("Font Si&ze"), this );
  lay->addMultiCellWidget(bg, r, r, E, W);

  QWhatsThis::add( bg, i18n("This is the relative font size Konqueror uses to display web sites.") );

  bg->setExclusive( TRUE );
  connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));

  m_pXSmall = new QRadioButton( i18n("&Tiny"), bg );
  m_pSmall = new QRadioButton( i18n("&Small"), bg );
  m_pMedium = new QRadioButton( i18n("&Medium"), bg );
  m_pLarge = new QRadioButton( i18n("&Large"), bg );
  m_pXLarge = new QRadioButton( i18n("&Huge"), bg );

  QLabel* minSizeLA = new QLabel( i18n( "M&inimum Font Size" ), this );
  r++;
  lay->addMultiCellWidget( minSizeLA, r, r, E , E+1 );

  minSizeSB = new QSpinBox( this );
  minSizeLA->setBuddy( minSizeSB );
  connect( minSizeSB, SIGNAL( valueChanged( int ) ),
	   this, SLOT( slotMinimumFontSize( int ) ) );
  connect( minSizeSB, SIGNAL( valueChanged( int ) ),
	   this, SLOT( changed() ) );
  lay->addWidget( minSizeSB, r, M );
  QWhatsThis::add( minSizeSB, i18n( "Konqueror will never display text smaller than this size,<br> no matter the web site settings" ) );

  QLabel *chsetLA = new QLabel( i18n("Charset:"), this );
  ++r;
  lay->addMultiCellWidget( chsetLA , r, r, E, E+1 );

  m_pChset = new QComboBox( false, this );
  chSets = KGlobal::charsets()->availableCharsetNames();
  m_pChset->insertStringList( chSets );
  lay->addMultiCellWidget(m_pChset,r, r, M, W);
  connect( m_pChset, SIGNAL( activated(const QString& ) ),
	   SLOT( slotCharset(const QString&) ) );

  QLabel* label = new QLabel( i18n("S&tandard Font"), this );
  lay->addWidget( label , ++r, E+1);

  m_pStandard = new KFontCombo( this );
  label->setBuddy( m_pStandard );
  lay->addMultiCellWidget(m_pStandard, r, r, M, W);

  wtstr = i18n("This is the font used to display normal text in a web page.");
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pStandard, wtstr );

  connect( m_pStandard, SIGNAL( activated(const QString&) ),
	   SLOT( slotStandardFont(const QString&) ) );
  connect( m_pStandard, SIGNAL( activated(const QString&) ),
	   SLOT(changed() ) );

  label = new QLabel( i18n( "&Fixed Font"), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFixed = new KFontCombo( this );
  label->setBuddy( m_pFixed );
  lay->addMultiCellWidget(m_pFixed, r, r, M, W);

  wtstr = i18n("This is the font used to display fixed-width (i.e. non-proportional) text.");
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFixed, wtstr );

  connect( m_pFixed, SIGNAL( activated(const QString&) ),
	   SLOT( slotFixedFont(const QString&) ) );
  connect( m_pFixed, SIGNAL( activated(const QString&) ),
	   SLOT(changed() ) );

  label = new QLabel( i18n( "S&erif Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pSerif = new KFontCombo( this );
  label->setBuddy( m_pSerif );
  lay->addMultiCellWidget( m_pSerif, r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as serif." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pSerif, wtstr );

  connect( m_pSerif, SIGNAL( activated( const QString& ) ),
	   SLOT( slotSerifFont( const QString& ) ) );
  connect( m_pSerif, SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "S&ans Serif Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pSansSerif = new KFontCombo( this );
  label->setBuddy( m_pSansSerif );
  lay->addMultiCellWidget( m_pSansSerif, r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as sans-serif." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pSansSerif, wtstr );

  connect( m_pSansSerif, SIGNAL( activated( const QString& ) ),
	   SLOT( slotSansSerifFont( const QString& ) ) );
  connect( m_pSansSerif, SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "&Cursive Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pCursive = new KFontCombo( this );
  label->setBuddy( m_pCursive );
  lay->addMultiCellWidget( m_pCursive, r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as italic." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pCursive, wtstr );

  connect( m_pCursive, SIGNAL( activated( const QString& ) ),
	   SLOT( slotCursiveFont( const QString& ) ) );
  connect( m_pCursive, SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "Fantas&y Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFantasy = new KFontCombo( this );
  label->setBuddy( m_pFantasy );
  lay->addMultiCellWidget( m_pFantasy, r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as a fantasy font." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFantasy, wtstr );

  connect( m_pFantasy, SIGNAL( activated( const QString& ) ),
	   SLOT( slotFantasyFont( const QString& ) ) );
  connect( m_pFantasy, SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "Font size adjustment for this encoding:" ), this );
  lay->addWidget( label, ++r, M );

  m_pFontSizeAdjust = new QSpinBox( -5, 5, 1, this );
  label->setBuddy( m_pFontSizeAdjust );
  lay->addMultiCellWidget( m_pFontSizeAdjust, r, r, M+1, W );

  connect( m_pFontSizeAdjust, SIGNAL( valueChanged( int ) ),
	   SLOT( slotFontSizeAdjust( int ) ) );
  connect( m_pFontSizeAdjust, SIGNAL( valueChanged( int ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "&Default Encoding"), this );
  ++r;
  lay->addMultiCellWidget( label, r, r, E, E+1);

  m_pEncoding = new QComboBox( false, this );
  label->setBuddy( m_pEncoding );
  encodings = KGlobal::charsets()->availableEncodingNames();
  encodings.prepend(i18n("Use language encoding"));
  m_pEncoding->insertStringList( encodings );
  lay->addMultiCellWidget(m_pEncoding,r, r, M, W);

  wtstr = i18n( "Select the default encoding to be used. Normally, you'll be fine with 'Use language encoding' "
	       "and should not have to change this.");
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pEncoding, wtstr );

  connect( m_pEncoding, SIGNAL( activated(const QString& ) ),
	   SLOT( slotEncoding(const QString&) ) );
  connect( m_pEncoding, SIGNAL( activated(const QString& ) ),
	   SLOT(changed() ) );

  connect( bg, SIGNAL( clicked( int ) ), SLOT( slotFontSize( int ) ) );

  r++; lay->setRowStretch(r, 8);
  load();
}

void KAppearanceOptions::slotFontSize( int i )
{
    fSize = i - 1;
}


void KAppearanceOptions::slotMinimumFontSize( int i )
{
  fMinSize = i;
}


void KAppearanceOptions::slotStandardFont(const QString& n )
{
    fonts[0] = n;
}


void KAppearanceOptions::slotFixedFont(const QString& n )
{
    fonts[1] = n;
}


void KAppearanceOptions::slotSerifFont( const QString& n )
{
    fonts[2] = n;
}


void KAppearanceOptions::slotSansSerifFont( const QString& n )
{
    fonts[3] = n;
}


void KAppearanceOptions::slotCursiveFont( const QString& n )
{
    fonts[4] = n;
}


void KAppearanceOptions::slotFantasyFont( const QString& n )
{
    fonts[5] = n;
}

void KAppearanceOptions::slotFontSizeAdjust( int value )
{
    fonts[6] = QString::number( value );
}

void KAppearanceOptions::slotEncoding(const QString& n)
{
    encodingName = n;
}

void KAppearanceOptions::slotCharset( const QString &n )
{
    fontsForCharset[charset] = fonts;
    charset = n;
    updateGUI();
}

void KAppearanceOptions::load()
{
    m_pConfig->setGroup(m_groupname);
    fSize = m_pConfig->readNumEntry( "FontSize", 1 ); // medium
    fMinSize = m_pConfig->readNumEntry( "MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE );

    defaultFonts = QStringList();
    defaultFonts.append( m_pConfig->readEntry( "StandardFont", KGlobalSettings::generalFont().family() ) );
    defaultFonts.append( m_pConfig->readEntry( "FixedFont", KGlobalSettings::fixedFont().family() ) );
    defaultFonts.append( m_pConfig->readEntry( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT ) );
    defaultFonts.append( m_pConfig->readEntry( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT ) );
    defaultFonts.append( m_pConfig->readEntry( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT ) );
    defaultFonts.append( m_pConfig->readEntry( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT ) );
    defaultFonts.append( QString("0") ); // default font size adjustment
    for ( QStringList::Iterator it = chSets.begin(); it != chSets.end(); ++it ) {
	fonts = m_pConfig->readListEntry( *it );
	if( fonts.count() == 6 ) {
	    fonts.append( QString( "0" ) );
	}
	if ( fonts.count() != 7 )
	    fonts = defaultFonts;
	fontsForCharset.insert( *it, fonts );
    }
    charset = chSets[0];
    encodingName = m_pConfig->readEntry( "DefaultEncoding", "" );
    //kdDebug(0) << "encoding = " << encodingName << endl;

    updateGUI();
}

void KAppearanceOptions::defaults()
{
  fSize = 1; // medium
  fMinSize = HTML_DEFAULT_MIN_FONT_SIZE;
  encodingName = "";
  defaultFonts.clear();
  defaultFonts.append( KGlobalSettings::generalFont().family() );
  defaultFonts.append( KGlobalSettings::fixedFont().family() );
  defaultFonts.append( HTML_DEFAULT_VIEW_SERIF_FONT );
  defaultFonts.append( HTML_DEFAULT_VIEW_SANSSERIF_FONT );
  defaultFonts.append( HTML_DEFAULT_VIEW_CURSIVE_FONT );
  defaultFonts.append( HTML_DEFAULT_VIEW_FANTASY_FONT );
  defaultFonts.append( QString("0") ); // default font size adjustment
  for ( QStringList::Iterator it = chSets.begin(); it != chSets.end(); ++it ) {
      fontsForCharset.insert( *it, defaultFonts );
  }

  updateGUI();
}

void KAppearanceOptions::updateGUI()
{
    //kdDebug() << "KAppearanceOptions::updateGUI " << charset << endl;
    int i;
    fonts = fontsForCharset[charset];
    if(fonts.count() != 7) {
	kdDebug() << "fonts wrong" << endl;
	fonts = defaultFonts;
    }

    KCharsets *s = KGlobal::charsets();
    //kdDebug() << s->xNameToID( charset ) << endl;
    QStringList families = s->availableFamilies( s->xNameToID( charset ) );
    families.sort();

    m_pStandard->setFonts( families );
    i = 0;
    for ( QStringList::Iterator sit = families.begin(); sit != families.end(); ++sit, i++ ) {
        if ( fonts[0] == *sit )
            m_pStandard->setCurrentItem( i );
    }

    m_pFixed->setFonts( families );
    i = 0;
    for ( QStringList::Iterator sit = families.begin(); sit != families.end(); ++sit, i++ ) {
        if ( fonts[1] == *sit )
            m_pFixed->setCurrentItem( i );
    }

    m_pSerif->setFonts( families );
    i = 0;
    for ( QStringList::Iterator sit = families.begin(); sit != families.end(); ++sit, i++ ) {
        if ( fonts[2] == *sit )
            m_pSerif->setCurrentItem( i );
    }

    m_pSansSerif->setFonts( families );
    i = 0;
    for ( QStringList::Iterator sit = families.begin(); sit != families.end(); ++sit, i++ ) {
        if ( fonts[3] == *sit )
            m_pSansSerif->setCurrentItem( i );
    }

    m_pCursive->setFonts( families );
    i = 0;
    for ( QStringList::Iterator sit = families.begin(); sit != families.end(); ++sit, i++ ) {
        if ( fonts[4] == *sit )
            m_pCursive->setCurrentItem( i );
    }

    m_pFantasy->setFonts( families );
    i = 0;
    for ( QStringList::Iterator sit = families.begin(); sit != families.end(); ++sit, i++ ) {
        if ( fonts[5] == *sit )
            m_pFantasy->setCurrentItem( i );
    }

    i = 0;
    for ( QStringList::Iterator it = encodings.begin(); it != encodings.end(); ++it, ++i )
    {
        if ( encodingName == *it )
            m_pEncoding->setCurrentItem( i );
    }
    
    m_pFontSizeAdjust->setValue( fonts[6].toInt() );

    m_pXSmall->setChecked( fSize == -1 );
    m_pSmall->setChecked( fSize == 0 );
    m_pMedium->setChecked( fSize == 1 );
    m_pLarge->setChecked( fSize == 2 );
    m_pXLarge->setChecked( fSize == 3 );

	minSizeSB->setValue( fMinSize );
}

void KAppearanceOptions::save()
{
    fontsForCharset[charset] = fonts;
    
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "FontSize", fSize );
    m_pConfig->writeEntry( "MinimumFontSize", fMinSize );

    QMap<QString, QStringList>::Iterator it;
    for( it = fontsForCharset.begin(); it != fontsForCharset.end(); ++it ) {
	//kdDebug() << "KAppearanceOptions::save "<< it.key() << endl;
	//kdDebug() << "         "<< it.data().join(",") << endl;
	m_pConfig->writeEntry( it.key(), it.data() );
    }
    
    // If the user chose "Use language encoding", write an empty string
    if (encodingName == i18n("Use language encoding"))
        encodingName = "";
    m_pConfig->writeEntry( "DefaultEncoding", encodingName );
    m_pConfig->sync();


}


void KAppearanceOptions::changed()
{
  emit KCModule::changed(true);
}

