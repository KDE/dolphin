/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
// $Id$

#ifndef __konq_fileitem_h__
#define __konq_fileitem_h__

// This is just in case we need a KonqFileItem again.
// At the moment this is not the case.

#include <kfileitem.h>

#define KonqFileItem KFileItem

/**
 * List of KonqFileItems
 */
typedef QList<KonqFileItem> KonqFileItemList;

/**
 * Iterator for KonqFileItemList
 */
typedef QListIterator<KonqFileItem> KonqFileItemListIterator;

#endif
