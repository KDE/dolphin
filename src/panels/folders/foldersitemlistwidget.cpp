/***************************************************************************
 *  Copyright (C) 2012 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com>*
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

#include "foldersitemlistwidget.h"

FoldersItemListWidget::FoldersItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent) :
    KFileItemListWidget(informant, parent)
{
}

FoldersItemListWidget::~FoldersItemListWidget()
{
}

QPalette::ColorRole FoldersItemListWidget::normalTextColorRole() const
{
    return QPalette::WindowText;
}

#include "foldersitemlistwidget.moc"
