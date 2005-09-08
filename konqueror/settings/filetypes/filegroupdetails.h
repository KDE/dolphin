/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

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
#ifndef _FILEGROUPDETAILS_H
#define _FILEGROUPDETAILS_H

#include <qwidget.h>
class TypesListItem;
class Q3ButtonGroup;

/**
 * This widget contains the details for a filetype group.
 * Currently this only involves the embedding configuration.
 */
class FileGroupDetails : public QWidget
{
  Q_OBJECT
public:
  FileGroupDetails(QWidget *parent = 0, const char *name = 0);

  void setTypeItem( TypesListItem * item );

signals:
  void changed(bool);

protected slots:
  void slotAutoEmbedClicked(int button);

private:
  TypesListItem * m_item;

  // Embedding config
  Q3ButtonGroup *m_autoEmbed;
};

#endif
