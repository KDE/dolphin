/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef KITEMCATEGORIZER_H
#define KITEMCATEGORIZER_H

#include <libdolphin_export.h>

class QString;
class QPainter;
class QModelIndex;
class QStyleOption;

/**
  * @short Class for item categorizing on KListView
  *
  * This class is meant to be used with KListView class. Its purpose is
  * to decide to which category belongs a given index with the given role.
  * Additionally it will let you to customize the way categories are drawn,
  * only in the case that you want to do so
  *
  * @see KListView
  *
  * @author Rafael Fernández López <ereslibre@gmail.com>
  */
class LIBDOLPHINPRIVATE_EXPORT KItemCategorizer
{
public:
    KItemCategorizer();

    virtual ~KItemCategorizer();

    /**
      * This method will return the category where @param index fit on with the
      * given @param sortRole role
      */
    virtual QString categoryForItem(const QModelIndex &index,
                                    int sortRole) const = 0;

    /**
      * This method purpose is to draw a category represented by the given
      * @param index with the given @param sortRole sorting role
      *
      * @note This method will be called one time per category, always with the
      *       first element in that category
      */
    virtual void drawCategory(const QModelIndex &index,
                              int sortRole,
                              const QStyleOption &option,
                              QPainter *painter) const;

    virtual int categoryHeight(const QStyleOption &option) const;
};

#endif // KITEMCATEGORIZER_H
