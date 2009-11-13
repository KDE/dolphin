/***************************************************************************
 *   Copyright (C) 2009 by Adam Kidder <thekidder@gmail.com>               *
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

#ifndef SEARCHCRITERIONSELECTOR_H
#define SEARCHCRITERIONSELECTOR_H

#include <QList>
#include <QString>
#include <QWidget>

#include <search/searchcriteriondescription.h>

class SearchCriterionValue;
class QComboBox;
class QHBoxLayout;
class QPushButton;
class ValueWidget;

/**
 * @brief Allows the user to select a search criterion.
 * The widget represents one row of the DolphinSearchOptionsConfigurator.
 * Example: [File Size] [greater than] [10] [Byte]
 *
 * @see DolphinSearchOptionsConfigurator.
 */
class SearchCriterionSelector : public QWidget
{
    Q_OBJECT

public:
    enum Type { Date, Size, Tag };

    SearchCriterionSelector(Type type, QWidget* parent = 0);
    virtual ~SearchCriterionSelector();

    /**
     * Converts the string representation of the criterion.
     * The string is conform to get added to a nepomuk:/-URI.
     */
    QString toString() const;

signals:
    /**
     * Is emitted if the criterion selector should be removed
     * because the user clicked the "Remove" button.
     */
    void removeCriterion();

    /** Is emitted if the user has changed the search criterion. */
    void criterionChanged();

private slots:
    void slotDescriptionChanged(int index);
    void slotComparatorChanged(int index);

private:
    /**
     * Creates all available search criterion descriptions m_descriptions
     * and adds them into the combobox m_descriptionsBox.
     */
    void createDescriptions();

private:
    QHBoxLayout* m_layout;
    QComboBox* m_descriptionsBox;        // has items like "File Size", "Date Modified", ...
    QComboBox* m_comparatorBox;          // has items like "greater than", "less than", ...
    SearchCriterionValue* m_valueWidget; // contains the value of a file size or a date
    QPushButton* m_removeButton;         // requests a removing of the search criterion instance

    QList<SearchCriterionDescription> m_descriptions;
};

#endif // SEARCHCRITERIONSELECTOR_H
