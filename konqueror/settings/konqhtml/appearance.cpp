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
#include <knuminput.h>

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

  QGroupBox* gb = new QGroupBox( 1, QGroupBox::Horizontal, i18n("Font Si&ze"), this );
  lay->addMultiCellWidget(gb, r, r, E, W);

  QWhatsThis::add( gb, i18n("This is the relative font size Konqueror uses to display web sites.") );

  m_minSize = new KIntNumInput( fMinSize, gb );
  m_minSize->setLabel( i18n( "M&inimum Font Size" ) );
  m_minSize->setRange( 1, 20 );
  connect( m_minSize, SIGNAL( valueChanged( int ) ), this, SLOT( slotMinimumFontSize( int ) ) );
  connect( m_minSize, SIGNAL( valueChanged( int ) ), this, SLOT( changed() ) );
  QWhatsThis::add( m_minSize, i18n( "Konqueror will never display text smaller than "
                                    "this size,<br>overriding any other settings" ) );

  m_MedSize = new KIntNumInput( m_minSize, fSize, gb );
  m_MedSize->setLabel( i18n( "Medium Font Size" ) );
  m_MedSize->setRange( 4, 28 );
  connect( m_MedSize, SIGNAL( valueChanged( int ) ), this, SLOT( slotFontSize( int ) ) );
  connect( m_MedSize, SIGNAL( valueChanged( int ) ), this, SLOT( changed() ) );
  QWhatsThis::add( m_MedSize,
                   i18n("This is the relative font size Konqueror uses "
                        "to display web sites.") );

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

  m_pFonts[0] = new KFontCombo( this );
  label->setBuddy( m_pFonts[0] );
  lay->addMultiCellWidget(m_pFonts[0], r, r, M, W);

  wtstr = i18n("This is the font used to display normal text in a web page.");
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFonts[0], wtstr );

  connect( m_pFonts[0], SIGNAL( activated(const QString&) ),
	   SLOT( slotStandardFont(const QString&) ) );
  connect( m_pFonts[0], SIGNAL( activated(const QString&) ),
	   SLOT(changed() ) );

  label = new QLabel( i18n( "&Fixed Font"), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFonts[1] = new KFontCombo( this );
  label->setBuddy( m_pFonts[1] );
  lay->addMultiCellWidget(m_pFonts[1], r, r, M, W);

  wtstr = i18n("This is the font used to display fixed-width (i.e. non-proportional) text.");
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFonts[1], wtstr );

  connect( m_pFonts[1], SIGNAL( activated(const QString&) ),
	   SLOT( slotFixedFont(const QString&) ) );
  connect( m_pFonts[1], SIGNAL( activated(const QString&) ),
	   SLOT(changed() ) );

  label = new QLabel( i18n( "S&erif Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFonts[2] = new KFontCombo( this );
  label->setBuddy( m_pFonts[2] );
  lay->addMultiCellWidget( m_pFonts[2], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as serif." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFonts[2], wtstr );

  connect( m_pFonts[2], SIGNAL( activated( const QString& ) ),
	   SLOT( slotSerifFont( const QString& ) ) );
  connect( m_pFonts[2], SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "S&ans Serif Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFonts[3] = new KFontCombo( this );
  label->setBuddy( m_pFonts[3] );
  lay->addMultiCellWidget( m_pFonts[3], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as sans-serif." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFonts[3], wtstr );

  connect( m_pFonts[3], SIGNAL( activated( const QString& ) ),
	   SLOT( slotSansSerifFont( const QString& ) ) );
  connect( m_pFonts[3], SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "&Cursive Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFonts[4] = new KFontCombo( this );
  label->setBuddy( m_pFonts[4] );
  lay->addMultiCellWidget( m_pFonts[4], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as italic." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFonts[4], wtstr );

  connect( m_pFonts[4], SIGNAL( activated( const QString& ) ),
	   SLOT( slotCursiveFont( const QString& ) ) );
  connect( m_pFonts[4], SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "Fantas&y Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFonts[5] = new KFontCombo( this );
  label->setBuddy( m_pFonts[5] );
  lay->addMultiCellWidget( m_pFonts[5], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as a fantasy font." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFonts[5], wtstr );

  connect( m_pFonts[5], SIGNAL( activated( const QString& ) ),
	   SLOT( slotFantasyFont( const QString& ) ) );
  connect( m_pFonts[5], SIGNAL( activated( const QString& ) ),
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

  m_pEnforceCharset = new QCheckBox( i18n( "Enforce default Encoding charset" ), this );
  connect( m_pEnforceCharset, SIGNAL( toggled( bool ) ), this, SLOT( slotEnforceDefault( bool ) ) );
  connect( m_pEnforceCharset, SIGNAL( toggled( bool ) ), this, SLOT( changed() ) );
  QWhatsThis::add( m_pEnforceCharset ,
                   i18n( "Select this to enforce that the default locale encoding "
                         "is always used as charset. This is useful when you don't "
                         "have unicode fonts installed and some webpages show ugly "
                         "fixed size fonts for you. You won't be able to view pages "
                         "that require foreign charsets properly any more. " ) );
  ++r;
  lay->addMultiCellWidget( m_pEnforceCharset, r, r, M, W );


  ++r; lay->setRowStretch(r, 8);


  load();
}

void KAppearanceOptions::slotFontSize( int i )
{
    fSize = i;
    if ( fSize < fMinSize ) {
        m_minSize->setValue( fSize );
        fMinSize = fSize;
    }
}


void KAppearanceOptions::slotMinimumFontSize( int i )
{
    fMinSize = i;
    if ( fMinSize > fSize ) {
        m_MedSize->setValue( fMinSize );
        fSize = fMinSize;
    }
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

void KAppearanceOptions::slotEnforceDefault( bool n )
{
    enforceCharset = n;
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
    fSize = m_pConfig->readNumEntry( "MediumFontSize", 10 );
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
    enforceCharset = m_pConfig->readBoolEntry( "EnforceDefaultCharset", false );

    //kdDebug(0) << "encoding = " << encodingName << endl;

    updateGUI();
}

void KAppearanceOptions::defaults()
{
  fSize = 10;
  fMinSize = HTML_DEFAULT_MIN_FONT_SIZE;
  encodingName = "";
  enforceCharset = true;
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
    m_families = s->availableFamilies( s->xNameToID( charset ) );
    m_families.sort();

    m_pFonts[0]->setFonts( m_families );
    m_pFonts[1]->setFonts( m_families );
    m_pFonts[2]->setFonts( m_families );
    m_pFonts[3]->setFonts( m_families );
    m_pFonts[4]->setFonts( m_families );
    m_pFonts[5]->setFonts( m_families );

    for ( int f = 0; f < 6; f++ ) {
        QString cf = defaultFonts[f];
        QString ff = fonts[f].lower();

        for ( QStringList::Iterator sit = m_families.begin(); sit != m_families.end(); ++sit, i++ )
            if ( ( *sit ).lower() == ff )
                cf = *sit;

        i = 0;
        for ( QStringList::Iterator sit = m_families.begin(); sit != m_families.end(); ++sit, i++ ) {
            if ( cf == *sit )
                m_pFonts[f]->setCurrentItem( i );
        }
    }

    i = 0;
    for ( QStringList::Iterator it = encodings.begin(); it != encodings.end(); ++it, ++i )
        if ( encodingName == *it )
            m_pEncoding->setCurrentItem( i );

    m_pFontSizeAdjust->setValue( fonts[6].toInt() );
    m_pEnforceCharset->setChecked( enforceCharset );
    m_MedSize->setValue( fSize );
    m_minSize->setValue( fMinSize );
}

void KAppearanceOptions::save()
{
    fontsForCharset[charset] = fonts;

    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "MediumFontSize", fSize );
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
    m_pConfig->writeEntry( "EnforceDefaultCharset", enforceCharset );
    m_pConfig->sync();
}


void KAppearanceOptions::changed()
{
  emit KCModule::changed(true);
}

