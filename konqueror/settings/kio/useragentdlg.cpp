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

#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlistview.h>

#include <dcopclient.h>
#include <kdialog.h>
#include <kprotocolmanager.h>


UserAgentOptions::UserAgentOptions( QWidget * parent, const char * name )
                 :KCModule( parent, name )
{
  QVBoxLayout *lay = new QVBoxLayout( this, KDialog::marginHint(),
                                      KDialog::spacingHint() );
  onserverLA = new QLabel( i18n( "When connecting &to:" ), this );
  lay->addWidget(onserverLA);
  onserverED = new QLineEdit( this );
  onserverLA->setBuddy( onserverED );
  lay->addWidget(onserverED);

  QString wtstr = i18n( "Enter name of the site you want to fool about Konqueror's identity."
                        "Wildcard syntax, e.g. <em>*.kde.org</em>, is allowed." );
  QWhatsThis::add( onserverLA, wtstr );
  QWhatsThis::add( onserverED, wtstr );

  connect( onserverED, SIGNAL( textChanged(const QString&) ),
           SLOT( textChanged( const QString&) ) );  
  
  loginasLA = new QLabel( "&Send useragent string:", this );
  lay->addWidget(loginasLA);
  loginasED = new QComboBox( true, this );
  loginasED->setInsertionPolicy( QComboBox::AtBottom );
  lay->addWidget(loginasED);
  loginasLA->setBuddy(loginasED);  
  
  wtstr = i18n( "Here you can either enter or select the identification Konqueror should "
                "use whenever it connects to the site given above."
                "<br/>Example: <em>Mozilla/4.0 (compatible; Konqueror 2.0; Linux)</em>");
  QWhatsThis::add( loginasLA, wtstr );
  QWhatsThis::add( loginasED, wtstr );
  
  connect( loginasED, SIGNAL( textChanged(const QString&) ),
           SLOT( textChanged(const QString&) ) );
  
  connect( loginasED, SIGNAL( activated(const QString&) ),
           SLOT( activated(const QString&) ) );
  
  loginidLA = new QLabel( i18n("A&lias for useragent string:"), this );
  lay->addWidget(loginidLA);
  loginidED = new QLineEdit( this );
  lay->addWidget(loginidED);
  loginidLA->setBuddy(loginidED);
  
  connect( loginidED, SIGNAL( textChanged(const QString&) ),
           SLOT( textChanged(const QString&) ) );

  wtstr = i18n( "Here you can enter an alias or plain description for the "
                "useragent string you supplied above."
                "<br/>Example: <em>Konqueror 2.0</em>" );
  QWhatsThis::add( loginidLA, wtstr );
  QWhatsThis::add( loginidED, wtstr );
  
  QHBox *hbox = new QHBox( this );
  lay->addSpacing(5);
  lay->addWidget(hbox);
  lay->addSpacing(5);
  
  // Reset button
  resetPB = new QPushButton( i18n( "&Reset" ), hbox );
  QWhatsThis::add( resetPB, i18n("Clears the text boxes.") );
  resetPB->setEnabled( false );
  connect( resetPB, SIGNAL( clicked() ), SLOT( resetClicked() ) );  
  
  // Add/change button  
  addPB = new QPushButton( i18n( "&Add" ), hbox );
  QWhatsThis::add( addPB, i18n("Allows you to add a new or change an existing useragent binding. "
                               "Button will not enabled if no site name <em>and</em> useragent "
							   "string is supplied or selected from the list.") );
  addPB->setEnabled( false );
  connect( addPB, SIGNAL( clicked() ), SLOT( addClicked() ) );
  connect( addPB, SIGNAL( clicked() ), SLOT( changed() ) );
  
  // Delete button  
  deletePB = new QPushButton( i18n( "D&elete" ), hbox );
  QWhatsThis::add( deletePB, i18n("Removes the selected binding from the list of agent bindings.") );
  deletePB->setEnabled( false );
  connect( deletePB, SIGNAL( clicked() ), SLOT( deleteClicked() ) );
  connect( deletePB, SIGNAL( clicked() ), SLOT( changed() ) );
  
  // List box
  bindingsLA = new QLabel( i18n( "Configured useragent bindings:" ), this );
  lay->addWidget(bindingsLA);  
  bindingsLV = new QListView( this );
  bindingsLV->setShowSortIndicator(true);
  bindingsLV->setAllColumnsShowFocus(true);
  bindingsLV->addColumn( i18n( "Server Mask" ));
  bindingsLV->addColumn( i18n( "UserAgent" ));
  bindingsLV->addColumn( i18n( "UserAgent Alias" ));
  bindingsLV->setColumnAlignment(0, Qt::AlignLeft);
  bindingsLV->setColumnAlignment(1, Qt::AlignLeft);
  bindingsLV->setColumnAlignment(2, Qt::AlignLeft);
  bindingsLV->setTreeStepSize(0);
  bindingsLV->setSorting(0);
  lay->addWidget(bindingsLV);
  wtstr = i18n( "This box contains a list of user agent bindings.  These bindings enable "
                "Konqueror to change its identity when you visit these particular site. "
				"This \"cloaking\" feature is provided to allow you to fool sites that "
				"refuse to support browsers other than Netscape Navigator or Internet Explorer "
				"even when if they are perfectly capable of rendering them properly. "
	            "<br/>Select a binding to change or delete it." );
  QWhatsThis::add( bindingsLA, wtstr );
  QWhatsThis::add( bindingsLV, wtstr );

  connect( bindingsLV, SIGNAL( selectionChanged() ),
           SLOT( bindingsSelected() ) );
  onlySelectionChange = false;
  load();
}

UserAgentOptions::~UserAgentOptions()
{
}

void UserAgentOptions::load()
{
    aliasMap.clear();
	// Default string.
    aliasMap.insert( DEFAULT_USERAGENT_STRING, i18n( "Default") );
    // This is the one that proved to fool many "clever" browser detections
    // to detect the IE 5.0 which is most of the time the best for khtml (Dirk)  
    aliasMap.insert( QString("Mozilla/5.0 (Konqueror/")+QString(KDE_VERSION_STRING)+
                     QString("; compatible: MSIE 5.0)"), i18n( "Recommended" ) );
    // MSIE  
    aliasMap.insert( "Mozilla/4.0 (compatible; MSIE 4.01; Windows NT)",
                     i18n("Internet Explorer 4.01 on Windows NT") );
    aliasMap.insert( "Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)",
                     i18n("Internet Explorer 5.01 on Windows NT 5.0") );
    aliasMap.insert( "Mozilla/4.0 (compatible; MSIE 5.0; Win32)",
                     i18n("Internet Explorer 5.0 on Win32") );
    aliasMap.insert( "Mozilla/4.0 (compatible; MSIE 5.5; Windows NT 5.0)",
                     i18n("Internet Explorer 5.5 on Windows NT 5.0") );
    aliasMap.insert( "Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)",
                     i18n("Internet Explorer 5.5 on Windows 98") );
    aliasMap.insert( "Mozilla/4.0 (compatible; MSIE 5.0; Mac_PowerPC)",
                     i18n("Internet Explorer 5.5 on Mac") );
    // NS stuff
    aliasMap.insert( "Mozilla/3.01 (X11; I; Linux 2.2.14 i686)",
                     i18n("Netscape Navigator 3.01 on Linux") );
    aliasMap.insert( "Mozilla/4.75 (X11; U; Linux 2.2.14 i686)",
                     i18n("Netscape Navigator 4.75 on Linux") );
    aliasMap.insert( "Mozilla/5.0 (Windows; U; WinNT4.0; en-US; m18) Gecko/20001010",
                     i18n("Mozilla M18 on Windows NT 4.0" ) );
    aliasMap.insert( "Mozilla/4.76 (Macintosh; U; PPC)",
                     i18n("Netscape Navigator 4.76 on Mac") );

    // misc
    aliasMap.insert( "Opera/4.03 (Windows NT 4.0; U)",
                     i18n("Opera 4.03 on Windows NT 4.0") );
    aliasMap.insert( "Wget/1.5.3", i18n("Wget 1.5.3") );
    aliasMap.insert( "Lynx/2.8.3dev.6 libwww-FM/2.14", i18n("Lynx 2.8.3dev.6") );
    aliasMap.insert( "w3m/0.1.9", i18n("w3m 0.1.9") );
    
	// Load the user-agent string into the list...
	QMap<QString,QString>::ConstIterator it = aliasMap.begin();
	for( ; it != aliasMap.end(); ++it )
        loginasED->insertItem( it.key() );    
	loginasED->setEditText( "" );

    QStringList list = KProtocolManager::userAgentList();
    uint entries = list.count();
    if( entries == 0 )
        defaults();
    else
    {
		int count;
		QString sep = "::";
        bindingsLV->clear();
        for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
		{
		    int pos = (*it).find(sep);
		    if ( pos == -1 )
			{
			    pos = (*it).find( ':' );
				if ( pos == -1 ) continue;
				sep = (*it).mid(pos+1);
				(void) new QListViewItem( bindingsLV, (*it).left(pos), sep, aliasMap[sep] );
            }
			else
			{
		        QStringList split = QStringList::split( sep, (*it) );
			    count = split.count();
			    if ( count < 2 ) continue;
			    if ( count < 3 ) split.append( aliasMap[split[1]] );
                (void) new QListViewItem( bindingsLV, split[0], split[1], split[2] );
            }
        }
    }
}

void UserAgentOptions::defaults()
{
    bindingsLV->clear();
	QStringList split = QStringList::split( ':', DEFAULT_USERAGENT_STRING );
    if ( split.count() == 1 ) split.append( aliasMap[split[0]] );
    (void) new QListViewItem(bindingsLV, "*", split[0], split[1]);
}

void UserAgentOptions::save()
{
    if(!bindingsLV->childCount())
        defaults();

    QStringList list;
    QListViewItem* it = bindingsLV->firstChild();
    while(it)
    {
        list.append(QString(it->text(0) + "::" + it->text(1) + "::" + it->text(2)).stripWhiteSpace());
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
    if ( !loginasED->currentText().isEmpty() ||
	     !loginidED->text().isEmpty() ||
	     !onserverED->text().isEmpty() )
		 resetPB->setEnabled( true );
    else
		resetPB->setEnabled( false );
       
    if( !loginasED->currentText().isEmpty() && !onserverED->text().isEmpty() )
	{
        addPB->setEnabled( !onlySelectionChange );
        loginidED->setEnabled ( true );
    }
    else
	{
        addPB->setEnabled( onlySelectionChange );
		loginidED->setEnabled( false );
    }	
	
	if ( onlySelectionChange )
	    addPB->setText( i18n( "Ch&ange" ) );
    
	deletePB->setEnabled( false );
}

void UserAgentOptions::activated( const QString& data )
{
    loginidED->setText( aliasMap[data] );
}

void UserAgentOptions::addClicked()
{
    bool found=false;
    // scan for duplicates
    QListViewItem* it = bindingsLV->firstChild();
    while(it)
    {
        if(it->text(0) == onserverED->text().stripWhiteSpace())
        {
            it->setText(1, loginasED->currentText().stripWhiteSpace());
			it->setText(2, loginidED->text().stripWhiteSpace());
            found = true;
            break;
        }
        it = it->nextSibling();
    }
    if(!found)
        (void) new QListViewItem( bindingsLV,
		                          onserverED->text().stripWhiteSpace(),
		                          loginasED->currentText().stripWhiteSpace(),
								  loginidED->text().stripWhiteSpace() );
    bindingsLV->sort();
	resetClicked();
}

void UserAgentOptions::deleteClicked()
{
    QListViewItem* item = bindingsLV->selectedItem()->itemBelow();
	if ( !item )
	    item = bindingsLV->selectedItem()->itemAbove();
	delete bindingsLV->selectedItem();	
    bindingsLV->setSelected( item, true );       
    deletePB->setEnabled(item);

    if(!bindingsLV->childCount() ) // no more items
        defaults();
	else
	    bindingsSelected();	
}

void UserAgentOptions::resetClicked()
{
    onserverED->setText( "" );
	loginasED->setEditText( "" );
	loginidED->setText( "" );	
    textChanged( "" );
    addPB->setText( i18n( "&Add" ) );
    onserverED->setFocus();
}

void UserAgentOptions::bindingsSelected()
{
    QListViewItem* it = bindingsLV->selectedItem();
    if(!it) return;

    onserverED->setText( it->text(0) );
    loginasED->setEditText( it->text(1) );
	loginidED->setText( it->text(2) );
	onlySelectionChange = true;
    textChanged(QString());
	onlySelectionChange = false;
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
