/*
   Original Authors:
   Copyright (c) Kalle Dalheimer <kalle@kde.org> 1997
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QToolTip>
#include <QPushButton>
//Added by qt3to4:
#include <QVBoxLayout>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <k3listview.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kio/http_slave_defaults.h>

#include "ksaveioconfig.h"
#include "fakeuaprovider.h"
#include "uagentproviderdlg.h"

#include "useragentdlg.h"
#include "useragentdlg_ui.h"
#include <kgenericfactory.h>

typedef KGenericFactory<UserAgentDlg> UserAgentDlgFactory;
K_EXPORT_COMPONENT_FACTORY(useragent, UserAgentDlgFactory("kcmkio"))

UserAgentDlg::UserAgentDlg(QWidget *parent, const QStringList &)
    : KCModule(UserAgentDlgFactory::instance(), parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(0);
  mainLayout->setSpacing(KDialog::spacingHint());

  dlg = new UserAgentDlgUI (this);
  mainLayout->addWidget(dlg);

  dlg->lvDomainPolicyList->setSorting(0);

  connect( dlg->cbSendUAString, SIGNAL(clicked()), SLOT(configChanged()) );

  connect( dlg->gbDefaultId, SIGNAL(clicked(int)),
           SLOT(changeDefaultUAModifiers(int)));

  connect( dlg->lvDomainPolicyList, SIGNAL(selectionChanged()),
           SLOT(selectionChanged()) );
  connect( dlg->lvDomainPolicyList, SIGNAL(doubleClicked (Q3ListViewItem *)),
           SLOT(changePressed()) );
  connect( dlg->lvDomainPolicyList, SIGNAL( returnPressed ( Q3ListViewItem * ) ),
           SLOT( changePressed() ));

  connect( dlg->pbNew, SIGNAL(clicked()), SLOT( addPressed() ) );
  connect( dlg->pbChange, SIGNAL( clicked() ), SLOT( changePressed() ) );
  connect( dlg->pbDelete, SIGNAL( clicked() ), SLOT( deletePressed() ) );
  connect( dlg->pbDeleteAll, SIGNAL( clicked() ), SLOT( deleteAllPressed() ) );

  load();
}

UserAgentDlg::~UserAgentDlg()
{
    delete m_provider;
    delete m_config;
}

void UserAgentDlg::load()
{
  d_itemsSelected = 0;
  dlg->lvDomainPolicyList->clear();

  m_config = new KConfig("kio_httprc", false, false);
  m_provider = new FakeUASProvider();

  QStringList list = m_config->groupList();
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
      if ( (*it) == "<default>")
         continue;
      QString domain = *it;
      m_config->setGroup(*it);
      QString agentStr = m_config->readEntry("UserAgent");
      if (!agentStr.isEmpty())
      {
         QString realName = m_provider->aliasStr(agentStr);
         new Q3ListViewItem( dlg->lvDomainPolicyList, domain.toLower(), realName, agentStr );
      }
  }

  // Update buttons and checkboxes...
  m_config->setGroup(QString());
  bool b = m_config->readEntry("SendUserAgent", QVariant(true)).toBool();
  dlg->cbSendUAString->setChecked( b );
  m_ua_keys = m_config->readEntry("UserAgentKeys", DEFAULT_USER_AGENT_KEYS).toLower();
  dlg->leDefaultId->setSqueezedText( KProtocolManager::defaultUserAgent( m_ua_keys ) );
  dlg->cbOS->setChecked( m_ua_keys.contains('o') );
  dlg->cbOSVersion->setChecked( m_ua_keys.contains('v') );
  dlg->cbOSVersion->setEnabled( m_ua_keys.contains('o') );
  dlg->cbPlatform->setChecked( m_ua_keys.contains('p') );
  dlg->cbProcessorType->setChecked( m_ua_keys.contains('m') );
  dlg->cbLanguage->setChecked( m_ua_keys.contains('l') );
  updateButtons();
  emit changed( false );
}

void UserAgentDlg::updateButtons()
{
  bool hasItems = dlg->lvDomainPolicyList->childCount() > 0;

  dlg->pbChange->setEnabled ((hasItems && d_itemsSelected == 1));
  dlg->pbDelete->setEnabled ((hasItems && d_itemsSelected > 0));
  dlg->pbDeleteAll->setEnabled ( hasItems );
}

void UserAgentDlg::defaults()
{
  dlg->lvDomainPolicyList->clear();
  m_ua_keys = DEFAULT_USER_AGENT_KEYS;
  dlg->leDefaultId->setSqueezedText( KProtocolManager::defaultUserAgent(m_ua_keys) );
  dlg->cbOS->setChecked( m_ua_keys.contains('o') );
  dlg->cbOSVersion->setChecked( m_ua_keys.contains('v') );
  dlg->cbOSVersion->setEnabled( m_ua_keys.contains('o') );
  dlg->cbPlatform->setChecked( m_ua_keys.contains('p') );
  dlg->cbProcessorType->setChecked( m_ua_keys.contains('m') );
  dlg->cbLanguage->setChecked( m_ua_keys.contains('l') );
  dlg->cbSendUAString->setChecked( true );
  updateButtons();
  configChanged();
}

void UserAgentDlg::save()
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

  Q3ListViewItem* it = dlg->lvDomainPolicyList->firstChild();
  while(it)
  {
    QString domain = it->text(0);
    if (domain[0] == '.')
      domain = domain.mid(1);
    QString userAgent = it->text(2);
    m_config->setGroup(domain);
    m_config->writeEntry("UserAgent", userAgent);
    deleteList.removeAll(domain);

    it = it->nextSibling();
  }

  m_config->setGroup(QString());
  m_config->writeEntry("SendUserAgent", dlg->cbSendUAString->isChecked());
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
           m_config->writeEntry("UserAgent", QString());
     }
     m_config->sync();
  }

  KSaveIOConfig::updateRunningIOSlaves (this);

  emit changed( false );
}

bool UserAgentDlg::handleDuplicate( const QString& site,
                                        const QString& identity,
                                        const QString& alias )
{
  Q3ListViewItem* item = dlg->lvDomainPolicyList->firstChild();
  while ( item != 0 )
  {
    if ( item->text(0) == site )
    {
      QString msg = i18n("<qt><center>Found an existing identification for"
                         "<br/><b>%1</b><br/>"
                         "Do you want to replace it?</center>"
                         "</qt>", site);
      int res = KMessageBox::warningContinueCancel(this, msg,
                                          i18n("Duplicate Identification"),
                                          i18n("Replace"));
      if ( res == KMessageBox::Continue )
      {
        item->setText(0, site);
        item->setText(1, identity);
        item->setText(2, alias);
        configChanged();
      }
      return true;
    }
    item = item->nextSibling();
  }
  return false;
}

void UserAgentDlg::addPressed()
{
  UAProviderDlg pdlg ( i18n("Add Identification"), this, m_provider );

  if ( pdlg.exec() == QDialog::Accepted )
  {
    if ( !handleDuplicate( pdlg.siteName(), pdlg.identity(), pdlg.alias() ) )
    {
      Q3ListViewItem* index = new Q3ListViewItem( dlg->lvDomainPolicyList,
                                                pdlg.siteName(),
                                                pdlg.identity(),
                                                pdlg.alias() );
      dlg->lvDomainPolicyList->sort();
      dlg->lvDomainPolicyList->setCurrentItem( index );
      configChanged();
    }
  }
}

void UserAgentDlg::changePressed()
{
  UAProviderDlg pdlg ( i18n("Modify Identification"), this, m_provider );

  Q3ListViewItem *index = dlg->lvDomainPolicyList->currentItem();

  if(!index)
    return;

  QString old_site = index->text(0);
  pdlg.setSiteName( old_site );
  pdlg.setIdentity( index->text(1) );

  if ( pdlg.exec() == QDialog::Accepted )
  {
    QString new_site = pdlg.siteName();
    if ( new_site == old_site ||
         !handleDuplicate( new_site, pdlg.identity(), pdlg.alias() ) )
    {
      index->setText( 0, new_site );
      index->setText( 1, pdlg.identity() );
      index->setText( 2, pdlg.alias() );
      configChanged();
    }
  }
}

void UserAgentDlg::deletePressed()
{
  Q3ListViewItem* item;
  Q3ListViewItem* nextItem = 0;

  item = dlg->lvDomainPolicyList->firstChild ();

  while (item != 0L)
  {
    if (dlg->lvDomainPolicyList->isSelected (item))
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
    dlg->lvDomainPolicyList->setSelected (nextItem, true);

  updateButtons();
  configChanged();
}

void UserAgentDlg::deleteAllPressed()
{
  dlg->lvDomainPolicyList->clear();
  updateButtons();
  configChanged();
}

void UserAgentDlg::configChanged()
{
  emit changed ( true );
}

void UserAgentDlg::changeDefaultUAModifiers( int )
{
  m_ua_keys = ":"; // Make sure it's not empty

  if ( dlg->cbOS->isChecked() )
     m_ua_keys += 'o';

  if ( dlg->cbOSVersion->isChecked() )
     m_ua_keys += 'v';

  if ( dlg->cbPlatform->isChecked() )
     m_ua_keys += 'p';

  if ( dlg->cbProcessorType->isChecked() )
     m_ua_keys += 'm';

  if ( dlg->cbLanguage->isChecked() )
     m_ua_keys += 'l';

  dlg->cbOSVersion->setEnabled(m_ua_keys.contains('o'));

  QString modVal = KProtocolManager::defaultUserAgent( m_ua_keys );
  if ( dlg->leDefaultId->text() != modVal )
  {
    dlg->leDefaultId->setSqueezedText(modVal);
    configChanged();
  }
}

void UserAgentDlg::selectionChanged ()
{
  Q3ListViewItem* item;

  d_itemsSelected = 0;
  item = dlg->lvDomainPolicyList->firstChild ();

  while (item != 0L)
  {
    if (dlg->lvDomainPolicyList->isSelected (item))
      d_itemsSelected++;
    item = item->nextSibling ();
  }

  updateButtons ();
}

QString UserAgentDlg::quickHelp() const
{
  return i18n( "<h1>Browser Identification</h1> "
               "The browser-identification module allows you to have full "
               "control over how Konqueror will identify itself to web "
               "sites you browse."
               "<P>This ability to fake identification is necessary because "
               "some web sites do not display properly when they detect that "
               "they are not talking to current versions of either Netscape "
               "Navigator or Internet Explorer, even if the browser actually "
               "supports all the necessary features to render those pages "
               "properly. "
               "For such sites, you can use this feature to try to browse "
               "them. Please understand that this might not always work, since "
               "such sites might be using non-standard web protocols and or "
               "specifications."
               "<P><u>NOTE:</u> To obtain specific help on a particular section "
               "of the dialog box, simply click on the quick help button on "
               "the window title bar, then click on the section "
               "for which you are seeking help." );
}

#include "useragentdlg.moc"
