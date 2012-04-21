/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "kstandarditem.h"

KStandardItem::KStandardItem(KStandardItem* parent) :
    m_text(),
    m_icon(),
    m_group(),
    m_parent(parent),
    m_children(),
    m_model(0)
{
}

KStandardItem::KStandardItem(const QString& text, KStandardItem* parent) :
    m_text(text),
    m_icon(),
    m_group(),
    m_parent(parent),
    m_children(),
    m_model(0)
{
}

KStandardItem::KStandardItem(const QIcon& icon, const QString& text, KStandardItem* parent) :
    m_text(text),
    m_icon(icon),
    m_group(),
    m_parent(parent),
    m_children(),
    m_model(0)
{
}

KStandardItem::~KStandardItem()
{
}

void KStandardItem::setText(const QString& text)
{
    m_text = text;
}

QString KStandardItem::text() const
{
    return m_text;
}

void KStandardItem::setIcon(const QIcon& icon)
{
    m_icon = icon;
}

QIcon KStandardItem::icon() const
{
    return m_icon;
}

void KStandardItem::setGroup(const QString& group)
{
    m_group = group;
}

QString KStandardItem::group() const
{
    return m_group;
}

void KStandardItem::setParent(KStandardItem* parent)
{
    // TODO: not implemented yet
    m_parent = parent;
}

KStandardItem* KStandardItem::parent() const
{
    return m_parent;
}

QList<KStandardItem*> KStandardItem::children() const
{
    return m_children;
}
