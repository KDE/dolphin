/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003 David Faure <faure@kde.org>
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KSERVICELISTWIDGET_H
#define _KSERVICELISTWIDGET_H

#include <q3groupbox.h>
#include <q3listbox.h>
class TypesListItem;
class QLineEdit;
class QPushButton;
class KService;

class KServiceListItem : public Q3ListBoxText
{
public:
    KServiceListItem(  KService *pService, int kind );
    bool isImmutable();
    QString desktopPath;
    QString localPath;
};

/**
 * This widget holds a list of services, with 5 buttons to manage it.
 * It's a separate class so that it can be used by both tabs of the
 * module, once for applications and once for services.
 * The "kind" is determined by the argument given to the constructor.
 */
class KServiceListWidget : public Q3GroupBox
{
  Q_OBJECT
public:
  enum { SERVICELIST_APPLICATIONS, SERVICELIST_SERVICES };
  KServiceListWidget(int kind, QWidget *parent = 0, const char *name = 0);

  void setTypeItem( TypesListItem * item );

signals:
  void changed(bool);

protected slots:
  void promoteService();
  void demoteService();
  void addService();
  void editService();
  void removeService();
  void enableMoveButtons(int index);

protected:
  void updatePreferredServices();

private:
  int m_kind;
  Q3ListBox *servicesLB;
  QPushButton *servUpButton, *servDownButton;
  QPushButton *servNewButton, *servEditButton, *servRemoveButton;
  TypesListItem *m_item;
};

#endif
