/***************************************************************************
*    Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>            *
*    Copyright (C) 2019 by Ismael Asensio <isma.af@mgmail.com>            *
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

#include "dolphinfacetswidget.h"

#include <KLocalizedString>

#include <QComboBox>
#include <QDate>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>

DolphinFacetsWidget::DolphinFacetsWidget(QWidget* parent) :
    QWidget(parent),
    m_typeSelector(nullptr),
    m_dateSelector(nullptr),
    m_ratingSelector(nullptr)
{
    m_typeSelector = new QComboBox(this);
    m_typeSelector->addItem(QIcon::fromTheme(QStringLiteral("none")), i18nc("@item:inlistbox", "Any Type"), QString());
    m_typeSelector->addItem(QIcon::fromTheme(QStringLiteral("inode-directory")), i18nc("@item:inlistbox", "Folders") , QStringLiteral("Folder"));
    m_typeSelector->addItem(QIcon::fromTheme(QStringLiteral("text-x-generic")), i18nc("@item:inlistbox", "Documents") , QStringLiteral("Document"));
    m_typeSelector->addItem(QIcon::fromTheme(QStringLiteral("image-x-generic")), i18nc("@item:inlistbox", "Images") , QStringLiteral("Image"));
    m_typeSelector->addItem(QIcon::fromTheme(QStringLiteral("audio-x-generic")), i18nc("@item:inlistbox", "Audio Files"), QStringLiteral("Audio"));
    m_typeSelector->addItem(QIcon::fromTheme(QStringLiteral("video-x-generic")), i18nc("@item:inlistbox", "Videos") , QStringLiteral("Video"));
    initComboBox(m_typeSelector);

    const QDate currentDate = QDate::currentDate();

    m_dateSelector = new QComboBox(this);
    m_dateSelector->addItem(QIcon::fromTheme(QStringLiteral("view-calendar")), i18nc("@item:inlistbox", "Any Date"), QDate());
    m_dateSelector->addItem(QIcon::fromTheme(QStringLiteral("go-jump-today")), i18nc("@item:inlistbox", "Today") , currentDate);
    m_dateSelector->addItem(QIcon::fromTheme(QStringLiteral("go-jump-today")), i18nc("@item:inlistbox", "Yesterday") , currentDate.addDays(-1));
    m_dateSelector->addItem(QIcon::fromTheme(QStringLiteral("view-calendar-week")), i18nc("@item:inlistbox", "This Week") , currentDate.addDays(1 - currentDate.dayOfWeek()));
    m_dateSelector->addItem(QIcon::fromTheme(QStringLiteral("view-calendar-month")), i18nc("@item:inlistbox", "This Month"), currentDate.addDays(1 - currentDate.day()));
    m_dateSelector->addItem(QIcon::fromTheme(QStringLiteral("view-calendar-year")), i18nc("@item:inlistbox", "This Year") , currentDate.addDays(1 - currentDate.dayOfYear()));
    initComboBox(m_dateSelector);

    m_ratingSelector = new QComboBox(this);
    m_ratingSelector->addItem(QIcon::fromTheme(QStringLiteral("non-starred-symbolic")), i18nc("@item:inlistbox", "Any Rating"), 0);
    m_ratingSelector->addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "1 or more"), 1);
    m_ratingSelector->addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "2 or more"), 2);
    m_ratingSelector->addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "3 or more"), 3);
    m_ratingSelector->addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "4 or more"), 4);
    m_ratingSelector->addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "Highest Rating"), 5);
    initComboBox(m_ratingSelector);

    QHBoxLayout* topLayout = new QHBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(m_typeSelector);
    topLayout->addWidget(m_dateSelector);
    topLayout->addWidget(m_ratingSelector);

    resetOptions();
}

DolphinFacetsWidget::~DolphinFacetsWidget()
{
}

void DolphinFacetsWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange && !isEnabled()) {
        resetOptions();
    }
}

void DolphinFacetsWidget::resetOptions()
{
    m_typeSelector->setCurrentIndex(0);
    m_dateSelector->setCurrentIndex(0);
    m_ratingSelector->setCurrentIndex(0);
}

QString DolphinFacetsWidget::ratingTerm() const
{
    QStringList terms;

    if (m_ratingSelector->currentIndex() > 0) {
        const int rating = m_ratingSelector->currentData().toInt() * 2;
        terms << QStringLiteral("rating>=%1").arg(rating);
    }

    if (m_dateSelector->currentIndex() > 0) {
        const QDate date = m_dateSelector->currentData().toDate();
        terms << QStringLiteral("modified>=%1").arg(date.toString(Qt::ISODate));
    }

    return terms.join(QLatin1String(" AND "));
}

QString DolphinFacetsWidget::facetType() const
{
    return m_typeSelector->currentData().toString();
}

bool DolphinFacetsWidget::isRatingTerm(const QString& term) const
{
    const QStringList subTerms = term.split(' ', QString::SkipEmptyParts);

    // If term has sub terms, then sone of the sub terms are always "rating" and "modified" terms.
    bool containsRating = false;
    bool containsModified = false;

    foreach (const QString& subTerm, subTerms) {
        if (subTerm.startsWith(QLatin1String("rating>="))) {
            containsRating = true;
        } else if (subTerm.startsWith(QLatin1String("modified>="))) {
            containsModified = true;
        }
    }

    return containsModified || containsRating;
}

void DolphinFacetsWidget::setRatingTerm(const QString& term)
{
    // If term has sub terms, then the sub terms are always "rating" and "modified" terms.
    // If term has no sub terms, then the term itself is either a "rating" term or a "modified"
    // term. To avoid code duplication we add term to subTerms list, if the list is empty.
    QStringList subTerms = term.split(' ', QString::SkipEmptyParts);

    foreach (const QString& subTerm, subTerms) {
        if (subTerm.startsWith(QLatin1String("modified>="))) {
            const QString value = subTerm.mid(10);
            const QDate date = QDate::fromString(value, Qt::ISODate);
            setTimespan(date);
        } else if (subTerm.startsWith(QLatin1String("rating>="))) {
            const QString value = subTerm.mid(8);
            const int stars = value.toInt() / 2;
            setRating(stars);
        }
    }
}

void DolphinFacetsWidget::setFacetType(const QString& type)
{
    for (int index = 0; index <= m_typeSelector->count(); index++) {
        if (type == m_typeSelector->itemData(index).toString()) {
            m_typeSelector->setCurrentIndex(index);
            break;
        }
    }
}

void DolphinFacetsWidget::setRating(const int stars)
{
    if (stars < 0 || stars > 5) {
        return;
    }
    m_ratingSelector->setCurrentIndex(stars);
}

void DolphinFacetsWidget::setTimespan(const QDate& date)
{
    if (!date.isValid()) {
        return;
    }
    m_dateSelector->setCurrentIndex(0);
    for (int index = 1; index <= m_dateSelector->count(); index++) {
        if (date >= m_dateSelector->itemData(index).toDate()) {
            m_dateSelector->setCurrentIndex(index);
            break;
        }
    }
}

void DolphinFacetsWidget::initComboBox(QComboBox* combo)
{
    combo->setFrame(false);
    combo->setMinimumHeight(parentWidget()->height());
    combo->setCurrentIndex(0);
    connect(combo, QOverload<int>::of(&QComboBox::activated), this, &DolphinFacetsWidget::facetChanged);
}

