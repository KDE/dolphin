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

#include "tagsearchfilterwidget.h"

#include <nepomuk/tag.h>
#include <nepomuk/comparisonterm.h>
#include <nepomuk/literalterm.h>
#include <nepomuk/orterm.h>
#include <nepomuk/property.h>
#include <nepomuk/query.h>
#include <klocale.h>
#include <Soprano/LiteralValue>
#include <Soprano/Vocabulary/NAO>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

static bool tagLabelLessThan(const Nepomuk::Tag& t1, const Nepomuk::Tag& t2)
{
    return t1.genericLabel() < t2.genericLabel();
}

TagSearchFilterWidget::TagSearchFilterWidget(QWidget* parent) :
    AbstractSearchFilterWidget(parent),
    m_tagButtons()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(0);

    QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
    qSort(tags.begin(), tags.end(), tagLabelLessThan);

    // TODO: handle case if no tag is available
    foreach (const Nepomuk::Tag& tag, tags) {
        QPushButton* button = createButton();
        button->setText(tag.genericLabel());

        layout->addWidget(button);
        m_tagButtons.append(button);
    }

    layout->addStretch(1);
}

TagSearchFilterWidget::~TagSearchFilterWidget()
{
}

QString TagSearchFilterWidget::filterLabel() const
{
    return i18nc("@title:group", "Tag");
}

Nepomuk::Query::Term TagSearchFilterWidget::queryTerm() const
{
    Nepomuk::Query::OrTerm orTerm;

    foreach (const QPushButton* tagButton, m_tagButtons) {
        if (tagButton->isChecked()) {
            const Nepomuk::Query::LiteralTerm term(tagButton->text());
            const Nepomuk::Query::ComparisonTerm compTerm(Soprano::Vocabulary::NAO::hasTag(),
                                                          term,
                                                          Nepomuk::Query::ComparisonTerm::Equal);
            orTerm.addSubTerm(compTerm);
        }
    }

    return orTerm;
}

#include "tagsearchfilterwidget.moc"
