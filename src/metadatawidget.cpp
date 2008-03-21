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

#include "commentwidget.h"

#include <config-nepomuk.h>

#include <klocale.h>
#include <KDebug>
#include <KMessageBox>

#include <QtCore/QEvent>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QTextEdit>

#ifdef HAVE_NEPOMUK
#include "nepomukmassupdatejob.h"
#include <nepomuk/kmetadatatagwidget.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/resource.h>
#include <nepomuk/variant.h>
#include <nepomuk/kratingwidget.h>
#include <Soprano/Vocabulary/Xesam>
#include "tagcloud/resourcetaggingwidget.h"
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

    CommentWidget* editComment;
    KRatingWidget* ratingWidget;
    Nepomuk::ResourceTaggingWidget* tagWidget;
#endif
};

#ifdef HAVE_NEPOMUK
void MetaDataWidget::Private::loadComment(const QString& comment)
{
    editComment->setComment( comment );
}
#endif


MetaDataWidget::MetaDataWidget(QWidget* parent) :
    QWidget(parent)
{
#ifdef HAVE_NEPOMUK
    d = new Private;
    d->editComment = new CommentWidget(this);
    d->editComment->setFocusPolicy(Qt::ClickFocus);
    d->ratingWidget = new KRatingWidget(this);
    d->ratingWidget->setAlignment( Qt::AlignCenter );
    d->tagWidget = new Nepomuk::ResourceTaggingWidget(this);
    connect(d->ratingWidget, SIGNAL(ratingChanged(unsigned int)), this, SLOT(slotRatingChanged(unsigned int)));
    connect(d->editComment, SIGNAL(commentChanged(const QString&)), this, SLOT(slotCommentChanged(const QString&)));
    connect( d->tagWidget, SIGNAL( tagClicked( const Nepomuk::Tag& ) ), this, SLOT( slotTagClicked( const Nepomuk::Tag& ) ) );

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setMargin(0);
    lay->addWidget(d->ratingWidget);
    lay->addWidget(d->editComment);
    QHBoxLayout* hbox = new QHBoxLayout;
    lay->addWidget( d->tagWidget );
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
    kDebug() << url;
    KUrl::List urls;
    urls.append( url );
    setFiles( urls );
}


void MetaDataWidget::setFiles(const KUrl::List& urls)
{
#ifdef HAVE_NEPOMUK
    d->files.clear();
    bool first = true;
    QList<Nepomuk::Resource> fileRes;
    Q_FOREACH( KUrl url, urls ) {
        Nepomuk::Resource file( url, Soprano::Vocabulary::Xesam::File() );
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
             d->editComment->comment() != file.description() ) {
            d->loadComment( QString() );
        }
        else if ( first ) {
            d->loadComment( file.description() );
        }
        first = false;
    }
    d->tagWidget->setResource( fileRes.first() );
#endif
}


void MetaDataWidget::slotCommentChanged( const QString& s )
{
#ifdef HAVE_NEPOMUK
    Nepomuk::MassUpdateJob* job = Nepomuk::MassUpdateJob::commentResources( d->files.values(), s );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( metadataUpdateDone() ) );
    setEnabled( false ); // no updates during execution
    job->start();
#endif
}


void MetaDataWidget::slotRatingChanged(unsigned int rating)
{
#ifdef HAVE_NEPOMUK
    Nepomuk::MassUpdateJob* job = Nepomuk::MassUpdateJob::rateResources( d->files.values(), rating );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( metadataUpdateDone() ) );
    setEnabled( false ); // no updates during execution
    job->start();
#endif
}


void MetaDataWidget::metadataUpdateDone()
{
    setEnabled( true );
}


bool MetaDataWidget::eventFilter(QObject* obj, QEvent* event)
{
    return QWidget::eventFilter(obj, event);
}


void MetaDataWidget::slotTagClicked( const Nepomuk::Tag& tag )
{
    // FIXME
    KMessageBox::information( this, "FIXME: connect me to the dolphinmodel: tags:/" + tag.genericLabel() );
}

#include "metadatawidget.moc"
