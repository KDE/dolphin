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

#include "datesearchfilterwidget.h"

#include <klocale.h>
#include <nepomuk/comparisonterm.h>
#include <nepomuk/literalterm.h>
#include <nepomuk/orterm.h>
#include <nepomuk/property.h>
#include <nepomuk/query.h>
#include "nie.h"
#include <QDate>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

DateSearchFilterWidget::DateSearchFilterWidget(QWidget* parent) :
    AbstractSearchFilterWidget(parent),
    m_dateButtons()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(0);

    for (int i = Today; i <= ThisYear; ++i) {
        QPushButton* button = createButton();
        switch (i) {
        case Today:     button->setText(i18nc("@action:button", "Today")); break;
        case Yesterday: button->setText(i18nc("@action:button", "Yesterday")); break;
        case ThisWeek:  button->setText(i18nc("@action:button", "This Week")); break;
        case ThisMonth: button->setText(i18nc("@action:button", "This Month")); break;
        case ThisYear:  button->setText(i18nc("@action:button", "This Year")); break;
        default: Q_ASSERT(false);
        }

        layout->addWidget(button);
        m_dateButtons.append(button);
    }
    layout->addStretch(1);
}

DateSearchFilterWidget::~DateSearchFilterWidget()
{
}


QString DateSearchFilterWidget::filterLabel() const
{
    return i18nc("@title:group", "Date");
}

Nepomuk::Query::Term DateSearchFilterWidget::queryTerm() const
{
    Nepomuk::Query::OrTerm orTerm;

    int index = 0;
    foreach (const QPushButton* button, m_dateButtons) {
        if (button->isChecked()) {
            QDate today = QDate::currentDate();
            QDate date;
            switch (index) {
            case Today:
                // Current date is already set
                break;
            case Yesterday:
                date.addDays(-1);
                break;
            case ThisWeek:
                date.addDays(-today.dayOfWeek());
                break;
            case ThisMonth:
                date = QDate(today.year(), today.month(), 1);
                break;           
            case ThisYear:
                date = QDate(today.year(), 1, 1);
                break;
            default:
                Q_ASSERT(false);
            }

            const QDateTime dateTime(date);
            const Nepomuk::Query::LiteralTerm term(dateTime);
            const Nepomuk::Query::ComparisonTerm compTerm(Nepomuk::Vocabulary::NIE::lastModified(),
                                                          term,
                                                          Nepomuk::Query::ComparisonTerm::GreaterOrEqual);
            orTerm.addSubTerm(compTerm);
        }
        ++index;
    }

    return orTerm;
}

#include "datesearchfilterwidget.moc"
