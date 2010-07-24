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

#include "ratingsearchfilterwidget.h"

#define DISABLE_NEPOMUK_LEGACY

#include <klocale.h>
#include <nepomuk/comparisonterm.h>
#include <nepomuk/literalterm.h>
#include <nepomuk/orterm.h>
#include <nepomuk/kratingpainter.h>
#include <nepomuk/property.h>
#include <nepomuk/query.h>
#include "nie.h"
#include <Soprano/LiteralValue>
#include <Soprano/Vocabulary/NAO>
#include <QFontMetrics>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QHBoxLayout>

namespace {
    // Only show the ratings 0, 2, 4, ... 10
    const int RatingInc = 2;
};

RatingSearchFilterWidget::RatingSearchFilterWidget(QWidget* parent) :
    AbstractSearchFilterWidget(parent),
    m_ratingButtons()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(0);

    QFontMetrics fontMetrics(font());
    const int iconHeight = fontMetrics.height();

    KRatingPainter ratingPainter;
    const int maxRating = ratingPainter.maxRating();
    const QSize iconSize(iconHeight * (maxRating / 2), iconHeight);
    const QRect paintRect(QPoint(0, 0), iconSize);

    for (int rating = 0; rating <= ratingPainter.maxRating(); rating += RatingInc) {
        // Create pixmap that represents the rating
        QPixmap pixmap(iconSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        ratingPainter.paint(&painter, paintRect, rating);

        // Create button with the rating pixmap as icon
        QPushButton* button = createButton();
        button->setIconSize(iconSize);
        button->setIcon(QIcon(pixmap));

        layout->addWidget(button);
        m_ratingButtons.append(button);
    }

    layout->addStretch(1);
}

RatingSearchFilterWidget::~RatingSearchFilterWidget()
{
}

QString RatingSearchFilterWidget::filterLabel() const
{
    return i18nc("@title:group", "Rating");
}

Nepomuk::Query::Term RatingSearchFilterWidget::queryTerm() const
{
    Nepomuk::Query::OrTerm orTerm;

    int rating = 0;
    foreach (const QPushButton* ratingButton, m_ratingButtons) {
        if (ratingButton->isChecked()) {
            const Nepomuk::Query::LiteralTerm term(rating);
            const Nepomuk::Query::ComparisonTerm compTerm(Soprano::Vocabulary::NAO::numericRating(),
                                                          term,
                                                          Nepomuk::Query::ComparisonTerm::Equal);
            orTerm.addSubTerm(compTerm);
        }
        rating += RatingInc;
    }

    return orTerm;
}

#include "ratingsearchfilterwidget.moc"
