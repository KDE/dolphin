//-----------------------------------------------------------------------------
//
// UserAgent Options
// (c) Kalle Dalheimer 1997
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998

#include "useragentdlg.h"

#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>

#include <qlayout.h> //CT
#include <qwhatsthis.h>

#include <kprotocolmanager.h> // for default useragent string

UserAgentOptions::UserAgentOptions( QWidget * parent, const char * name ) :
  KCModule( parent, name )
{
  QString wtstr;
  QGridLayout *lay = new QGridLayout(this,7,5,10,5);
  lay->addRowSpacing(0,10);
  lay->addRowSpacing(3,25);
  lay->addRowSpacing(6,10);
  lay->addColSpacing(0,10);
  lay->addColSpacing(4,10);

  lay->setRowStretch(0,0);
  lay->setRowStretch(1,0);
  lay->setRowStretch(2,0);
  lay->setRowStretch(3,0);
  lay->setRowStretch(4,0);
  lay->setRowStretch(5,1);
  lay->setRowStretch(6,0);

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,1);
  lay->setColStretch(3,0);
  lay->setColStretch(4,0);

  onserverLA = new QLabel( i18n( "On &server:" ), this );
  onserverLA->setAlignment( AlignRight|AlignVCenter );
  lay->addWidget(onserverLA,1,1);

  onserverED = new QLineEdit( this );
  onserverLA->setBuddy( onserverED );
  lay->addWidget(onserverED,1,2);

  wtstr = i18n( "Enter the server (or a domain) you want to fool about Konqueror's identity here."
    " Wildcard syntax (e.g. *.cnn.com) is allowed.");
  QWhatsThis::add( onserverLA, wtstr );
  QWhatsThis::add( onserverED, wtstr );

  connect( onserverED, SIGNAL( textChanged(const QString&) ),
           SLOT( textChanged( const QString&) ) );

  loginasLA = new QLabel( i18n( "&login as:" ), this );
  loginasLA->setAlignment( AlignRight|AlignVCenter );
  lay->addWidget(loginasLA,2,1);

  loginasED = new QLineEdit( this );
  lay->addWidget(loginasED,2,2);
  loginasLA->setBuddy( loginasED );

  wtstr = i18n( "Here you can enter the identification Konqueror should use for the given server."
    " Example: <em>Mozilla/4.0 (compatible; Konqueror 2.0; Linux)</em>");
  QWhatsThis::add( loginasLA, wtstr );
  QWhatsThis::add( loginasED, wtstr );

  connect( loginasED, SIGNAL( textChanged(const QString&) ),
           SLOT( textChanged(const QString&) ) );

  addPB = new QPushButton( i18n( "&Add" ), this );
  lay->addWidget(addPB,1,3);
  QWhatsThis::add( addPB, i18n("Adds the agent binding you've specified to the list of agent bindings."
    " This is not enabled if you haven't provided server <em>and</em> login information.") );

  addPB->setEnabled( false );
  connect( addPB, SIGNAL( clicked() ), SLOT( addClicked() ) );
  connect( addPB, SIGNAL( clicked() ), SLOT( changed() ) );

  deletePB = new QPushButton( i18n( "&Delete" ), this );
  lay->addWidget(deletePB,2,3);

  QWhatsThis::add( deletePB, i18n("Removes the selected binding from the list of agent bindings.") );

  deletePB->setEnabled( false );
  connect( deletePB, SIGNAL( clicked() ), SLOT( deleteClicked() ) );
  connect( deletePB, SIGNAL( clicked() ), SLOT( changed() ) );

  bindingsLA = new QLabel( i18n( "Configured agent bindings:" ), this );
  lay->addMultiCellWidget(bindingsLA,4,4,2,3);

  bindingsLB = new QListBox( this );
  lay->addMultiCellWidget(bindingsLB,5,5,2,3);

  wtstr = i18n( "This box contains a list of agent bindings, i.e. information on how"
    " Konqueror will identify itself to a server. You might want to set up bindings"
    " to fool servers that think other browsers than Netscape are dumb, so Konqueror"
    " will identify itself as 'Mozilla/4.0'.<p>"
    " Select a binding to change or delete it." );
  QWhatsThis::add( bindingsLA, wtstr );
  QWhatsThis::add( bindingsLB, wtstr );

  lay->activate();

  bindingsLB->setMultiSelection( false );
  connect( bindingsLB, SIGNAL( highlighted( const QString&) ),
           SLOT( listboxHighlighted( const QString& ) ) );

  load();
}


UserAgentOptions::~UserAgentOptions()
{
}

void UserAgentOptions::load()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

  // read entries for UserAgentDlg
  g_pConfig->setGroup( "Browser Settings/UserAgent" );
  int entries = g_pConfig->readNumEntry( "EntriesCount", 0 );
  settingsList.clear();
  for( int i = 0; i < entries; i++ )
        {
          QString key;
          key.sprintf( "Entry%d", i );
          QString entry = g_pConfig->readEntry( key, "" );
          if (entry.left( 13 ) == "*:Mozilla/4.0") // update version number
            settingsList.append( "*:"+DEFAULT_USERAGENT_STRING );
          else
            settingsList.append( entry );
        }
  if( entries == 0 ) {
    defaults();
  }
  else {
    bindingsLB->clear();
    bindingsLB->insertStringList( settingsList );
  }

  delete g_pConfig;
}

void UserAgentOptions::defaults()
{
  bindingsLB->clear();
  bindingsLB->insertItem( QString("*:"+DEFAULT_USERAGENT_STRING) );
}


void UserAgentOptions::save()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

    // write back the entries from UserAgent
    g_pConfig->setGroup("Browser Settings/UserAgent");

    if(!bindingsLB->count())
      defaults();

    g_pConfig->writeEntry( "EntriesCount", bindingsLB->count() );
    for( uint i = 0; i < bindingsLB->count(); i++ ) {
      QString key;
      key.sprintf( "Entry%d", i );
      g_pConfig->writeEntry( key, bindingsLB->text( i ) );
    }
    g_pConfig->sync();

  delete g_pConfig;
}

void UserAgentOptions::textChanged( const QString& )
{
  if( !loginasED->text().isEmpty() && !onserverED->text().isEmpty() )
    addPB->setEnabled( true );
  else
    addPB->setEnabled( false );

  deletePB->setEnabled( false );
}

void UserAgentOptions::addClicked()
{
  // no need to check if the fields contain text, the add button is only
  // enabled if they do

  QString text = onserverED->text();
  text += ':';
  text += loginasED->text();
  bindingsLB->insertItem( new QListBoxText( text ), 0);
  onserverED->setText( "" );
  loginasED->setText( "" );
  onserverED->setFocus();
}


void UserAgentOptions::deleteClicked()
{
  if( bindingsLB->count() )
    bindingsLB->removeItem( highlighted_item );
  if( !bindingsLB->count() ) // no more items
    listboxHighlighted("");
}


void UserAgentOptions::listboxHighlighted( const QString& _itemtext )
{
  QString itemtext( _itemtext );
  int colonpos = itemtext.find( ':' );
  onserverED->setText( itemtext.left( colonpos ) );
  loginasED->setText( itemtext.right( itemtext.length() - colonpos - 1) );
  deletePB->setEnabled( true );
  addPB->setEnabled( false );

  highlighted_item = bindingsLB->currentItem();
}


void UserAgentOptions::changed()
{
  emit KCModule::changed(true);
}

QString UserAgentOptions::quickHelp() const
{
    return i18n("<h1>User Agent</h1>The user agent control screen allows "
        "you to have full control over what type of browser "
        "konqueror will report itself to be to remote web sites.<p>"
        "Some web sites will not function properly if they think "
        "they are talking to browsers other than the latest "
        "Netscape or Internet Explorer. For these sites, you may "
        "want to override the default of reporting to be Konqueror "
        "and instead substitute Netscape."
        "<ul><li>In the <em>server</em> field, enter the server you "
        "are interested in fooling, such as <em>my.yahoo.com</em>."
        "You may specify a whole group of sites with wildcard "
        "syntax, i.e. *.cnn.com."
        "<li>In the <em>login</em> field, enter "
        "<em>Mozilla/4.0 (compatible; MSIE 4.0)</em>"
        "</ul>");
}

#include "useragentdlg.moc"
