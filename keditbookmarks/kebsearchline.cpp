/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

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

#include "kebsearchline.h"

KEBSearchLine::KEBSearchLine(QWidget *parent, KListView *listView, const char *name)
    : KListViewSearchLine(parent, listView, name)
{
    mmode = AND;
}

KEBSearchLine::KEBSearchLine(QWidget *parent, const char *name)
    :KListViewSearchLine(parent, name)
{
    mmode = AND;
}

KEBSearchLine::~KEBSearchLine()
{
}

bool KEBSearchLine::itemMatches(const QListViewItem *item, const QString &s) const
{
    if(mmode == EXACTLY)
       return KListViewSearchLine::itemMatches(item, s);

    if(lastpattern != s)
    {
       splitted = QStringList::split(QChar(' '), s);
       lastpattern = s;
    }

    QStringList::const_iterator it = splitted.begin();
    QStringList::const_iterator end = splitted.end();

    if(mmode == OR)
    {
       if(it == end) //Nothing to match
           return true;
       for( ; it != end; ++it)
           if(KListViewSearchLine::itemMatches(item, *it))
               return true;
    }
    else if(mmode == AND)
       for( ; it != end; ++it)
           if(! KListViewSearchLine::itemMatches(item, *it))
               return false;

    return (mmode == AND);
}

KEBSearchLine::modes KEBSearchLine::mode()
{
    return mmode;
}

void KEBSearchLine::setMode(modes m)
{
    mmode = m;
}
