/***************************************************************************
*    Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>            *
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

#include <KLocale>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDate>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#ifdef HAVE_NEPOMUK
    #include <Nepomuk/Query/AndTerm>
    #include <Nepomuk/Query/ComparisonTerm>
    #include <Nepomuk/Query/LiteralTerm>
    #include <Nepomuk/Query/OrTerm>
    #include <Nepomuk/Query/Query>
    #include <Nepomuk/Query/ResourceTypeTerm>
    #include <Nepomuk/Vocabulary/NFO>
    #include <Nepomuk/Vocabulary/NIE>
    #include <Soprano/Vocabulary/NAO>
#endif

DolphinFacetsWidget::DolphinFacetsWidget(QWidget* parent) :
    QWidget(parent),
    m_documents(0),
    m_images(0),
    m_audio(0),
    m_videos(0),
    m_anytime(0),
    m_today(0),
    m_yesterday(0),
    m_thisWeek(0),
    m_thisMonth(0),
    m_thisYear(0),
    m_anyRating(0),
    m_oneOrMore(0),
    m_twoOrMore(0),
    m_threeOrMore(0),
    m_fourOrMore(0),
    m_maxRating(0)
{
    m_documents = createCheckBox(i18nc("@option:check", "Documents"));
    m_images    = createCheckBox(i18nc("@option:check", "Images"));
    m_audio     = createCheckBox(i18nc("@option:check", "Audio Files"));
    m_videos    = createCheckBox(i18nc("@option:check", "Videos"));

    QVBoxLayout* typeLayout = new QVBoxLayout();
    typeLayout->setSpacing(0);
    typeLayout->addWidget(m_documents);
    typeLayout->addWidget(m_images);
    typeLayout->addWidget(m_audio);
    typeLayout->addWidget(m_videos);
    typeLayout->addStretch();

    QButtonGroup* timespanGroup = new QButtonGroup(this);
    m_anytime   = createRadioButton(i18nc("@option:option", "Anytime"), timespanGroup);
    m_today     = createRadioButton(i18nc("@option:option", "Today"), timespanGroup);
    m_yesterday = createRadioButton(i18nc("@option:option", "Yesterday"), timespanGroup);
    m_thisWeek  = createRadioButton(i18nc("@option:option", "This Week"), timespanGroup);
    m_thisMonth = createRadioButton(i18nc("@option:option", "This Month"), timespanGroup);
    m_thisYear  = createRadioButton(i18nc("@option:option", "This Year"), timespanGroup);

    QVBoxLayout* timespanLayout = new QVBoxLayout();
    timespanLayout->setSpacing(0);
    timespanLayout->addWidget(m_anytime);
    timespanLayout->addWidget(m_today);
    timespanLayout->addWidget(m_yesterday);
    timespanLayout->addWidget(m_thisWeek);
    timespanLayout->addWidget(m_thisMonth);
    timespanLayout->addWidget(m_thisYear);
    timespanLayout->addStretch();

    QButtonGroup* ratingGroup = new QButtonGroup(this);
    m_anyRating   = createRadioButton(i18nc("@option:option", "Any Rating"), ratingGroup);
    m_oneOrMore   = createRadioButton(i18nc("@option:option", "1 or more"), ratingGroup);
    m_twoOrMore   = createRadioButton(i18nc("@option:option", "2 or more"), ratingGroup);
    m_threeOrMore = createRadioButton(i18nc("@option:option", "3 or more"), ratingGroup);
    m_fourOrMore  = createRadioButton(i18nc("@option:option", "4 or more"), ratingGroup);
    m_maxRating   = createRadioButton(i18nc("@option:option", "Highest Rating"), ratingGroup);

    QVBoxLayout* ratingLayout = new QVBoxLayout();
    ratingLayout->setSpacing(0);
    ratingLayout->addWidget(m_anyRating);
    ratingLayout->addWidget(m_oneOrMore);
    ratingLayout->addWidget(m_twoOrMore);
    ratingLayout->addWidget(m_threeOrMore);
    ratingLayout->addWidget(m_fourOrMore);
    ratingLayout->addWidget(m_maxRating);

    QHBoxLayout* topLayout = new QHBoxLayout(this);
    topLayout->addLayout(typeLayout);
    topLayout->addLayout(timespanLayout);
    topLayout->addLayout(ratingLayout);
    topLayout->addStretch();

    m_anytime->setChecked(true);
    m_anyRating->setChecked(true);
}

DolphinFacetsWidget::~DolphinFacetsWidget()
{
}

#ifdef HAVE_NEPOMUK
Nepomuk::Query::Term DolphinFacetsWidget::facetsTerm() const
{
    Nepomuk::Query::AndTerm andTerm;

    const bool hasTypeFilter = m_documents->isChecked() ||
                               m_images->isChecked() ||
                               m_audio->isChecked() ||
                               m_videos->isChecked();
    if (hasTypeFilter) {
        Nepomuk::Query::OrTerm orTerm;

        if (m_documents->isChecked()) {
            orTerm.addSubTerm(Nepomuk::Query::ResourceTypeTerm(Nepomuk::Vocabulary::NFO::Document()));
        }

        if (m_images->isChecked()) {
            orTerm.addSubTerm(Nepomuk::Query::ResourceTypeTerm(Nepomuk::Vocabulary::NFO::Image()));
        }

        if (m_audio->isChecked()) {
            orTerm.addSubTerm(Nepomuk::Query::ComparisonTerm(Nepomuk::Vocabulary::NIE::mimeType(),
                                                             Nepomuk::Query::LiteralTerm("audio")));
        }

        if (m_videos->isChecked()) {
            orTerm.addSubTerm(Nepomuk::Query::ComparisonTerm(Nepomuk::Vocabulary::NIE::mimeType(),
                                                             Nepomuk::Query::LiteralTerm("video")));
        }

        andTerm.addSubTerm(orTerm);
    }

    if (!m_anyRating->isChecked()) {
        int stars = 1; // represents m_oneOrMore
        if (m_twoOrMore->isChecked()) {
            stars = 2;
        } else if (m_threeOrMore->isChecked()) {
            stars = 3;
        } else if (m_fourOrMore->isChecked()) {
            stars = 4;
        } else if (m_maxRating->isChecked()) {
            stars = 5;
        }

        const int rating = stars * 2;
        Nepomuk::Query::ComparisonTerm term(Soprano::Vocabulary::NAO::numericRating(),
                                            Nepomuk::Query::LiteralTerm(rating),
                                            Nepomuk::Query::ComparisonTerm::GreaterOrEqual);
        andTerm.addSubTerm(term);
    }

    if (!m_anytime->isChecked()) {
        QDate date = QDate::currentDate(); // represents m_today
        if (m_yesterday->isChecked()) {
            date.addDays(-1);
        } else if (m_thisWeek->isChecked()) {
            date.addDays(1 - date.dayOfWeek());
        } else if (m_thisMonth->isChecked()) {
            date.addDays(1 - date.day());
        } else if (m_thisYear->isChecked()) {
            date.addDays(1 - date.dayOfYear());
        }

        Nepomuk::Query::ComparisonTerm term(Nepomuk::Vocabulary::NIE::lastModified(),
                                            Nepomuk::Query::LiteralTerm(QDateTime(date)),
                                            Nepomuk::Query::ComparisonTerm::GreaterOrEqual);
        andTerm.addSubTerm(term);
    }

    return andTerm;
}
#endif

QCheckBox* DolphinFacetsWidget::createCheckBox(const QString& text)
{
    QCheckBox* checkBox = new QCheckBox(text);
    connect(checkBox, SIGNAL(clicked()), this, SIGNAL(facetChanged()));
    return checkBox;
}

QRadioButton* DolphinFacetsWidget::createRadioButton(const QString& text,
                                                     QButtonGroup* group)
{
    QRadioButton* button = new QRadioButton(text);
    connect(button, SIGNAL(clicked()), this, SIGNAL(facetChanged()));
    group->addButton(button);
    return button;
}

#include "dolphinfacetswidget.moc"
