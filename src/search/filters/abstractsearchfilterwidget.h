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

#ifndef ABSTRACTSEARCHFILTERWIDGET_H
#define ABSTRACTSEARCHFILTERWIDGET_H

#include <nepomuk/term.h>
#include <QWidget>

class QPushButton;

/**
 * @brief Base class for widgets that act as filter for searching.
 *
 * Derived classes need to implement the methods filterLabel() and
 * queryTerm(). It is recommended to use createButton() for a filter-switch.
 * The created button will automatically emit the signal filterChanged().
 */
class AbstractSearchFilterWidget : public QWidget {
    Q_OBJECT

public:
    AbstractSearchFilterWidget(QWidget* parent = 0);
    virtual ~AbstractSearchFilterWidget();

    /**
     * @return Label that describes the kind of filter.
     */
    virtual QString filterLabel() const = 0;

    /**
     * @return Query-term for this filter, that respects the currently
     *         selected filter-switches.
     */
    virtual Nepomuk::Query::Term queryTerm() const = 0;

protected:
    /**
     * @return A checkable button, that automatically emits the signal
     *         filterChanged() when being pressed.
     */
    QPushButton* createButton();

signals:
    /**
     * Is emitted, if a filter-switch has been changed by the user.
     */
    void filterChanged();
};

#endif
