//-----------------------------------------------------------------------------
//
// UserAgent Options
// (c) Kalle Dalheimer 1997
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998
//
// (C) Dirk Mueller <mueller@kde.org> 2000


#include "useragentdlg.h"


#include <kapp.h>
#include <klocale.h>

#include <qpushbutton.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qcombobox.h>
#include <qlistview.h>

#include <dcopclient.h>
#include <kprotocolmanager.h>


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

  loginasED = new QComboBox( true, this );
  loginasED->setInsertionPolicy( QComboBox::AtTop );
  lay->addWidget(loginasED,2,2);
  loginasLA->setBuddy( loginasED );
  loginasED->insertItem( DEFAULT_USERAGENT_STRING );
  // this is the one that proofed to fool many "clever" browser detections
  // to detect the IE 5.0 which is most of the time the best for khtml (Dirk)
  loginasED->insertItem( QString("Mozilla/5.0 (Konqueror/") + QString(KDE_VERSION_STRING)
                         + QString("; compatible: MSIE 5.0)") );

  // MSIE strings
  loginasED->insertItem( "Mozilla/4.0 (compatible; MSIE 4.01; Windows NT)" );
  loginasED->insertItem( "Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)" );
  loginasED->insertItem( "Mozilla/4.0 (compatible; MSIE 5.0; Win32)" );
  loginasED->insertItem( "Mozilla/4.0 (compatible; MSIE 5.5; Windows NT 5.0)" );
  loginasED->insertItem( "Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)" );

  // NS stuff
  loginasED->insertItem( "Mozilla/3.01 (X11; I; Linux 2.2.14 i686)" );
  loginasED->insertItem( "Mozilla/4.75 (X11; U; Linux 2.2.14 i686)" );
  loginasED->insertItem( "Mozilla/5.0 (Windows; U; WinNT4.0; en-US; m18) Gecko/20001010" );

  // misc
  loginasED->insertItem( "Opera/4.03 (Windows NT 4.0; U)" );
  loginasED->insertItem( "Konqueror/1.1.2" );
  loginasED->insertItem( "Wget/1.5.3" );
  loginasED->insertItem( "Lynx/2.8.3dev.6 libwww-FM/2.14" );
  loginasED->insertItem( "w3m/0.1.9" );

  loginasED->setEditText( "" );

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

  bindingsLV = new QListView( this );
  bindingsLV->setShowSortIndicator(true);
  bindingsLV->setAllColumnsShowFocus(true);
  bindingsLV->addColumn( i18n( "Server Mask" ));
  bindingsLV->addColumn( i18n( "User Agent" ));
  bindingsLV->setColumnAlignment(0, Qt::AlignRight);
  bindingsLV->setColumnAlignment(1, Qt::AlignLeft);
  bindingsLV->setTreeStepSize(0);
  bindingsLV->setSorting(0);
  lay->addMultiCellWidget(bindingsLV,5,5,2,3);

  wtstr = i18n( "This box contains a list of agent bindings, i.e. information on how"
    " Konqueror will identify itself to a server. You might want to set up bindings"
    " to fool servers that think other browsers than Netscape are dumb, so Konqueror"
    " will identify itself as 'Mozilla/4.0'.<p>"
    " Select a binding to change or delete it." );
  QWhatsThis::add( bindingsLA, wtstr );
  QWhatsThis::add( bindingsLV, wtstr );

  lay->activate();

  connect( bindingsLV, SIGNAL( selectionChanged() ),
           SLOT( bindingsSelected() ) );

  load();
}


UserAgentOptions::~UserAgentOptions()
{
}

void UserAgentOptions::load()
{
    QStringList list = KProtocolManager::userAgentList();
    uint entries = list.count();
    if( entries == 0 )
        defaults();
    else
    {
        bindingsLV->clear();
        for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
            (void) new QListViewItem(bindingsLV, (*it).left((*it).find(':')), (*it).mid((*it).find(':')+1));
    }
}

void UserAgentOptions::defaults()
{
    bindingsLV->clear();
    (void) new QListViewItem(bindingsLV, "*", DEFAULT_USERAGENT_STRING);
}


void UserAgentOptions::save()
{
    if(!bindingsLV->childCount())
        defaults();

    QStringList list;
    QListViewItem* it = bindingsLV->firstChild();
    while(it)
    {
        list.append(QString(it->text(0) + ":" + it->text(1)).stripWhiteSpace());
        it = it->nextSibling();
    }
    KProtocolManager::setUserAgentList( list );

    // Force the io-slaves to reload this config change :)
    QByteArray data;
    QCString launcher = KApplication::launcher();
    kapp->dcopClient()->send( launcher, launcher, "reparseConfiguration()", data );
}

void UserAgentOptions::textChanged( const QString& )
{
    if( !loginasED->currentText().isEmpty() && !onserverED->text().isEmpty() )
        addPB->setEnabled( true );
    else
        addPB->setEnabled( false );

    deletePB->setEnabled( false );
}

void UserAgentOptions::addClicked()
{
    // no need to check if the fields contain text, the add button is only
    // enabled if they do

    bool found=false;
    // scan for duplicates
    QListViewItem* it = bindingsLV->firstChild();
    while(it)
    {
        if(it->text(0) == onserverED->text().stripWhiteSpace())
        {
            it->setText(1, loginasED->currentText().stripWhiteSpace());
            found = true;
            break;
        }
        it = it->nextSibling();
    }

    if(!found)
        (void) new QListViewItem(bindingsLV, onserverED->text().stripWhiteSpace(), loginasED->currentText().stripWhiteSpace());

    bindingsLV->sort();
    onserverED->setText( "" );
    loginasED->setEditText( "" );
    onserverED->setFocus();
}

void UserAgentOptions::deleteClicked()
{
    delete bindingsLV->selectedItem();

    addPB->setEnabled(true);
    deletePB->setEnabled(bindingsLV->selectedItem());

    if(!bindingsLV->childCount() ) // no more items
        defaults();
}

void UserAgentOptions::bindingsSelected()
{
    QListViewItem* it = bindingsLV->selectedItem();
    if(!it) return;

    onserverED->setText( it->text(0) );
    loginasED->setEditText( it->text(1) );
    textChanged(QString());
    deletePB->setEnabled(bindingsLV->selectedItem());
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
