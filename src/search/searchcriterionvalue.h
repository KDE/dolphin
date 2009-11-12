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

#ifndef SEARCHCRITERIONVALUE_H
#define SEARCHCRITERIONVALUE_H

#include <QWidget>

class QComboBox;
class QDateEdit;
class KLineEdit;

/**
 * @brief Helper class for SearchCriterionSelector.
 * Represents an input widget for the value of a search criterion.
 */
class SearchCriterionValue : public QWidget
{
    Q_OBJECT

public:
    SearchCriterionValue(QWidget* parent = 0);
    virtual ~SearchCriterionValue();

    virtual QString value() const = 0;

signals:
    void valueChanged(const QString& value);
};



/** @brief Allows to input a date value as search criterion. */
class DateValue : public SearchCriterionValue
{
    Q_OBJECT

public:
    DateValue(QWidget* parent = 0);
    virtual ~DateValue();
    virtual QString value() const;
    
private:
    QDateEdit* m_dateEdit;
};



/** @brief Allows to input a tag  as search criterion. */
class TagValue : public SearchCriterionValue
{
    Q_OBJECT

public:
    TagValue(QWidget* parent = 0);
    virtual ~TagValue();
    virtual QString value() const;

private:
    QComboBox* m_tags;
};



/** @brief Allows to input a file size value as search criterion. */
class SizeValue : public SearchCriterionValue
{
    Q_OBJECT

public:
    SizeValue(QWidget* parent = 0);
    virtual ~SizeValue();
    virtual QString value() const;

 private:
    KLineEdit* m_lineEdit;
    QComboBox* m_units;
};

#endif // SEARCHCRITERIONVALUE_H
