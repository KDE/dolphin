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

/*
 * Some code is from kuserprofile from libkio,
 * (c) by Torben Weis <weis@kde.org>
 */

#include "kprofiledlg.h"

#include <qlayout.h>
#include <qvbox.h>
#include <qcombobox.h>
#include <qlistbox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qspinbox.h>

#include <ksimpleconfig.h>
#include <kservicetype.h>
#include <klocale.h>

KProfileOptions::KProfileOptions( QWidget *parent, const char *name )
: KConfigWidget( parent, name )
{
  m_lstAllServiceTypes = KServiceType::allServiceTypes();

  m_pConfig = new KSimpleConfig( "profilerc" );
  
  m_pLayoutWidget = new QVBox( this );

  m_pLayoutWidget->setMargin( 10 );
  m_pLayoutWidget->setSpacing( 10 );
  
  new QLabel( i18n( "Select Servicetype:" ), m_pLayoutWidget );
  
  m_pServiceTypesBox = new QComboBox( m_pLayoutWidget );
  
  connect( m_pServiceTypesBox, SIGNAL( activated( const QString & ) ),
           this, SLOT( slotServiceTypeSelected( const QString & ) ) );

  /*QLabel *label1 = */new QLabel( i18n( "Select Service:" ), m_pLayoutWidget );

  m_pServicesBox = new QListBox( m_pLayoutWidget );
 
  connect( m_pServicesBox, SIGNAL( highlighted( const QString & ) ),
           this, SLOT( updateProfileMap() ) );
  
  m_pActivateServiceTypeProfileCheckBox = new QCheckBox( i18n( "Activate Servicetype Profile" ),
                                                         m_pLayoutWidget );

  connect( m_pActivateServiceTypeProfileCheckBox, SIGNAL( toggled( bool ) ),
           this, SLOT( updateProfileMap() ) );

  QHBox *hBox = new QHBox( m_pLayoutWidget );
  
  /*QLabel *label2 = */ new QLabel( i18n( "Preference Value :" ), hBox );
  
  m_pPreferenceSpinBox = new QSpinBox( hBox );
  
  m_pAllowAsDefaultCheckBox = new QCheckBox( i18n( "Allow As Default" ),
                                             m_pLayoutWidget );

  connect( m_pActivateServiceTypeProfileCheckBox, SIGNAL( toggled( bool ) ),
           m_pPreferenceSpinBox, SLOT( setEnabled( bool ) ) );
  connect( m_pActivateServiceTypeProfileCheckBox, SIGNAL( toggled( bool ) ),
           m_pAllowAsDefaultCheckBox, SLOT( setEnabled( bool ) ) );

  loadSettings();
}

KProfileOptions::~KProfileOptions()
{
  delete m_pConfig;
}

void KProfileOptions::loadSettings()
{
  m_mapProfiles.clear();
  
  QStringList groupList = m_pConfig->groupList();
  QStringList::ConstIterator it = groupList.begin();
  QStringList::ConstIterator end = groupList.end();
  for (; it != end; ++it )
  {
    if ( (*it) == "<default>" )
      continue;
      
    m_pConfig->setGroup( *it );
    
    QString type = m_pConfig->readEntry( "ServiceType" );
    int pref = m_pConfig->readNumEntry( "Preference" );
    bool allow = m_pConfig->readBoolEntry( "AllowAsDefault" );
    
    if ( !type.isEmpty() && pref >= 0 )
    {
      ProfileMap::Iterator it2 = m_mapProfiles.find( type );
      if ( it2 == m_mapProfiles.end() )
      {
        ServiceMap foo;
        it2 = m_mapProfiles.insert( type, foo );
      }	
	
      it2.data().insert( *it, Service( pref, allow ) );
    }
  }
  updateGUI();
}

void KProfileOptions::saveSettings()
{
  updateProfileMap();
  
  QStringList groupList = m_pConfig->groupList();
  QStringList::ConstIterator git = groupList.begin();
  QStringList::ConstIterator gend = groupList.end();
  for (; git != gend; ++git )
    if ( (*git) != "<default>" )
      m_pConfig->deleteGroup( *git );

  ProfileMap::ConstIterator it = m_mapProfiles.begin();
  ProfileMap::ConstIterator end = m_mapProfiles.end();
  for (; it != end; ++it )
  {
    ServiceMap::ConstIterator it2 = it.data().begin();
    ServiceMap::ConstIterator end2 = it.data().end();
    for (; it2 != end2; ++it2 )
    {
      m_pConfig->setGroup( it2.key() );
      m_pConfig->writeEntry( "ServiceType", it.key() );
      m_pConfig->writeEntry( "Preference", it2.data().m_iPreference );
      m_pConfig->writeEntry( "AllowAsDefault", it2.data().m_bAllowAsDefault );
    }
  }
  
  m_pConfig->sync();
}

void KProfileOptions::applySettings()
{
  saveSettings();
}

void KProfileOptions::defaultSettings()
{
  m_mapProfiles.clear();
  updateGUI();
}

void KProfileOptions::resizeEvent( QResizeEvent * )
{
  m_pLayoutWidget->setGeometry( 0, 0, width(), height() );
}

void KProfileOptions::slotServiceTypeSelected( const QString &serviceType )
{
  m_pServicesBox->clear();
  
  KService::List offers = KServiceType::offers( serviceType );
  
  KService::List::ConstIterator it = offers.begin();
  KService::List::ConstIterator end = offers.end();
  for (; it != end; ++it )
    m_pServicesBox->insertItem( (*it)->name() );

  m_pServicesBox->setCurrentItem( 0 );

  updateProfileMap();
}

void KProfileOptions::updateGUI()
{
  m_pServiceTypesBox->clear();
  
  QValueListIterator<KServiceType::Ptr> it= m_lstAllServiceTypes.begin();
  for (; it != m_lstAllServiceTypes.end(); ++it )
  {
    KService::List tempOffers = KServiceType::offers( (*it)->name() );
    if ( tempOffers.count() > 0 ) // at least one offer ?
      m_pServiceTypesBox->insertItem( (*it)->name() ); // yes -> insert
  }
  
  if ( m_pServiceTypesBox->count() > 0 )
  {
    m_pServiceTypesBox->setCurrentItem( 0 );
    slotServiceTypeSelected( m_pServiceTypesBox->text( 0 ) );
  }    
}

void KProfileOptions::updateProfileMap()
{
  QString serviceType = m_pServiceTypesBox->text( m_pServiceTypesBox->currentItem() );
  QString serviceName = m_pServicesBox->text( m_pServicesBox->currentItem() );

  if ( !m_strLastServiceType.isEmpty() )
  {
    if ( m_pActivateServiceTypeProfileCheckBox->isChecked() )
      setService( m_strLastServiceType, m_strLastServiceName,
                  Service( m_pPreferenceSpinBox->text().toInt(),
		           m_pAllowAsDefaultCheckBox->isChecked() ) );
    else
      removeService( m_strLastServiceType, m_strLastServiceName );
  }      
  
  Service service;
  bool enable = findService( serviceType, serviceName, service );

  m_strLastServiceType = serviceType;
  m_strLastServiceName = serviceName;

  m_pActivateServiceTypeProfileCheckBox->setChecked( enable );
  m_pPreferenceSpinBox->setEnabled( enable );
  m_pAllowAsDefaultCheckBox->setEnabled( enable );
  
  if ( enable )
  {
    m_pPreferenceSpinBox->setValue( service.m_iPreference );
    m_pAllowAsDefaultCheckBox->setChecked( service.m_bAllowAsDefault );
  }
  else
  {
    m_pPreferenceSpinBox->setValue( 0 );
    m_pAllowAsDefaultCheckBox->setChecked( false );
  }
  
}

bool KProfileOptions::findService( const QString &serviceType, const QString &serviceName, Service &service )
{
  ProfileMap::ConstIterator it = m_mapProfiles.find( serviceType );
  
  if ( it == m_mapProfiles.end() )
    return false;

  ServiceMap::ConstIterator it2 = it.data().find( serviceName );
  
  if ( it2 == it.data().end() )
    return false;

  service = it2.data();
  return true;
}

void KProfileOptions::setService( const QString &serviceType, const QString &serviceName, Service service )
{
  ProfileMap::Iterator it = m_mapProfiles.find( serviceType );
  
  if ( it == m_mapProfiles.end() )
  {
    ServiceMap foo;
    it = m_mapProfiles.insert( serviceType, foo );
  }
  
  ServiceMap::Iterator it2 = it.data().find( serviceName );
  
  if ( it2 != it.data().end() )
    it.data().remove( it2 );
    
  it.data().insert( serviceName, service );
}

void KProfileOptions::removeService( const QString &serviceType, const QString &serviceName )
{
  ProfileMap::Iterator it = m_mapProfiles.find( serviceType );
  
  if ( it == m_mapProfiles.end() )
    return;
  
  ServiceMap::Iterator it2 = it.data().find( serviceName );
  
  if ( it2 == it.data().end() )
    return;
    
  it.data().remove( it2 );
}

#include "kprofiledlg.moc"
