/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef TAGSEARCHFILTERWIDGET_H
#define TAGSEARCHFILTERWIDGET_H

#include <search/filters/abstractsearchfilterwidget.h>
#include <QList>

class QPushButton;

/**
 * @brief Allows to filter the search by tags.
 */
class TagSearchFilterWidget : public AbstractSearchFilterWidget {
    Q_OBJECT

public:
    TagSearchFilterWidget(QWidget* parent = 0);
    virtual ~TagSearchFilterWidget();
    virtual QString filterLabel() const;
    virtual Nepomuk::Query::Term queryTerm() const;

private:
    QList<QPushButton*> m_tagButtons;
};

#endif
