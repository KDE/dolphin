/*
   Original Authors:
   Copyright (c) Kalle Dalheimer 1997
   Copyright (c) David Faure <faure@kde.org> 1998
   Copyright (c) Dirk Mueller <mueller@kde.org> 2000

   Completely re-written by:
   Copyright (C) 2000- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License (GPL)
   version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qvbox.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qvbuttongroup.h>

#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <klistview.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <ksaveioconfig.h>
#include <kio/http_slave_defaults.h>

#include "useragentdlg.h"
#include "fakeuaprovider.h"
#include "uagentproviderdlg.h"

UserAgentOptions::UserAgentOptions( QWidget * parent, const char * name )
                 :KCModule( parent, name )
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this, KDialog::marginHint(),
                                             KDialog::spacingHint() );

  QHBoxLayout* hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  // Send User-agent info ?
  cb_sendUAString = new QCheckBox( i18n("Send browser &identification"), this );
  QString wtstr = i18n("<qt>If unchecked, no identification information about "
                       "your browser will be sent to sites you visit while "
                       "browsing."
                       "<P><u>NOTE:</u> Many sites rely on this information to "
                       "display pages properly, hence, it is highly recommended "
                       "that you do not totaly disable this feature but rather "
                       "customize it."
                       "<P>Only minimal identification information is sent to "
                       "remote sites as shown below in <b>bold</b>.</qt>");
  QWhatsThis::add( cb_sendUAString, wtstr );
  connect( cb_sendUAString, SIGNAL(clicked()), this, SLOT(changeSendUAString()) );
  connect( cb_sendUAString, SIGNAL(clicked()), this, SLOT(changed()) );

  hlay->addWidget( cb_sendUAString );

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                                         QSizePolicy::Minimum );
  hlay->addItem( spacer );
  mainLayout->addLayout( hlay );

  // Default User-agent customization.
  bg_default = new QButtonGroup( i18n("Customize Default Identification"), this );
  // vlay->setStretchFactor( bg_default, 0 );
  QGridLayout *bg_grid = new QGridLayout( bg_default, 7, 2,
                                          KDialog::marginHint(),
                                          KDialog::spacingHint() );
  bg_grid->addRowSpacing(0, fontMetrics().lineSpacing());
  bg_grid->setColStretch(0, 0);
  bg_grid->setColStretch(1, 2);
  bg_grid->addColSpacing(0, 3*KDialog::spacingHint() );
  wtstr = i18n("Check any one of the following boxes to modify the level of "
               "information that should be included in the default browser "
               "identification shown above in <b>bold</b>.");
  QWhatsThis::add( bg_default, wtstr );
  connect(bg_default, SIGNAL(clicked(int)), this, SLOT(changeDefaultUAModifiers(int)));
  lb_default = new KLineEdit( bg_default );
  lb_default->setReadOnly( true );
  lb_default->setFrameShape( QFrame::Box );
  lb_default->setFrameShadow( QFrame::Raised );
  lb_default->setLineWidth( 0 );
  lb_default->setMidLineWidth( 0 );
  QFont f = lb_default->font();
  f.setBold( true );
  lb_default->setFont( f );
  bg_grid->addMultiCellWidget (lb_default, 1, 1, 0, 1);
  wtstr = i18n("This is the default identification sent to remote sites "
               "during browsing. You can modify it using the checkboxes "
               "below.");
  QWhatsThis::add( lb_default, wtstr );

  // Operating system ...
  hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  cb_showOS = new QCheckBox( i18n("Add operating s&ystem name"), bg_default);
  wtstr = i18n("Check this box to add your <code>operating system name</code> "
               "to the default identification string.");
  QWhatsThis::add( cb_showOS, wtstr );
  hlay->addWidget( cb_showOS );
  spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  hlay->addItem( spacer );
  bg_grid->addMultiCellLayout( hlay, 2, 2, 0, 1 );

  // Operating system version...
  hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  cb_showOSV = new QCheckBox( i18n("Add operating system &version"), bg_default );
  cb_showOSV->setEnabled( false );
  wtstr = i18n("Check this box to add your <code>operating system version "
               "number</code> to the default identification string.");
  QWhatsThis::add( cb_showOSV, wtstr );
  hlay->addWidget( cb_showOSV );
  spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  hlay->addItem( spacer );
  bg_grid->addLayout( hlay, 3, 1 );

  // Platform name...
  hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  cb_showPlatform = new QCheckBox( i18n("Add &platform name"), bg_default );

  wtstr = i18n("Check this box to add your <code>platform</code> to the default "
               "identification string.");
  QWhatsThis::add( cb_showPlatform, wtstr );
  hlay->addWidget( cb_showPlatform );
  spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  hlay->addItem( spacer );
  bg_grid->addMultiCellLayout( hlay, 4, 4, 0, 1 );

  // Processor type...
  hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  cb_showMachine = new QCheckBox( i18n("Add &machine (processor) type"), bg_default );
  wtstr = i18n("Check this box to add your <code>machine or processor type"
               "</code> to the default identification string.");
  QWhatsThis::add( cb_showMachine, wtstr );
  hlay->addWidget( cb_showMachine );
  spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  hlay->addItem( spacer );
  bg_grid->addMultiCellLayout( hlay, 5, 5, 0, 1 );

  // Language Setting...
  hlay = new QHBoxLayout;
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  cb_showLanguage = new QCheckBox( i18n("Add your &language setting"), bg_default );
  wtstr = i18n("Check this box to add your language settings to the default "
               "identification string.");
  QWhatsThis::add( cb_showLanguage, wtstr );
  hlay->addWidget( cb_showLanguage );
  spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  hlay->addItem( spacer );
  bg_grid->addMultiCellLayout( hlay, 6, 6, 0, 1 );


  mainLayout->addWidget( bg_default );

  // Site/Domain specific settings
  gb_siteSpecific = new QGroupBox( i18n("Site/Domain Specific Identification"), this );
  QGridLayout* s_grid = new QGridLayout( gb_siteSpecific, 3, 2,
                                         KDialog::marginHint(),
                                         KDialog::spacingHint() );
  s_grid->addRowSpacing( 0, fontMetrics().lineSpacing() );
  s_grid->setColStretch( 0, 2 ); // Only resize the list box
  s_grid->setRowStretch( 2, 2 );

  lv_siteUABindings = new KListView( gb_siteSpecific );
  lv_siteUABindings->setShowSortIndicator( true );
  lv_siteUABindings->setSelectionMode (QListView::Extended);

  lv_siteUABindings->addColumn(i18n("Site/Domain Name"));
  lv_siteUABindings->addColumn(i18n("User Agent"));
  lv_siteUABindings->addColumn(i18n("Alias"));

  lv_siteUABindings->setAllColumnsShowFocus( true );
  lv_siteUABindings->setTreeStepSize(0);
  lv_siteUABindings->setSorting(0);
  s_grid->addMultiCellWidget( lv_siteUABindings, 1, 2, 0, 0 );

  lv_siteUABindings->setColumnAlignment(0, Qt::AlignLeft);
  lv_siteUABindings->setColumnAlignment(1, Qt::AlignLeft);
  lv_siteUABindings->setColumnAlignment(2, Qt::AlignLeft);
  lv_siteUABindings->setColumnWidthMode(0, QListView::Manual);
  lv_siteUABindings->setColumnWidthMode(1, QListView::Manual);
  lv_siteUABindings->setColumnWidthMode(2, QListView::Manual);
  lv_siteUABindings->setColumnWidth(0, lv_siteUABindings->fontMetrics().width('W')*15);

  connect( lv_siteUABindings, SIGNAL(selectionChanged()),
           SLOT(selectionChanged()) );
  connect( lv_siteUABindings, SIGNAL(doubleClicked (QListViewItem *)),
           SLOT(changePressed()) );
  connect( lv_siteUABindings, SIGNAL( returnPressed ( QListViewItem * ) ),
           SLOT( changePressed() ));

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
  QWhatsThis::add( pb_add, i18n("Add browser identification for a specific site.") );
  connect( pb_add, SIGNAL(clicked()), SLOT( addPressed() ) );

  pb_change = new QPushButton( i18n("C&hange..."), vbox );
  pb_change->setEnabled( false );
  QWhatsThis::add( pb_change, i18n("Change the selected identifier.") );
  connect( pb_change, SIGNAL( clicked() ), this, SLOT( changePressed() ) );

  pb_delete = new QPushButton( i18n("De&lete"), vbox );
  pb_delete->setEnabled( false );
  QWhatsThis::add( pb_delete, i18n("Delete the selected identifier.") );
  connect( pb_delete, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

  pb_deleteAll = new QPushButton( i18n("D&elete All"), vbox );
  pb_deleteAll->setEnabled( false );
  QWhatsThis::add( pb_deleteAll, i18n("Delete all identifiers") );
  connect( pb_deleteAll, SIGNAL( clicked() ), this, SLOT( deleteAllPressed() ) );

#if 0
  pb_import = new QPushButton( i18n("Import..."), vbox );
  pb_import->hide();
  QWhatsThis::add( pb_import, i18n("Import pre-packaged site/domain specific identifiers") );
  connect( pb_import, SIGNAL( clicked() ), this, SLOT( importPressed() ) );

  pb_export = new QPushButton( i18n("Export..."), vbox );
  pb_export->hide();
  QWhatsThis::add( pb_export, i18n("Export pre-packaged site/domain specific identifiers" ) );
  connect( pb_export, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
#endif

  wtstr = i18n("<qt>Here you can modify the default browser-identification string "
               "and/or set a site <code>(ex:www.kde.org)</code> or a domain "
               "<code>(ex:kde.org)</code> specific identification."
               "<P>To add a new agent string, simply click on the <code>New "
               "</code>button and supply the necessary information requested "
               "by the dialog box. To change an existing site specific entry, "
               "click on the <code>Change</code> button. The <code>Delete "
               "</code> button will remove the selected policy, causing the "
               "default setting to be used for that site or domain.");
#if 0
               "The <code>Import</code> and <code>Export</code> buttons allows "
               "you to easily share your policies with other people by allowing "
               "you to save and retrieve them from a zipped file.")
#endif

  QWhatsThis::add( gb_siteSpecific, wtstr );
  s_grid->addWidget( vbox, 1, 1 );

  mainLayout->addWidget( gb_siteSpecific );
  mainLayout->addSpacing( KDialog::spacingHint() );

  load();
}

UserAgentOptions::~UserAgentOptions()
{
  if ( m_provider )
    delete m_provider;
  delete m_config;
}

void UserAgentOptions::load()
{
  d_itemsSelected = 0;
  lv_siteUABindings->clear();

  m_config = new KConfig("kio_httprc", false, false);
  m_provider = new FakeUASProvider();

  QStringList list = m_config->groupList();
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
      if ( (*it) == "<default>")
         continue;
      QString domain = *it;
      m_config->setGroup(*it);
      QString userAgent = m_config->readEntry("UserAgent");
      if (!userAgent.isEmpty());
      {
         QString comment = m_provider->aliasFor(userAgent);
         (void) new QListViewItem( lv_siteUABindings, domain, userAgent, comment );
      }
  }

  // Update buttons and checkboxes...
  m_config->setGroup(QString::null);
  bool b = m_config->readBoolEntry("SendUserAgent", true);
  cb_sendUAString->setChecked( b );
  m_ua_keys = m_config->readEntry("UserAgentKeys", DEFAULT_USER_AGENT_KEYS).lower();
  lb_default->setText( KProtocolManager::defaultUserAgent( m_ua_keys ) );
  cb_showOS->setChecked( m_ua_keys.contains('o') );
  cb_showOSV->setChecked( m_ua_keys.contains('v') );
  cb_showOSV->setEnabled( m_ua_keys.contains('o') );
  cb_showPlatform->setChecked( m_ua_keys.contains('p') );
  cb_showMachine->setChecked( m_ua_keys.contains('m') );
  cb_showLanguage->setChecked( m_ua_keys.contains('l') );
  changeSendUAString();

  updateButtons();
}

void UserAgentOptions::updateButtons()
{
  bool hasItems = lv_siteUABindings->childCount() > 0;

  pb_change->setEnabled ((hasItems && d_itemsSelected == 1));
  pb_delete->setEnabled ((hasItems && d_itemsSelected > 0));
  pb_deleteAll->setEnabled ( hasItems );
}

void UserAgentOptions::defaults()
{
  lv_siteUABindings->clear();
  m_ua_keys = DEFAULT_USER_AGENT_KEYS;
  lb_default->setText( KProtocolManager::defaultUserAgent(m_ua_keys) );
  cb_showOS->setChecked( m_ua_keys.contains('o') );
  cb_showOSV->setChecked( m_ua_keys.contains('v') );
  cb_showOSV->setEnabled( m_ua_keys.contains('o') );
  cb_showPlatform->setChecked( m_ua_keys.contains('p') );
  cb_showMachine->setChecked( m_ua_keys.contains('m') );
  cb_showLanguage->setChecked( m_ua_keys.contains('l') );
  cb_sendUAString->setChecked( true );
  updateButtons();
  changeSendUAString();
}

void UserAgentOptions::save()
{
  QStringList deleteList;

  // This is tricky because we have to take care to delete entries
  // as well.
  QStringList list = m_config->groupList();
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
      if ( (*it) == "<default>")
         continue;
      QString domain = *it;
      m_config->setGroup(*it);
      if (m_config->hasKey("UserAgent"))
         deleteList.append(*it);
  }

  QListViewItem* it = lv_siteUABindings->firstChild();
  while(it)
  {
    QString domain = it->text(0);
    if (domain[0] == '.')
      domain = domain.mid(1);
    QString userAgent = it->text(1);
    m_config->setGroup(domain);
    m_config->writeEntry("UserAgent", userAgent);
    deleteList.remove(domain);

    it = it->nextSibling();
  }

  m_config->setGroup(QString::null);
  m_config->writeEntry("SendUserAgent", cb_sendUAString->isChecked());
  m_config->writeEntry("UserAgentKeys", m_ua_keys );
  m_config->sync();

  // Delete all entries from deleteList.
  if (!deleteList.isEmpty())
  {
     // Remove entries from local file.
     KSimpleConfig cfg("kio_httprc");
     for ( QStringList::Iterator it = deleteList.begin();
           it != deleteList.end(); ++it )
     {
        cfg.setGroup(*it);
        cfg.deleteEntry("UserAgent", false);
        cfg.deleteGroup(*it, false); // Delete if empty.
     }
     cfg.sync();

     m_config->reparseConfiguration();
     // Check everything is gone, reset to blank otherwise.
     for ( QStringList::Iterator it = deleteList.begin();
           it != deleteList.end(); ++it )
     {
        m_config->setGroup(*it);
        if (m_config->hasKey("UserAgent"))
           m_config->writeEntry("UserAgent", QString::null);
     }
     m_config->sync();
  }

  // Inform running io-slaves about change...
  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << QString::null;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "*", "KIO::Scheduler", "reparseSlaveConfiguration(QString)", data );
}

bool UserAgentOptions::handleDuplicate( const QString& site,
                                        const QString& identity,
                                        const QString& alias )
{
  QListViewItem* item = lv_siteUABindings->firstChild();
  while ( item != 0 )
  {
    if ( item->text(0) == site )
    {
      QString msg = i18n("<qt><center>Found an existing identification for"
                         "<br/><b>%1</b><br/>"
                         "Do you want to replace it?</center>"
                         "</qt>").arg(site);
      int res = KMessageBox::warningYesNo(this, msg,
                                          i18n("Duplicate Identification"),
                                          QString::null);
      if ( res == KMessageBox::Yes )
      {
        item->setText(0, site);
        item->setText(1, identity);
        item->setText(2, alias);
        changed();
      }
      return true;
    }
    item = item->nextSibling();
  }
  return false;
}

void UserAgentOptions::addPressed()
{
  UAProviderDlg* dlg = new UAProviderDlg( i18n("Add Identification"),
                                          this, 0L, m_provider );
  if ( dlg->exec() == QDialog::Accepted )
  {
    if ( !handleDuplicate( dlg->siteName(), dlg->identity(), dlg->alias() ) )
    {
      QListViewItem* index = new QListViewItem( lv_siteUABindings,
                                                dlg->siteName(),
                                                dlg->identity(),
                                                dlg->alias() );
      lv_siteUABindings->sort();
      lv_siteUABindings->setCurrentItem( index );
      changed();
    }
  }
  delete dlg;
}

void UserAgentOptions::changePressed()
{
  UAProviderDlg* dlg = new UAProviderDlg( i18n("Modify Identification"),
                                          this, 0L, m_provider );
  QListViewItem *index = lv_siteUABindings->currentItem();
  if(!index)
    return;
  QString old_site = index->text(0);
  dlg->setSiteName( old_site );
  dlg->setIdentity( index->text(1) );

  if ( dlg->exec() == QDialog::Accepted )
  {
    QString new_site = dlg->siteName();
    if ( new_site == old_site ||
         !handleDuplicate( dlg->siteName(), dlg->identity(), dlg->alias() ) )
    {
      index->setText( 0, new_site );
      index->setText( 1, dlg->identity() );
      index->setText( 2, dlg->alias() );
      changed();
    }
  }
  delete dlg;
}

void UserAgentOptions::deletePressed()
{
  QListViewItem* item;
  QListViewItem* nextItem = 0;

  item = lv_siteUABindings->firstChild ();

  while (item != 0L)
  {
    if (lv_siteUABindings->isSelected (item))
    {
      nextItem = item->itemBelow();
      if ( !nextItem )
        nextItem = item->itemAbove();

      delete item;
      item = nextItem;
    }
    else
    {
      item = item->itemBelow();
    }
  }

  if (nextItem)
    lv_siteUABindings->setSelected (nextItem, true);

  updateButtons();
  changed();
}

void UserAgentOptions::deleteAllPressed()
{
  lv_siteUABindings->clear();
  updateButtons();
  changed();
}

void UserAgentOptions::changeSendUAString()
{
  bool enabled = cb_sendUAString->isChecked();
  bg_default->setEnabled( enabled );
  gb_siteSpecific->setEnabled( enabled );
}

void UserAgentOptions::changeDefaultUAModifiers( int )
{
  m_ua_keys = ":"; // Make sure it's not empty

  if ( cb_showOS->isChecked() )
     m_ua_keys += 'o';

  if ( cb_showOSV->isChecked() )
     m_ua_keys += 'v';

  if ( cb_showPlatform->isChecked() )
     m_ua_keys += 'p';

  if ( cb_showMachine->isChecked() )
     m_ua_keys += 'm';

  if ( cb_showLanguage->isChecked() )
     m_ua_keys += 'l';

  cb_showOSV->setEnabled(m_ua_keys.contains('o'));

  QString modVal = KProtocolManager::defaultUserAgent( m_ua_keys );
  if ( lb_default->text() != modVal )
  {
    lb_default->setText(modVal);
    changed();
  }
}

void UserAgentOptions::changed()
{
  emit KCModule::changed(true);
}

QString UserAgentOptions::quickHelp() const
{
  return i18n( "<h1>Browser Identification</h1> "
               "The browser-identification control screen allows you to have "
               "full control over what Konqueror will report itself as to web "
               "sites."
               "<P>This ability to spoof or fake identity is necessary because "
               "some web sites do not display properly when they detect that "
               "they are not talking to current versions of Netscape Navigator "
               "or Internet Explorer, even if the \"unsupported browser\" actually "
               "supports all the necessary features to render those pages properly. "
               "Hence, for such sites, you may want to override the default "
               "identification by adding a site or domain specific entry."
               "<P><u>NOTE:</u> To obtain specific help on a particular section "
               "of the dialog box, simply click on the little <b>?</b> button on "
               "the top right corner of this window, then click on the section "
               "for which you are seeking help." );
}

void UserAgentOptions::selectionChanged ()
{
  QListViewItem* item;

  d_itemsSelected = 0;
  item = lv_siteUABindings->firstChild ();

  while (item != 0L)
  {
    if (lv_siteUABindings->isSelected (item))
      d_itemsSelected++;
    item = item->nextSibling ();
  }

  updateButtons ();

}

#include "useragentdlg.moc"
