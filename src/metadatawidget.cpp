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

#include "metadatawidget.h"

#include <config-nepomuk.h>

#include <klocale.h>

#include <QtCore/QEvent>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QTextEdit>

#ifdef HAVE_NEPOMUK
#include <nepomuk/kmetadatatagwidget.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/resource.h>
#include <nepomuk/variant.h>
#include <nepomuk/kratingwidget.h>
#include <nepomuk/global.h>
#include <Soprano/Vocabulary/Xesam>
#endif


bool MetaDataWidget::metaDataAvailable()
{
#ifdef HAVE_NEPOMUK
    return !Nepomuk::ResourceManager::instance()->init();
#else
    return false;
#endif
}


class MetaDataWidget::Private
{
public:
#ifdef HAVE_NEPOMUK
    void loadComment(const QString& comment);

    QMap<KUrl, Nepomuk::Resource> files;

    QTextEdit* editComment;
    KRatingWidget* ratingWidget;
    Nepomuk::TagWidget* tagWidget;
#endif
};

#ifdef HAVE_NEPOMUK
void MetaDataWidget::Private::loadComment(const QString& comment)
{
    editComment->blockSignals(true);
    if (comment.isEmpty()) {
        editComment->setFontItalic(true);
        editComment->setText(i18nc("@info:tooltip", "Click to add comment..."));
    } else {
        editComment->setFontItalic(false);
        editComment->setText(comment);
    }
    editComment->blockSignals(false);
}
#endif


MetaDataWidget::MetaDataWidget(QWidget* parent) :
    QWidget(parent)
{
#ifdef HAVE_NEPOMUK
    d = new Private;
    d->editComment = new QTextEdit(this);
    d->editComment->setFocusPolicy(Qt::ClickFocus);
    d->ratingWidget = new KRatingWidget(this);
    d->tagWidget = new Nepomuk::TagWidget(this);
    connect(d->ratingWidget, SIGNAL(ratingChanged(unsigned int)), this, SLOT(slotRatingChanged(unsigned int)));
    connect(d->editComment, SIGNAL(textChanged()), this, SLOT(slotCommentChanged()));

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setMargin(0);
    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(i18nc("@label:slider", "Rating:"), this));
    hbox->addStretch(1);
    hbox->addWidget(d->ratingWidget);
    lay->addLayout(hbox);
    lay->addWidget(d->editComment);
    hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(i18nc("@label:textbox", "Tags:"), this));
    hbox->addWidget(d->tagWidget, 1);
    lay->addLayout(hbox);

    d->editComment->installEventFilter(this);
    d->editComment->viewport()->installEventFilter(this);
#else
    d = 0;
#endif
}


MetaDataWidget::~MetaDataWidget()
{
    delete d;
}


void MetaDataWidget::setFile(const KUrl& url)
{
    KUrl::List urls;
    urls.append( url );
    setFiles( urls );
}


void MetaDataWidget::setFiles(const KUrl::List& urls)
{
#ifdef HAVE_NEPOMUK
    // FIXME #1: For 100 files MetaDataWidget::setFiles() blocks
    // for around 15 seconds (maybe we should delegate this to a KJob).
    // In the meantime we temporary just reset the widgets:
    d->ratingWidget->setRating( 0 );
    d->loadComment( QString() );
    return;

    // FIXME #2: replace with KMetaData::File once we have it again
    d->files.clear();
    bool first = true;
    QList<Nepomuk::Resource> fileRes;
    Q_FOREACH( KUrl url, urls ) {
        Nepomuk::Resource file( url.url(), Soprano::Vocabulary::Xesam::File() );
//    file.setLocation(url.url());
        d->files.insert( url, file );
        fileRes.append( file );

       if ( !first &&
            d->ratingWidget->rating() != file.rating() ) {
           d->ratingWidget->setRating( 0 ); // reset rating
       }
       else if ( first ) {
           d->ratingWidget->setRating( (qint32)(file.rating()) );
       }

       if ( !first &&
            d->editComment->toPlainText() != file.description() ) {
           d->loadComment( QString() );
       }
       else if ( first ) {
           d->loadComment( file.description() );
       }
       first = false;
    }
    d->tagWidget->setTaggedResources(fileRes);
#endif
}


void MetaDataWidget::slotCommentChanged()
{
#ifdef HAVE_NEPOMUK
    for ( QMap<KUrl, Nepomuk::Resource>::iterator it = d->files.begin();
          it != d->files.end(); ++it ) {
        it.value().setDescription(d->editComment->toPlainText());
    }
#endif
}


void MetaDataWidget::slotRatingChanged(unsigned int rating)
{
#ifdef HAVE_NEPOMUK
    for ( QMap<KUrl, Nepomuk::Resource>::iterator it = d->files.begin();
          it != d->files.end(); ++it ) {
        it.value().setRating(rating);
    }
#endif
}


bool MetaDataWidget::eventFilter(QObject* obj, QEvent* event)
{
#ifdef HAVE_NEPOMUK
    if (obj == d->editComment->viewport() || obj == d->editComment) {
        if (event->type() == QEvent::FocusOut) {
            // make sure the info text is displayed again
            d->loadComment(d->editComment->toPlainText());
        } else if (event->type() == QEvent::FocusIn) {
            d->editComment->setFontItalic(false);
            if (!d->files.isEmpty() && d->files.begin().value().description().isEmpty()) {
                d->editComment->setText(QString());
            }
        }
    }
#endif

    return QWidget::eventFilter(obj, event);
}

#include "metadatawidget.moc"
