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
#include "fakeuaprovider.h"
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
  QString wtstr = i18n("<qt>If checked, no identification information about "
                       "your browser will be sent to sites you visit while "
                       "browsing."
                       "<P><u>NOTE:</u>Many sites rely on this information to "
                       "display pages properly, hence, it is highly recommended "
                       "that you do not totaly disable this feature but rather "
                       "customize it."
                       "<P>Only minimal identification information is sent to "
                       "remote sites as shown below in <b>bold</b>.</qt>");
  QWhatsThis::add( cb_sendUAString, wtstr );
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
  wtstr = i18n("Check anyone of the following boxes to modify the level of "
               "information that should be included in the default browser "
               "identification shown above in <b>bold<b>.");
  QWhatsThis::add( bg_default, wtstr );
  connect(bg_default, SIGNAL(clicked(int)), this, SLOT(changeDefaultUAModifiers(int)));
  lb_default = new QLabel( bg_default );
  QFont f = lb_default->font();
  f.setBold( true );
  lb_default->setFont( f );
  lb_default->setAlignment( Qt::AlignVCenter | Qt::AlignHCenter | Qt::WordBreak );
  bg_grid->addMultiCellWidget( lb_default, 1, 1, 0, 1);
  wtstr = i18n("This is the default identification sent to remote sites "
               "during browsing.  You can modify it using the checkboxes "
               "below.");
  QWhatsThis::add( lb_default, wtstr );

  cb_showOS = new QCheckBox( i18n("Add operating s&ystem &name"), bg_default);
  bg_grid->addMultiCellWidget( cb_showOS, 2, 2, 0, 1 );
  wtstr = i18n("Check this box to add your <code>operating system name</code> "
               "to the default identification string.");
  QWhatsThis::add( cb_showOS, wtstr );

  cb_showOSV = new QCheckBox( i18n("Add operating system &version"), bg_default );
  bg_grid->addWidget( cb_showOSV, 3, 1 );
  cb_showOSV->setEnabled( false );
  wtstr = i18n("Check this box to add your <code>operating system version "
               "number</code> to the default identification string.");
  QWhatsThis::add( cb_showOSV, wtstr );

  cb_showPlatform = new QCheckBox( i18n("Add &platform name"), bg_default );
  bg_grid->addMultiCellWidget( cb_showPlatform, 4, 4, 0, 1 );
  wtstr = i18n("Check this box to add your <code>platform</code> to the default "
               "identification string.");
  QWhatsThis::add( cb_showPlatform, wtstr );

  cb_showMachine = new QCheckBox( i18n("Add &machine (processor) type"), bg_default );
  bg_grid->addMultiCellWidget( cb_showMachine, 5, 5, 0, 1 );
  wtstr = i18n("Check this box to add your <code>machine or processor type"
               "</code> to the default identification string.");
  QWhatsThis::add( cb_showMachine, wtstr );

  cb_showLanguage = new QCheckBox( i18n("Add your &language setting"), bg_default );
  bg_grid->addMultiCellWidget( cb_showLanguage, 6, 6, 0, 1 );
  wtstr = i18n("Check this box to add your language settings to the default "
               "identification string.");
  QWhatsThis::add( cb_showLanguage, wtstr );

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
  lv_siteUABindings->addColumn(i18n("Site/domain name"));
  lv_siteUABindings->addColumn(i18n("UserAgent"));
  lv_siteUABindings->addColumn(i18n("Alias"));
  lv_siteUABindings->setColumnAlignment(0, Qt::AlignLeft);
  lv_siteUABindings->setColumnAlignment(1, Qt::AlignLeft);
  lv_siteUABindings->setColumnAlignment(2, Qt::AlignLeft);
  lv_siteUABindings->setColumnWidthMode(0, QListView::Manual);
  lv_siteUABindings->setColumnWidthMode(1, QListView::Manual);
  lv_siteUABindings->setColumnWidthMode(2, QListView::Manual);
  lv_siteUABindings->setColumnWidth(0, lv_siteUABindings->fontMetrics().width('W')*15);

  lv_siteUABindings->setTreeStepSize(0);
  lv_siteUABindings->setSorting(0);
  s_grid->addMultiCellWidget( lv_siteUABindings, 1, 2, 0, 0 );
  connect( lv_siteUABindings, SIGNAL( selectionChanged() ), SLOT( updateButtons() ) );
  wtstr = i18n("<qt>This box contains a list of browser-identifications that "
               "will be used in place of the default one when browsing the "
               "listed site(s)."
               "<P>These site-specific bindings enable any browser that uses "
               "this information to change its identity when you visit the "
               "associated site."
               "<P>Use the buttons on the right to add a new site-specific "
               "identifier or to change and/or delete an existing one.</qt>");
  QWhatsThis::add( lv_siteUABindings, wtstr );
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
  pb_import->hide();
  QWhatsThis::add( pb_import, i18n("Import pre-packaged site/domain specific identifiers") );
  connect( pb_import, SIGNAL( clicked() ), this, SLOT( importPressed() ) );

  pb_export = new QPushButton( i18n("Export..."), vbox );
  pb_export->hide();
  QWhatsThis::add( pb_export, i18n("Export pre-packaged site/domain specific identifiers" ) );
  connect( pb_export, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );

  s_grid->addWidget( vbox, 1, 1 );
  wtstr = i18n("<qt>Here you can modify the default browser-identification string "
               "and/or set a site <code>(ex:www.kde.org)</code> or a domain "
               "<code>(ex:kde.org)</code> specific identification."
               "<P>To add a new agent string, simply click the <code>New "
               "</code>button and supply the necessary information requested "
               "by the dialog box. To change an existing site specific entry "
               ", click on the <code>Change</code> button.  The <code>Delete "
               "</code> will remove the selected policy causing the default "
               "setting to be used for that site or domain.");
               /*
               "The <code>Import</code> and <code>Export</code> buttons allows "
               "you to easily share your policies with other people by allowing "
               "you to save and retrieve them from a zipped file.")
               */
  QWhatsThis::add( gb_siteSpecific, wtstr );
  load();
}

UserAgentOptions::~UserAgentOptions()
{
  if ( m_provider )
    delete m_provider;
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
  m_provider = 0L;
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
  UAProviderDlg* dlg = new UAProviderDlg( i18n("Add Identification"), this, 0L, m_provider );
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
  UAProviderDlg* dlg = new UAProviderDlg( i18n("Modify Identification"), this, 0L, m_provider );
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
  return i18n( "<h1>Browser Identification</h1> "
               "The browser-identification control screen allows you to have "
               "full control over what konqueror will report itself as to web "
               "sites."
               "<P>This ability to fake identity is necessary because some web "
               "sites do not display properly when they detect that  they are "
               "not talking to current versions of Netscape Navigator or Internet "
               "Explorer even if the \"unsupported browser\" actually supports "
               "all the necessary features to render those pages properly. Hence "
               "for such sites, you may want to override the default identification "
               "by adding a site or domain specific entry."
               "<P><u>NOTE:</u>To obtain specific help on a particular section of "
               "the dialog box, simply click on the little question-mark button on "
               "the top right corner of this window and then click on that section "
               "for which you are seeking help." );
}

#include "useragentdlg.moc"
