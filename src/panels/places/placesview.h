/*
 * SPDX-FileCopyrightText: 2012 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLACESVIEW_H
#define PLACESVIEW_H

#include "kitemviews/kstandarditemlistview.h"

/**
 * @brief View class for the Places Panel.
 *
 * Reads the icon size from GeneralSettings::placesPanelIconSize().
 */
class PlacesView : public KStandardItemListView
{
    Q_OBJECT

public:
    explicit PlacesView(QGraphicsWidget* parent = nullptr);

    void setIconSize(int size);
    int iconSize() const;

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
};

#endif
