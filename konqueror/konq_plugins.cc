/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#include "konq_plugins.h"

#include <qstringlist.h>
#include <qlistview.h>
#include <qxembed.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qsplitter.h>
#include <qvbox.h>
#include <qpushbutton.h>

#include <kded_instance.h>
#include <knaming.h>
#include <kactivator.h>
#include <kservices.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kseparator.h>
#include <klocale.h>
#include <kconfig.h>
#include <kapp.h>

KTrader::OfferList KonqPlugins::komPluginOffers;
bool KonqPlugins::bInitialized = false;

KonqEventFilterProxy::KonqEventFilterProxy( CORBA::Object_ptr factory, const QStringList &events, KOM::Base_ptr obj )
{
  m_vVirtualFactoryRef = CORBA::Object::_duplicate( factory );
  
  KOM::EventTypeSeq seq;
  seq.length( events.count() );

  QStringList::ConstIterator it = events.begin();
  QStringList::ConstIterator end = events.end();
  
  CORBA::ULong i = 0;
 
  for (; it != end; ++it )
    seq[ i++ ] = CORBA::string_dup( (*it).ascii() );

  obj->installFilter( this, "eventFilter", seq, KOM::Base::FM_WRITE );
  m_vObj = KOM::Base::_duplicate( obj );
  m_bShutdown = false;
}

void KonqEventFilterProxy::cleanUp()
{
  if ( m_bIsClean )
    return;

  if ( !m_bShutdown )
    m_vObj->uninstallFilter( this );
  
  m_vObj = 0L;
  m_rRef = 0L;
  m_vVirtualFactoryRef = 0L;

  KOMBase::cleanUp();
}

CORBA::Boolean KonqEventFilterProxy::eventFilter( KOM::Base_ptr obj, const char *name, const CORBA::Any &value )
{
  if ( !CORBA::is_nil( m_rRef ) )
    return m_rRef->eventFilter( obj, name, value );
    
  KOM::PluginFactory_var factory = KOM::PluginFactory::_narrow( m_vVirtualFactoryRef );
  
  //HACK, but it's ok as every view (including the mainview) is a KOM::Component (Simon)
  KOM::Component_var comp = KOM::Component::_narrow( obj );
  
  m_rRef = factory->create( comp );
  return m_rRef->eventFilter( obj, name, value );
}

void KonqEventFilterProxy::disconnectFilterNotify( KOM::Base_ptr obj )
{
  KOMBase::disconnectFilterNotify( obj );
  m_bShutdown = true;
  destroy();
}

void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )
{
  kdebug(0, 1202, "void KonqPlugins::installKOMPlugins( KOM::Component_ptr comp )" );

  if ( !bInitialized )
    reload();

  KTrader::OfferList::ConstIterator it = komPluginOffers.begin();
  KTrader::OfferList::ConstIterator end = komPluginOffers.end();
  for (; it != end; ++it )
    installPlugin( comp, *it );
}

void KonqPlugins::reload()
{
  komPluginOffers = KdedInstance::self()->ktrader()->query( "Konqueror/EventFilter" );

  KConfig *config = kapp->getConfig();
  config->setGroup( "Plugins" );
  QStringList lstEnabledPlugins = config->readListEntry( "EnabledPlugins" );

  //remove all invalid offers
  KTrader::OfferList::Iterator it = komPluginOffers.begin();
  KTrader::OfferList::Iterator end = komPluginOffers.end();
  while ( it != end )
  {
    KService::PropertyPtr requiredIfProp = (*it)->property( "RequiredInterfaces" );
    KService::PropertyPtr eventListProp = (*it)->property( "Events" );
    
    if ( !requiredIfProp || !eventListProp || !lstEnabledPlugins.contains( (*it)->name() ) )
    {
      komPluginOffers.remove( it );
      it = komPluginOffers.begin();
      continue;
    }
    
    ++it;
  }

  bInitialized = true;
}

void KonqPlugins::configure( QWidget *parent )
{
  KonqPluginConfigDialog *dia = new KonqPluginConfigDialog( parent );

  if ( dia->exec() == QDialog::Accepted )
    bInitialized = false;
}

void KonqPlugins::installPlugin( KOM::Component_ptr comp, KService::Ptr pluginInfo )
{
  QStringList requiredInterfaces = pluginInfo->property( "RequiredInterfaces" )->stringListValue();
  
  QStringList::ConstIterator it = requiredInterfaces.begin();
  QStringList::ConstIterator end = requiredInterfaces.end();
  for (; it != end; ++it )
    if ( !comp->supportsInterface( (*it).ascii() ) )
    {
      kdebug(0, 1202, "component does not support %s", (*it).ascii() );
      return;
    }

  QString repoId = *( pluginInfo->repoIds().begin() );
  QString tag = pluginInfo->name();
  int tagPos = repoId.findRev( '#' );
  if ( tagPos != -1 )
  {
    tag = repoId.mid( tagPos+1 );
    repoId.truncate( tagPos );
  }
	
  CORBA::Object_var obj = KdedInstance::self()->kactivator()->activateService( pluginInfo->name(), repoId, tag );
	
  if ( CORBA::is_nil( obj ) )
  {
    kdebug(0, 1202, "component is not loaded!" );
    return;
  }
  
  (void)new KonqEventFilterProxy( obj, pluginInfo->property( "Events" )->stringListValue(), comp );
}

KonqPluginConfigDialog::KonqPluginConfigDialog( QWidget *parent )
: QDialog( parent, 0L, true )
{
  m_pProcess = 0L;

  QVBoxLayout *m_pTopLayout = new QVBoxLayout( this );

  m_pSplitter = new QSplitter( this );
  m_pSplitter->setOpaqueResize();

  m_pTopLayout->addWidget( m_pSplitter );

  m_pListView = new QListView( m_pSplitter );
  m_pListView->addColumn( i18n( "Name" ) );
  
  m_pLayout = new QVBox( m_pSplitter );
  
  m_pCheckBox = new QCheckBox( i18n( "Enable Plugin" ), m_pLayout );
  
  connect( m_pCheckBox, SIGNAL( toggled( bool ) ),
           this, SLOT( slotActivate( bool ) ) );

  (void)new KSeparator( m_pLayout );

  m_pCheckBox->setFixedHeight( m_pCheckBox->sizeHint().height() );
  m_pCheckBox->setEnabled( false );

  m_pEmbed = new QXEmbed( m_pLayout );

  m_mapPlugins.setAutoDelete( true );

  KConfig *config = kapp->getConfig();
  config->setGroup( "Plugins" );

  QStringList lstActivePlugins = config->readListEntry( "EnabledPlugins" );

  KTrader::OfferList offers = KdedInstance::self()->ktrader()->query( "Konqueror/EventFilter" );
  KTrader::OfferList::ConstIterator it = offers.begin();
  KTrader::OfferList::ConstIterator end = offers.end();
  for (; it != end; ++it )
  {
    Entry *e = new Entry;
    e->m_pService = *it;
    e->m_bActive = lstActivePlugins.contains( (*it)->name() );
    m_mapPlugins.insert( (*it)->name(), e );
    (void)new QListViewItem( m_pListView, (*it)->name() );
  }    
  
  m_pEntry = 0L;
  
  connect( m_pListView, SIGNAL( selectionChanged( QListViewItem * ) ),
           this, SLOT( slotSelectionChanged( QListViewItem * ) ) );
  connect( m_pListView, SIGNAL( returnPressed( QListViewItem * ) ),
           this, SLOT( slotSelectionChanged( QListViewItem * ) ) );

  m_pCloseButton = new QPushButton( i18n( "Close" ), this );
  m_pCloseButton->setFixedSize( m_pCloseButton->sizeHint() );

//  setMinimumWidth( m_pCloseButton->width() );
  
  m_pTopLayout->addWidget( m_pCloseButton );
  
  connect( m_pCloseButton, SIGNAL( clicked() ),
           this, SLOT( accept() ) );

  resize( m_pSplitter->sizeHint().width(), m_pSplitter->sizeHint().height() + m_pCloseButton->height() );
}

KonqPluginConfigDialog::~KonqPluginConfigDialog()
{
  if ( m_pProcess )
    delete m_pProcess;
}

void KonqPluginConfigDialog::accept()
{
  KConfig *config = kapp->getConfig();

  config->setGroup( "Plugins" );

  QDictIterator<Entry> it( m_mapPlugins );
  
  QStringList lstEnabledPlugins;
  
  for (; it.current(); ++it )
    if ( it.current()->m_bActive )
      lstEnabledPlugins.append( it.current()->m_pService->name() );
    
  config->writeEntry( "EnabledPlugins", lstEnabledPlugins );
  config->sync();

  QDialog::accept();
}

void KonqPluginConfigDialog::slotSelectionChanged( QListViewItem *item )
{
  Entry *entry = m_mapPlugins.find( item->text( 0 ) );

  if ( entry != m_pEntry && m_pProcess )
  {
    delete m_pProcess;
    m_pProcess = 0L;
  }
 
  if ( entry )
  {
    m_pEntry = entry;
    
    m_pCheckBox->setChecked( m_pEntry->m_bActive );
    
    if ( m_pEntry->m_bActive )
    {
      m_pProcess = new KProcess;
      (*m_pProcess) << m_pEntry->m_pService->exec();
      (*m_pProcess) << QString::fromLatin1( "-embed" ) << QString::number( m_pEmbed->winId() );
      m_pProcess->start();
    }
    
    m_pCheckBox->setEnabled( true );
  }
  else
    m_pCheckBox->setEnabled( false );
}

void KonqPluginConfigDialog::slotActivate( bool enable )
{
  m_pEntry->m_bActive = enable;
  
  if ( m_pProcess )
  {
    delete m_pProcess;
    m_pProcess = 0L;
  }
  
  if ( enable )
  {
    m_pProcess = new KProcess;
    (*m_pProcess) << m_pEntry->m_pService->exec();
    (*m_pProcess) << QString::fromLatin1( "-embed" ) << QString::number( m_pEmbed->winId() );
    m_pProcess->start();
  }
}

#include "konq_plugins.moc"
