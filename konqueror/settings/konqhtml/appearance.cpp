#include <qlayout.h>
#include <qwhatsthis.h>
#include <qfontdatabase.h>
#include <qvgroupbox.h>
#include <kglobal.h>
#include <kconfig.h>
#include <qlabel.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <X11/Xlib.h>
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

  QStringList emptyList;

  QLabel* label = new QLabel( i18n("S&tandard Font"), this );
  lay->addWidget( label , ++r, E+1);

  m_pFonts[0] = new KFontCombo( emptyList, this );

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

  m_pFonts[1] = new KFontCombo( emptyList, this );

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

  m_pFonts[2] = new KFontCombo( emptyList, this );

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

  m_pFonts[3] = new KFontCombo( emptyList, this );

  label->setBuddy( m_pFonts[3] );
  lay->addMultiCellWidget( m_pFonts[3], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as sans-serif." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( m_pFonts[3], wtstr );

  connect( m_pFonts[3], SIGNAL( activated( const QString& ) ),
	   SLOT( slotSansSerifFont( const QString& ) ) );
  connect( m_pFonts[3], SIGNAL( activated( const QString& ) ),
	   SLOT( changed() ) );

  label = new QLabel( i18n( "C&ursive Font" ), this );
  lay->addWidget( label, ++r, E+1 );

  m_pFonts[4] = new KFontCombo( emptyList, this );

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

  m_pFonts[5] = new KFontCombo( emptyList, this );

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

  label = new QLabel( i18n( "Default Encoding:"), this );
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

  ++r; lay->setRowStretch(r, 8);

  QFontDatabase db;

  m_families = db.families();

  m_pFonts[0]->setFonts( m_families );
  m_pFonts[1]->setFonts( m_families );
  m_pFonts[2]->setFonts( m_families );
  m_pFonts[3]->setFonts( m_families );
  m_pFonts[4]->setFonts( m_families );
  m_pFonts[5]->setFonts( m_families );

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

    fonts = m_pConfig->readListEntry( "Fonts" );
    while (fonts.count() < 7)
       fonts.append(QString::null);

    encodingName = m_pConfig->readEntry( "DefaultEncoding", "" );
    //kdDebug(0) << "encoding = " << encodingName << endl;

    updateGUI();
}

void KAppearanceOptions::defaults()
{
  fSize = 10;
  fMinSize = HTML_DEFAULT_MIN_FONT_SIZE;
  encodingName = "";
  defaultFonts.clear();
  defaultFonts.append( KGlobalSettings::generalFont().family() );
  defaultFonts.append( KGlobalSettings::fixedFont().family() );
  defaultFonts.append( HTML_DEFAULT_VIEW_SERIF_FONT );
  defaultFonts.append( HTML_DEFAULT_VIEW_SANSSERIF_FONT );
  defaultFonts.append( HTML_DEFAULT_VIEW_CURSIVE_FONT );
  defaultFonts.append( HTML_DEFAULT_VIEW_FANTASY_FONT );
  defaultFonts.append( QString("0") );
  fonts.clear();
  while (fonts.count() < 7)
    fonts.append(QString::null);

  updateGUI();
}

void KAppearanceOptions::updateGUI()
{
    //kdDebug() << "KAppearanceOptions::updateGUI " << charset << endl;
    for ( int f = 0; f < 6; f++ ) {
        QString ff = fonts[f];
        if (ff.isEmpty())
           ff = defaultFonts[f];
        m_pFonts[f]->setCurrentFont(ff);
    }

    int i = 0;
    for ( QStringList::Iterator it = encodings.begin(); it != encodings.end(); ++it, ++i )
        if ( encodingName == *it )
            m_pEncoding->setCurrentItem( i );
    if(encodingName.isEmpty())
        m_pEncoding->setCurrentItem( 0 );
    m_pFontSizeAdjust->setValue( fonts[6].toInt() );
    m_MedSize->setValue( fSize );
    m_minSize->setValue( fMinSize );
}

void KAppearanceOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "MediumFontSize", fSize );
    m_pConfig->writeEntry( "MinimumFontSize", fMinSize );
    m_pConfig->writeEntry( "Fonts", fonts );

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

