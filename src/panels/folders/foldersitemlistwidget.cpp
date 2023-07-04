/*
 * SPDX-FileCopyrightText: 2012 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "foldersitemlistwidget.h"

FoldersItemListWidget::FoldersItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent)
    : KFileItemListWidget(informant, parent)
{
}

FoldersItemListWidget::~FoldersItemListWidget()
{
}

QPalette::ColorRole FoldersItemListWidget::normalTextColorRole() const
{
    return QPalette::WindowText;
}

#include "moc_foldersitemlistwidget.cpp"
