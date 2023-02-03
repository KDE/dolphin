/*
 * SPDX-FileCopyrightText: 2012 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FOLDERSITEMLISTWIDGET_H
#define FOLDERSITEMLISTWIDGET_H

#include "kitemviews/kfileitemlistwidget.h"

/**
 * @brief Extends KFileItemListWidget to use the right text color.
*/
class FoldersItemListWidget : public KFileItemListWidget
{
    Q_OBJECT

public:
    FoldersItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent);
    ~FoldersItemListWidget() override;

protected:
    QPalette::ColorRole normalTextColorRole() const override;
};

#endif
