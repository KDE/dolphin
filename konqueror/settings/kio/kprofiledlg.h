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
#ifndef __kprofiledlg_h__
#define __kprofiledlg_h__ $Id$

#include <qstring.h>
#include <qmap.h>

#include <kcontrol.h>
#include <ktrader.h>

class KSimpleConfig;
class KServiceType;
class QComboBox;
class QListBox;
class QVBox;
class QCheckBox;
class QSpinBox;

class KProfileOptions : public KConfigWidget
{
  Q_OBJECT
public:
  KProfileOptions( int &argc, char **argv, QWidget *parent = 0L, const char *name = 0L );
  ~KProfileOptions();
  
  virtual void loadSettings();
  virtual void saveSettings();
  virtual void applySettings();
  virtual void defaultSettings();

protected:
  virtual void resizeEvent( QResizeEvent * );
  
protected slots:
  void slotServiceTypeSelected( const QString &serviceType );
  void updateProfileMap();

private:
  void updateGUI();
  
  KTrader::OfferList serviceOffers( const QString &serviceType );

  struct Service
  {
    Service() {}
    Service( int pref, bool allow )
    {
      m_iPreference = pref;
      m_bAllowAsDefault = allow;
    }
    int m_iPreference;
    bool m_bAllowAsDefault;
  };

  bool findService( const QString &serviceType, const QString &serviceName, Service &service );
  void setService( const QString &serviceType, const QString &serviceName, Service service );
  void removeService( const QString &serviceType, const QString &serviceName );
  
  typedef QMap<QString,Service> ServiceMap;
  
  typedef QMap<QString,ServiceMap> ProfileMap;
  
  ProfileMap m_mapProfiles;

  QComboBox *m_pServiceTypesBox;
  QListBox *m_pServicesBox;

  KSimpleConfig *m_pConfig;

  QVBox *m_pLayoutWidget;
  
  QCheckBox *m_pActivateServiceTypeProfileCheckBox;
  
  QSpinBox *m_pPreferenceSpinBox;
  QCheckBox *m_pAllowAsDefaultCheckBox;
  
  QString m_strLastServiceType;
  QString m_strLastServiceName;
  
  KTrader::OfferList m_lstAllServices;
  QList<KServiceType> m_lstAllServiceTypes;
};

#endif
