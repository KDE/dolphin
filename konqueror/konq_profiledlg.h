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

#ifndef __konq_profiledlg_h__
#define __konq_profiledlg_h__ $Id$

#include <kdialog.h>
#include <qmap.h>

class KonqViewManager;
class QListBox;
class QGridLayout;
class QCheckBox;
class QLineEdit;
class QPushButton;

class KonqProfileDlg : public KDialog
{
  Q_OBJECT
public:
  KonqProfileDlg( KonqViewManager *manager, QWidget *parent = 0L );
  ~KonqProfileDlg();

protected slots:
  void slotEnableSave( const QString &text );
  void slotSave();
  void slotDelete();

private:
  KonqViewManager *m_pViewManager;

  QMap<QString,QString> m_mapEntries;

  QGridLayout *m_pGrid;

  QLineEdit *m_pProfileNameLineEdit;

  QPushButton *m_pDeleteProfileButton;
  QPushButton *m_pSaveButton;
  QPushButton *m_pCloseButton;
  QCheckBox *m_cbSaveURLs;

  QListBox *m_pListBox;
};

#endif
