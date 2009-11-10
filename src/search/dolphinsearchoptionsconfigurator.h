/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef DOLPHINSEARCHOPTIONSCONFIGURATOR_H
#define DOLPHINSEARCHOPTIONSCONFIGURATOR_H

#include <QWidget>

class KComboBox;
class QPushButton;
class QVBoxLayout;

/**
 * @brief Allows the user to configure a search query for Nepomuk.
 */
class DolphinSearchOptionsConfigurator : public QWidget
{
    Q_OBJECT

public:
    DolphinSearchOptionsConfigurator(QWidget* parent = 0);
    virtual ~DolphinSearchOptionsConfigurator();

private slots:
    /**
     * Adds a new search description selector to the bottom
     * of the layout.
     */
    void addSelector();

    void removeCriterion();

    /**
     * Updates the 'enabled' property of the selector button
     * dependent from the number of existing selectors.
     */
    void updateSelectorButton();

    /**
     * Saves the current query by adding it as Places entry.
     */
    void saveQuery();

private:
    KComboBox* m_searchFromBox;
    KComboBox* m_searchWhatBox;
    QPushButton* m_addSelectorButton;
    QVBoxLayout* m_vBoxLayout;
};

#endif
