/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef DOLPHINFACETSWIDGET_H
#define DOLPHINFACETSWIDGET_H

#include <QWidget>
#include <KCoreDirLister>

class QComboBox;
class QDate;
class QEvent;
class QToolButton;

/**
 * @brief Allows to filter search-queries by facets.
 *
 * TODO: The current implementation is a temporary
 * workaround for the 4.9 release and represents no
 * real facets-implementation yet: There have been
 * some Dolphin specific user-interface and interaction
 * issues since 4.6 by embedding the Nepomuk facet-widget
 * into a QDockWidget (this is unrelated to the
 * Nepomuk facet-widget itself). Now in combination
 * with the search-shortcuts in the Places Panel some
 * existing issues turned into real showstoppers.
 *
 * So the longterm plan is to use the Nepomuk facets
 * again as soon as possible.
 */
class DolphinFacetsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DolphinFacetsWidget(QWidget* parent = nullptr);
    ~DolphinFacetsWidget() override;

    void resetOptions();

    QString ratingTerm() const;
    QString facetType() const;

    bool isRatingTerm(const QString& term) const;
    void setRatingTerm(const QString& term);

    void setFacetType(const QString& type);

signals:
    void facetChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void updateTagsMenu();
    void updateTagsMenuItems(const QUrl&, const KFileItemList& items);

private:
    void setRating(const int stars);
    void setTimespan(const QDate& date);
    void addSearchTag(const QString& tag);
    void removeSearchTag(const QString& tag);

    void initComboBox(QComboBox* combo);
    void updateTagsSelector();

private:
    QComboBox* m_typeSelector;
    QComboBox* m_dateSelector;
    QComboBox* m_ratingSelector;
    QToolButton* m_tagsSelector;

    QStringList m_searchTags;
    KCoreDirLister m_tagsLister;
};

#endif
