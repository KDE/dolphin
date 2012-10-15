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

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #include <Nepomuk2/Query/Term>
#endif

class QButtonGroup;
class QCheckBox;
class QRadioButton;

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
    explicit DolphinFacetsWidget(QWidget* parent = 0);
    virtual ~DolphinFacetsWidget();

#ifdef HAVE_NEPOMUK
    Nepomuk2::Query::Term facetsTerm() const;
#endif

signals:
    void facetChanged();

private:
    /**
     * @return New checkbox which is connected to the
     *         slotFacedChanged() slot whenever it has
     *         been toggled.
     */
    QCheckBox* createCheckBox(const QString& text);

    /**
     * @return New radiobutton which is connected to the
     *         slotFacedChanged() slot whenever it has
     *         been toggled.
     */
    QRadioButton* createRadioButton(const QString& text,
                                    QButtonGroup* group);

private:
    QCheckBox* m_documents;
    QCheckBox* m_images;
    QCheckBox* m_audio;
    QCheckBox* m_videos;

    QRadioButton* m_anytime;
    QRadioButton* m_today;
    QRadioButton* m_yesterday;
    QRadioButton* m_thisWeek;
    QRadioButton* m_thisMonth;
    QRadioButton* m_thisYear;

    QRadioButton* m_anyRating;
    QRadioButton* m_oneOrMore;
    QRadioButton* m_twoOrMore;
    QRadioButton* m_threeOrMore;
    QRadioButton* m_fourOrMore;
    QRadioButton* m_maxRating;
};

#endif
