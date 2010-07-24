/***************************************************************************
*    Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>            *
*                                                                         *
*    This program is free software; you can redistribute it and/or modify *
*    it under the terms of the GNU General Public License as published by *
*    the Free Software Foundation; either version 2 of the License, or    *
*    (at your option) any later version.                                  *
*                                                                         *
*    This program is distributed in the hope that it will be useful,      *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*    GNU General Public License for more details.                         *
*                                                                         *
*    You should have received a copy of the GNU General Public License    *
*    along with this program; if not, write to the                        *
*    Free Software Foundation, Inc.,                                      *
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA           *
* **************************************************************************/

#include "abstractsearchfilterwidget.h"

#include <QPushButton>

class SearchFilterButton : public QPushButton
{
public:
    SearchFilterButton(QWidget* parent = 0);
    virtual QSize sizeHint() const;
};

SearchFilterButton::SearchFilterButton(QWidget* parent) :
    QPushButton(parent)
{
    setCheckable(true);
}

QSize SearchFilterButton::sizeHint() const
{
    // Provide a larger preferred width, as this leads to a less
    // cluttered layout for all search filters
    const QSize defaultSize = QPushButton::sizeHint();
    QFontMetrics fontMetrics(font());
    const int minWidth = fontMetrics.height() * 8;
    const int width = qMax(minWidth, defaultSize.width());
    return QSize(width, defaultSize.height());
}



AbstractSearchFilterWidget::AbstractSearchFilterWidget(QWidget* parent) :
    QWidget(parent)
{
}

AbstractSearchFilterWidget::~AbstractSearchFilterWidget()
{
}

QPushButton* AbstractSearchFilterWidget::createButton()
{
    SearchFilterButton* button = new SearchFilterButton(this);
    connect(button, SIGNAL(toggled(bool)), this, SIGNAL(filterChanged()));
    return button;
}

#include "abstractsearchfilterwidget.moc"
