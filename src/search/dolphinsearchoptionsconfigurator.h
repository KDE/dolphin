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

#include <kurl.h>
#define DISABLE_NEPOMUK_LEGACY
#include <nepomuk/query.h>
#include <QList>
#include <QString>
#include <QWidget>

class KComboBox;
class SearchCriterionSelector;
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

    /**
     * Returns the sum of the configured options and the
     * custom search query as Nepomuk conform search URL. If the
     * query is invalid, an empty URL is returned.
     * @see DolphinSearchOptionsConfigurator::setCustomSearchQuery()
     */
    KUrl nepomukSearchUrl() const;

public slots:
    /**
     * Sets a custom search query that is added to the
     * search query defined by the search options configurator.
     * This is useful if a custom search user interface is
     * offered outside the search options configurator.
     */
    void setCustomSearchQuery(const QString& searchQuery);

signals:
    void searchOptionsChanged();

protected:
    virtual void showEvent(QShowEvent* event);

private slots:
    void slotAddSelectorButtonClicked();
    void removeCriterion();

    /**
     * Saves the current query by adding it as Places entry.
     */
    void saveQuery();

    /**
     * Enables the enabled property of the search-, save-button and the
     * add-selector button.
     */
    void updateButtons();

private:
    /**
     * Adds the new search description selector to the bottom
     * of the layout.
     */
    void addCriterion(SearchCriterionSelector* selector);

    /**
     * Returns the sum of the configured options and the
     * custom search query as Nepomuk confrom query.
     * @see DolphinSearchOptionsConfigurator::setCustomSearchQuery()
     */
    Nepomuk::Query::Query nepomukQuery() const;

private:
    bool m_initialized;
    KComboBox* m_locationBox;
    KComboBox* m_whatBox;
    QPushButton* m_addSelectorButton;
    QPushButton* m_searchButton;
    QPushButton* m_saveButton;
    QVBoxLayout* m_vBoxLayout;
    QList<SearchCriterionSelector*> m_criteria;
    QString m_customSearchQuery;
};

#endif
