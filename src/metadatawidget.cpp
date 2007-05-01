/***************************************************************************
 *   Copyright (C) 2007 by Sebastian Trueg <trueg@kde.org>                 *
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

#include <config-kmetadata.h>

#include "metadatawidget.h"

#include <klocale.h>

#include <QLabel>
#include <QGridLayout>
#include <QTextEdit>

#ifdef HAVE_KMETADATA
#include <kmetadata/kmetadatatagwidget.h>
#include <kmetadata/resourcemanager.h>
#include <kmetadata/resource.h>
#include <kmetadata/kratingwidget.h>
#include <kmetadata/kmetadatatagwidget.h>
#endif


bool MetaDataWidget::metaDataAvailable()
{
#ifdef HAVE_KMETADATA
    return !Nepomuk::KMetaData::ResourceManager::instance()->init();
#else
    return false;
#endif
}


class MetaDataWidget::Private
{
public:
#ifdef HAVE_KMETADATA
    void loadComment(const QString& comment)
    {
        editComment->blockSignals(true);
        if (comment.isEmpty()) {
            editComment->setFontItalic(true);
            editComment->setText(i18n("Click to add comment..."));
        } else {
            editComment->setFontItalic(false);
            editComment->setText(comment);
        }
        editComment->blockSignals(false);
    }

    KUrl fileUrl;

    Nepomuk::KMetaData::Resource file;

    QTextEdit* editComment;
    KRatingWidget* ratingWidget;
    Nepomuk::KMetaData::TagWidget* tagWidget;
#endif
};


MetaDataWidget::MetaDataWidget(QWidget* parent)
        : QWidget(parent)
{
#ifdef HAVE_KMETADATA
    d = new Private;
    d->editComment = new QTextEdit(this);
    d->tagWidget = new Nepomuk::KMetaData::TagWidget(this);
    d->ratingWidget = new KRatingWidget(this);
    connect(d->ratingWidget, SIGNAL(ratingChanged(int)), this, SLOT(slotRatingChanged(int)));
    connect(d->editComment, SIGNAL(textChanged()), this, SLOT(slotCommentChanged()));

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setMargin(0);
    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(i18n("Rating:"), this));
    hbox->addStretch(1);
    hbox->addWidget(d->ratingWidget);
    lay->addLayout(hbox);
    lay->addWidget(d->editComment);
    hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(i18n("Tags:"), this));
    hbox->addWidget(d->tagWidget, 1);
    lay->addLayout(hbox);

    d->editComment->installEventFilter(this);
    d->editComment->viewport()->installEventFilter(this);
#else
    d = 0L;
#endif
}


MetaDataWidget::~MetaDataWidget()
{
    delete d;
}


void MetaDataWidget::setFile(const KUrl& url)
{
#ifdef HAVE_KMETADATA
    // FIXME: replace with KMetaData::File once we have it again
    d->fileUrl = url;
    d->file = Nepomuk::KMetaData::Resource(url.url());
//    d->file.setLocation(url.url());
    d->ratingWidget->setRating(d->file.getRating());
    d->tagWidget->setTaggedResource(d->file);
    d->loadComment(d->file.getComment());
#endif
}


void MetaDataWidget::setFiles(const KUrl::List urls)
{
#ifdef HAVE_KMETADATA
    // FIXME: support multiple files
    setFile(urls.first());
#endif
}


void MetaDataWidget::slotCommentChanged()
{
#ifdef HAVE_KMETADATA
    d->file.setComment(d->editComment->toPlainText());
#endif
}


void MetaDataWidget::slotRatingChanged(int r)
{
#ifdef HAVE_KMETADATA
    d->file.setRating(r);
#endif
}


bool MetaDataWidget::eventFilter(QObject* obj, QEvent* event)
{
#ifdef HAVE_KMETADATA
    if (obj == d->editComment->viewport()
            || obj == d->editComment) {
        if (event->type() == QEvent::FocusOut) {
            // make sure the info text is displayed again
            d->loadComment(d->editComment->toPlainText());
        } else if (event->type() == QEvent::FocusIn) {
            d->editComment->setFontItalic(false);
            if (d->file.getComment().isEmpty())
                d->editComment->setText(QString());
        }
    }
#endif

    return QWidget::eventFilter(obj, event);
}

#include "metadatawidget.moc"
