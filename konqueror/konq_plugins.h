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

#ifndef __konq_plugins_h__
#define __konq_plugins_h__

#include <qdialog.h>
#include <qdict.h>

#include <komBase.h>
#include <ktrader.h>

class KonqEventFilterProxy: public KOMBase
{
public:
  KonqEventFilterProxy( CORBA::Object_ptr factory, const QStringList &events, KOM::Base_ptr obj );
  
  virtual void cleanUp();

  virtual CORBA::Boolean eventFilter( KOM::Base_ptr obj, const char *name, const CORBA::Any &value );
  
  virtual void disconnectFilterNotify( KOM::Base_ptr obj );
  
private:
  KOM::Base_var m_vObj;
  KOMVar<KOM::Plugin> m_rRef;
  CORBA::Object_var m_vVirtualFactoryRef;
  bool m_bShutdown;
};

class KonqPlugins
{
public:
  static void installKOMPlugins( KOM::Component_ptr comp );
  
  static void reload();
  
  static void configure( QWidget *parent = 0L );
  
private:
  static void installPlugin( KOM::Component_ptr comp, KService::Ptr pluginInfo );

  static KTrader::OfferList komPluginOffers;
  static bool bInitialized;
};

class QListView;
class QListViewItem;
class QXEmbed;
class KProcess;
class QCheckBox;
class QVBox;
class QSplitter;
class QPushButton;

class KonqPluginConfigDialog : public QDialog
{
  Q_OBJECT
public:
  KonqPluginConfigDialog( QWidget *parent = 0L );
  ~KonqPluginConfigDialog();

protected:
  virtual void accept();

private slots:
  void slotSelectionChanged( QListViewItem *item );
  void slotActivate( bool enable );

private:

  struct Entry
  {
    bool m_bActive;
    KService::Ptr m_pService;
  };

  QDict<Entry> m_mapPlugins;
  
  QSplitter *m_pSplitter;
  
  QListView *m_pListView;
  
  QCheckBox *m_pCheckBox;
  
  QVBox *m_pLayout;
  
  QXEmbed *m_pEmbed;
  
  QPushButton *m_pCloseButton;
  
  KProcess *m_pProcess;

  Entry *m_pEntry;
};

#endif
