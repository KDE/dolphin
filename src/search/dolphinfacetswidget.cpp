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
#include <QCheckBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

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
    m_documents = new QCheckBox(i18nc("@option:check", "Documents"));
    m_images = new QCheckBox(i18nc("@option:check", "Images"));
    m_audio = new QCheckBox(i18nc("@option:check", "Audio"));
    m_videos = new QCheckBox(i18nc("@option:check", "Videos"));

    QVBoxLayout* typeLayout = new QVBoxLayout();
    typeLayout->setSpacing(0);
    typeLayout->addWidget(m_documents);
    typeLayout->addWidget(m_images);
    typeLayout->addWidget(m_audio);
    typeLayout->addWidget(m_videos);
    typeLayout->addStretch();

    m_anytime = new QRadioButton(i18nc("@option:option", "Anytime"));
    m_today = new QRadioButton(i18nc("@option:option", "Today"));
    m_yesterday = new QRadioButton(i18nc("@option:option", "Yesterday"));
    m_thisWeek = new QRadioButton(i18nc("@option:option", "This Week"));
    m_thisMonth = new QRadioButton(i18nc("@option:option", "This Month"));
    m_thisYear = new QRadioButton(i18nc("@option:option", "This Year"));

    QVBoxLayout* timespanLayout = new QVBoxLayout();
    timespanLayout->setSpacing(0);
    timespanLayout->addWidget(m_anytime);
    timespanLayout->addWidget(m_today);
    timespanLayout->addWidget(m_yesterday);
    timespanLayout->addWidget(m_thisWeek);
    timespanLayout->addWidget(m_thisMonth);
    timespanLayout->addWidget(m_thisYear);
    timespanLayout->addStretch();

    m_anyRating = new QRadioButton(i18nc("@option:option", "Any Rating"));
    m_oneOrMore = new QRadioButton(i18nc("@option:option", "1 or more"));
    m_twoOrMore = new QRadioButton(i18nc("@option:option", "2 or more"));
    m_threeOrMore = new QRadioButton(i18nc("@option:option", "3 or more"));
    m_fourOrMore = new QRadioButton(i18nc("@option:option", "4 or more"));
    m_maxRating = new QRadioButton(i18nc("@option:option", "Maximum Rating"));

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

    // TODO:
    m_anytime->setChecked(true);
    m_anyRating->setChecked(true);
}

DolphinFacetsWidget::~DolphinFacetsWidget()
{
}

#include "dolphinfacetswidget.moc"
