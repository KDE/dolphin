/*
   UserAgent Options
   (c) Kalle Dalheimer 1997

   Port to KControl
   (c) David Faure <faure@kde.org> 1998

   (c) Dirk Mueller <mueller@kde.org> 2000
   (c) Dawit Alemayehu <adawit@kde.org> 2000-2001
*/

#include <qvbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qvbuttongroup.h>

#include <kapp.h>
#include <klocale.h>
#include <kdialog.h>
#include <klistview.h>
#include <dcopclient.h>

#include "useragentdlg.h"
#include "uagentproviderdlg.h"

UserAgentOptions::UserAgentOptions( QWidget * parent, const char * name )
                 :KCModule( parent, name )
{
  QVBoxLayout *lay = new QVBoxLayout( this, KDialog::marginHint(),
                                      KDialog::spacingHint() );
  lay->setAutoAdd( true );
  //lay->setMargin( KDialog::marginHint() );

  // Send User-agent info ?
  cb_sendUAString = new QCheckBox( i18n("Do not se&nd the user-agent string"), this );
  QWhatsThis::add( cb_sendUAString, i18n("<qt>If checked, no identification information about your "
                                         "browser will be sent to sites you visit while browsing.  "
                                         "<p><u>Note:</u> many sites rely on this information to display "
                                         "pages properly, hence, it is highly recommended that you do "
                                         "not totaly disable this feature but rather customize it.  "
                                         "<P>As shown below in <b>bold</>, only minimal identification "
                                         "information is sent to remote sites.</qt>") );
  connect( cb_sendUAString, SIGNAL( clicked() ), this, SLOT( changeSendUAString() ) );

  // Default User-agent customization.
  bg_default = new QButtonGroup( i18n("Customize default identification"), this );
  lay->setStretchFactor( bg_default, 0 );
  QGridLayout *bg_grid = new QGridLayout( bg_default, 7, 2,
                                          KDialog::marginHint(),
                                          KDialog::spacingHint() );
  bg_grid->addRowSpacing(0, fontMetrics().lineSpacing());
  bg_grid->setColStretch(0, 0);
  bg_grid->setColStretch(1, 2);
  bg_grid->addColSpacing(0, 3*KDialog::spacingHint() );
  QWhatsThis::add( bg_default, i18n("Check anyone of the following boxes to modify the level "
                                    "of information about your user-agent(browser) that is "
                                    "sent to remote sites.") );
  connect(bg_default, SIGNAL(clicked(int)), this, SLOT(changeDefaultUAModifiers(int)));
  lb_default = new QLabel( bg_default );
  QFont f = lb_default->font();
  f.setBold( true );
  lb_default->setFont( f );
  lb_default->setAlignment( Qt::AlignVCenter | Qt::AlignHCenter | Qt::WordBreak );
  bg_grid->addMultiCellWidget( lb_default, 1, 1, 0, 1);
  QWhatsThis::add( lb_default, i18n("This is the default information that is sent to remote "
                                    "sites when you browse the internet.  You can modify it "
                                    "using the checkboxes below.") );

  cb_showOS = new QCheckBox( i18n("Add operating system &name"), bg_default);
  bg_grid->addMultiCellWidget( cb_showOS, 2, 2, 0, 1 );
  QWhatsThis::add( cb_showOS, i18n("Check this box to add your <em>operting system name</em> "
                                   "to the default identification string.") );

  cb_showOSV = new QCheckBox( i18n("Add operating system &version"), bg_default );
  bg_grid->addWidget( cb_showOSV, 3, 1 );
  cb_showOSV->setEnabled( false );
  QWhatsThis::add( cb_showOSV, i18n("Check this box to add your <em>operting system version number</em> "
                                    "to the default identification string.") );

  cb_showPlatform = new QCheckBox( i18n("Add &platform name"), bg_default );
  bg_grid->addMultiCellWidget( cb_showPlatform, 4, 4, 0, 1 );
  QWhatsThis::add( cb_showPlatform, i18n("Check this box to add your <em>platform</em> to the "
                                         "default identification string.") );

  cb_showMachine = new QCheckBox( i18n("Add &machine (processor) type"), bg_default );
  bg_grid->addMultiCellWidget( cb_showMachine, 5, 5, 0, 1 );
  QWhatsThis::add( cb_showMachine, i18n("Check this box to add your <em>machine or processor "
                                        "type</em> to the default identification string.") );

  cb_showLanguage = new QCheckBox( i18n("Add your &language setting"), bg_default );
  bg_grid->addMultiCellWidget( cb_showLanguage, 6, 6, 0, 1 );
  QWhatsThis::add( cb_showLanguage, i18n("Check this box to add your <em>machine or processor "
                                         "type</em> to the default identification string.") );

  // Site/Domain specific settings
  gb_siteSpecific = new QGroupBox( i18n("Site/domain specific identification"), this );
  lay->setStretchFactor( gb_siteSpecific, 10 );
  QGridLayout* s_grid = new QGridLayout( gb_siteSpecific, 3, 2,
                                         KDialog::marginHint(),
                                         KDialog::spacingHint() );
  s_grid->addRowSpacing( 0, fontMetrics().lineSpacing() );
  s_grid->setColStretch( 0, 2 ); // Only resize the list box
  s_grid->setRowStretch( 2, 2 );

  lv_siteUABindings = new KListView( gb_siteSpecific );
  lv_siteUABindings->setShowSortIndicator( true );
  lv_siteUABindings->setAllColumnsShowFocus( true );
  lv_siteUABindings->addColumn(i18n("Server Mask"));
  lv_siteUABindings->addColumn(i18n("UserAgent" ));
  lv_siteUABindings->addColumn(i18n("Alias" ));
  lv_siteUABindings->setColumnAlignment(0, Qt::AlignLeft);
  lv_siteUABindings->setColumnAlignment(1, Qt::AlignLeft);
  lv_siteUABindings->setColumnAlignment(2, Qt::AlignLeft);
  lv_siteUABindings->setTreeStepSize(0);
  lv_siteUABindings->setSorting(0);
  s_grid->addMultiCellWidget( lv_siteUABindings, 1, 2, 0, 0 );
  connect( lv_siteUABindings, SIGNAL( selectionChanged() ), SLOT( updateButtons() ) );
  QWhatsThis::add( lv_siteUABindings, i18n( "<qt>This box contains a list of identification(s) that will "
                                            "be used in place of the default one when browsing the given "
                                            "sites.  These site-specific bindings enable Konqueror to "
                                            "change its identity when you visit the associated site(s) "
                                            "This feature allows you to fool sites that refuse to support "
                                            "browsers other than Netscape Navigator or Internet Explorer even "
                                            "when they are perfectly capable of rendering them properly. "
                                            "<br/>Use the buttons on the right to add a new site-specific "
                                            "identifier, change and/or delete an exisiting one.</qt>" ) );
  QVBox* vbox = new QVBox( gb_siteSpecific );
  vbox->setSpacing( KDialog::spacingHint() );
  pb_add = new QPushButton( i18n("&New..."), vbox );
  QWhatsThis::add( pb_add, i18n("Add a new site/domain specific identifier") );
  connect( pb_add, SIGNAL(clicked()), SLOT( addPressed() ) );

  pb_change = new QPushButton( i18n("C&hange..."), vbox );
  QWhatsThis::add( pb_change, i18n("Modify the selected entry") );
  connect( pb_change, SIGNAL( clicked() ), this, SLOT( changePressed() ) );

  pb_delete = new QPushButton( i18n("De&lete"), vbox );
  QWhatsThis::add( pb_delete, i18n("Delete the selected entry") );
  connect( pb_delete, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

  pb_import = new QPushButton( i18n("Import..."), vbox );
  pb_import->setEnabled( false );
  QWhatsThis::add( pb_import, i18n("Import pre-packaged site/domain specific identifiers") );
  connect( pb_import, SIGNAL( clicked() ), this, SLOT( importPressed() ) );

  pb_export = new QPushButton( i18n("Export..."), vbox );
  pb_export->setEnabled( false );
  QWhatsThis::add( pb_export, i18n("Export pre-packaged site/domain specific identifiers" ) );
  connect( pb_export, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );

  s_grid->addWidget( vbox, 1, 1 );
  QWhatsThis::add( gb_siteSpecific, i18n("<qt>You can modify the default identification string, most "
                                         "and or set a site (ex:www.kde.org) or a domain (ex:kde.org) "
                                         "specific identification string.<p>  To add a new agent string, "
                                         "simply click the <i>Add...</i> button and supply the necessary "
                                         "information requested by the dialog box. To change an exisiting "
                                         "string, click on the <i>Change...</i> button.  Clicking the <i>Delete</i> "
                                         "will remove the selected policy causing the default setting to be "
                                         "used for that site or domain. The <i>Import</i> and <i>Export</i> "
                                         "buttons allows you to easily share your policies with other people "
                                         "by allowing you to save and retrieve them from a zipped file.") );
  load();
}

UserAgentOptions::~UserAgentOptions()
{
}

void UserAgentOptions::load()
{
  QStringList list = KProtocolManager::userAgentList();
  uint entries = list.count();
  if( entries > 0 )
  {
    int count;
    QString sep = "::";
    lv_siteUABindings->clear();
    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
    {
      int pos = (*it).find(sep);
      if ( pos == -1 )
      {
        pos = (*it).find( ':' );
        if ( pos == -1 ) continue;
        sep = (*it).mid(pos+1);
        (void) new QListViewItem( lv_siteUABindings, (*it).left(pos), sep, sep );
      }
      else
      {
        QStringList split = QStringList::split( sep, (*it) );
        count = split.count();
        if ( count < 2 ) continue;
        if ( count < 3 ) split.append( split[1] );
        (void) new QListViewItem( lv_siteUABindings, split[0], split[1], split[2] );
      }
    }
  }
  // Update buttons and checkboxes...
  cb_sendUAString->setChecked( !KProtocolManager::sendUserAgent() );
  KProtocolManager::defaultUserAgentModifiers( m_iMods );
  lb_default->setText( KProtocolManager::customDefaultUserAgent( m_iMods ) );
  cb_showOS->setChecked( m_iMods.showOS );
  cb_showOSV->setChecked( m_iMods.showOSVersion );
  cb_showPlatform->setChecked( m_iMods.showPlatform );
  cb_showMachine->setChecked( m_iMods.showMachine );
  cb_showLanguage->setChecked( m_iMods.showLanguage );
  changeSendUAString();
}

void UserAgentOptions::updateButtons()
{
  bool hasItems = lv_siteUABindings->childCount() > 0;
  bool itemSelected = ( hasItems && lv_siteUABindings->selectedItem()!=0 );
  pb_delete->setEnabled( itemSelected );
  pb_change->setEnabled( itemSelected );
}

void UserAgentOptions::defaults()
{
  lv_siteUABindings->clear();
  m_iMods.showOS = false;
  m_iMods.showOSVersion = false;
  m_iMods.showPlatform = false;
  m_iMods.showMachine = false;
  m_iMods.showLanguage = false;
  cb_showOS->setChecked( m_iMods.showOS );
  cb_showOSV->setChecked( m_iMods.showOSVersion );
  cb_showPlatform->setChecked( m_iMods.showPlatform );
  cb_showMachine->setChecked( m_iMods.showMachine );
  cb_showLanguage->setChecked( m_iMods.showLanguage );
  lb_default->setText( KProtocolManager::customDefaultUserAgent(m_iMods) );
  cb_sendUAString->setChecked( false );
  changeSendUAString();
  emit KCModule::changed( true );
}

void UserAgentOptions::save()
{
  QStringList list;
  QListViewItem* it = lv_siteUABindings->firstChild();
  while(it)
  {
    list.append(QString(it->text(0) + "::" + it->text(1) + "::" + it->text(2)).stripWhiteSpace());
    it = it->nextSibling();
  }
  KProtocolManager::setEnableSendUserAgent( !cb_sendUAString->isChecked() );
  KProtocolManager::setDefaultUserAgentModifiers( m_iMods );
  KProtocolManager::setUserAgentList( list );

  QByteArray data;
  QCString launcher = KApplication::launcher();
  kapp->dcopClient()->send( launcher, launcher, "reparseConfiguration()", data );
}

void UserAgentOptions::addPressed()
{
  UAProviderDlg* dlg = new UAProviderDlg( i18n("Add Identification"),
                                          lv_siteUABindings->childCount(),
                                          this );
  if ( dlg->exec() == QDialog::Accepted )
  {
    bool found = false;
    QListViewItem* it = lv_siteUABindings->firstChild();
    while( it )
    {
      if( it->text(0) == dlg->siteName() )
      {
        it->setText( 1, dlg->identity() );
        it->setText( 2, dlg->alias() );
        found = true;
        break;
      }
      it = it->nextSibling();
    }
    if( !found )
      (void) new QListViewItem( lv_siteUABindings, dlg->siteName(), dlg->identity(), dlg->alias() );

    lv_siteUABindings->sort();
    emit KCModule::changed(true);
  }
  delete dlg;
}

void UserAgentOptions::changePressed()
{
  UAProviderDlg* dlg = new UAProviderDlg( i18n("Modify Identification"),
                                          lv_siteUABindings->childCount(),
                                          this );
  dlg->setEnableHostEdit( false );
  dlg->setSiteName( lv_siteUABindings->currentItem()->text(0) );
  dlg->setIdentity( lv_siteUABindings->currentItem()->text(1) );
  if ( dlg->exec() == QDialog::Accepted )
  {
    QListViewItem* it = lv_siteUABindings->currentItem();
    if( it->text(0) == dlg->siteName() )
    {
      it->setText( 1, dlg->identity() );
      it->setText( 2, dlg->alias() );
    }
    emit KCModule::changed(true);
  }
  delete dlg;
}

void UserAgentOptions::deletePressed()
{
  QListViewItem* item = lv_siteUABindings->selectedItem()->itemBelow();
  if ( !item )
    item = lv_siteUABindings->selectedItem()->itemAbove();
  delete lv_siteUABindings->selectedItem();
  if ( item )
    lv_siteUABindings->setSelected( item, true );
  updateButtons();
  emit KCModule::changed(true);
}

void UserAgentOptions::importPressed()
{
}

void UserAgentOptions::exportPressed()
{
}

void UserAgentOptions::changeSendUAString()
{
  if( cb_sendUAString->isChecked() )
  {
    bg_default->setEnabled( false );
    gb_siteSpecific->setEnabled( false );
  }
  else
  {
    bg_default->setEnabled( true );
    gb_siteSpecific->setEnabled( true );
  }
  updateButtons();
  emit KCModule::changed(true);
}

void UserAgentOptions::changeDefaultUAModifiers( int id )
{
  switch ( id )
  {
    case SHOW_OS:
      m_iMods.showOS= cb_showOS->isChecked();
      cb_showOSV->setEnabled( m_iMods.showOS );
      break;
    case SHOW_OS_VERSION:
      m_iMods.showOSVersion= cb_showOSV->isChecked();
      break;
    case SHOW_PLATFORM:
      m_iMods.showPlatform= cb_showPlatform->isChecked();
      break;
    case SHOW_MACHINE:
      m_iMods.showMachine= cb_showMachine->isChecked();
      break;
    case SHOW_LANGUAGE:
      m_iMods.showLanguage= cb_showLanguage->isChecked();
      break;
    default:
      break;
  }
  QString modVal = KProtocolManager::customDefaultUserAgent( m_iMods );
  if ( lb_default->text() != modVal )
  {
    lb_default->setText(modVal);
    emit KCModule::changed( true );
  }
}

QString UserAgentOptions::quickHelp() const
{
  return i18n( "<h1>User Agent</h1>The user agent control screen allows you "
               "to have full control over what konqueror will report itself "
               "as to remote web site machines.  The default is <br/><b>%1</b><br/> "
               "<p>Some web sites, however, do not function properly if they think "
               "they are talking to browsers other than the latest version of Netscape "
               "Navigator or Internet Explorer.  For these sites, you may want to override "
               "the default by adding a site or domain specific identification." ).arg( DEFAULT_USERAGENT_STRING );
}

#include "useragentdlg.moc"
