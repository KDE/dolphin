/*
 *  kcmhistory.h
 *  Copyright (c) 2002 Stephan Binner <binner@kde.org>
 *
 *  based on kcmtaskbar.h
 *  Copyright (c) 2000 Kurt Granroth <granroth@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */
#ifndef __kcmhistory_h__
#define __kcmhistory_h__

#include <kcmodule.h>

class KonqHistoryManager;
class KonqSidebarHistorySettings;
class KonqSidebarHistoryDlg;

class HistorySidebarConfig : public KCModule
{
  Q_OBJECT

public:
  HistorySidebarConfig( QWidget *parent=0, const QStringList &list=QStringList() );

  void load();
  void save();
  void defaults();

  QString quickHelp() const;

private Q_SLOTS:
  void configChanged();

  void slotGetFontNewer();
  void slotGetFontOlder();

  void slotExpireChanged( int );
  void slotNewerChanged( int );
  void slotOlderChanged( int );

  void slotClearHistory();

private:
  QFont m_fontNewer;
  QFont m_fontOlder;

  KonqSidebarHistoryDlg* dialog;
  KonqSidebarHistorySettings *m_settings;
  KonqHistoryManager *mgr;
};

#endif
