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

#ifndef __configwidget_h__
#define __configwidget_h__ $Id$

#include <qwidget.h>

#include "enginecfg.h"

class QListView;
class QListViewItem;
class QPushButton;
class QLineEdit;
class QListBox;

class ConfigWidget : public QWidget
{
  Q_OBJECT
public: 
  ConfigWidget();
  ~ConfigWidget();

private slots:
  void slotSelectionChanged( QListViewItem *item );
  
  void slotSaveEntry();
  void slotRemoveEntry();
  
  void slotTextChanged( const QString &text );
  
  void slotKeySelected( const QString &text );
  
  void slotKeyTextChanged( const QString &text );
  
  void slotAddKey();
  void slotRemoveKey();
  
  void slotQueryTextChanged();
  
private:
  void saveEngine( const EngineCfg::SearchEntry &e );

  QValueList<EngineCfg::SearchEntry> m_lstSearchEngines;

  QPushButton *m_pSaveEntryPushButton;
  
  QPushButton *m_pRemoveEntryPushButton;
  
  QLineEdit *m_pNameLineEdit;
  
  QLineEdit *m_pQueryLineEdit;
  
  QListBox *m_pKeyListBox;
  
  QLineEdit *m_pKeyLineEdit;
  
  QPushButton *m_pAddKeyPushButton;
  
  QPushButton *m_pRemoveKeyPushButton;
  
  QString m_strCurrentName;
  QString m_strCurrentKey;
  
  QListView *m_pListView;
};

#endif
