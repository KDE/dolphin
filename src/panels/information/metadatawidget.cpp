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
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
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
#include "resourcetaggingwidget.h"
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

    CommentWidget* editComment;
    KRatingWidget* ratingWidget;
    Nepomuk::ResourceTaggingWidget* tagWidget;

    // shared data between the GUI-thread and
    // the loader-thread (see LoadFilesThread):
    QMutex mutex;
    struct SharedData
    {
        int rating;
        QString comment;
        QList<Nepomuk::Resource> fileRes;
        QMap<KUrl, Nepomuk::Resource> files;
    } sharedData;

    /**
     * Loads the meta data of files and writes
     * the result into a shared data pool that
     * can be used by the widgets in the GUI thread.
     */
    class LoadFilesThread : public QThread
    {
    public:
        LoadFilesThread(SharedData* sharedData, QMutex* mutex);
        ~LoadFilesThread();
        void loadFiles(const KUrl::List& urls);
        virtual void run();

    private:
        SharedData* m_sharedData;
        QMutex* m_mutex;
        KUrl::List m_urls;
        bool m_canceled;
    };

    LoadFilesThread* loadFilesThread;
#endif
};

#ifdef HAVE_NEPOMUK
void MetaDataWidget::Private::loadComment(const QString& comment)
{
    editComment->setComment( comment );
}

MetaDataWidget::Private::LoadFilesThread::LoadFilesThread(
    MetaDataWidget::Private::SharedData* sharedData,
    QMutex* mutex) :
    m_sharedData(sharedData),
    m_mutex(mutex),
    m_urls(),
    m_canceled(false)
{
}

MetaDataWidget::Private::LoadFilesThread::~LoadFilesThread()
{
    // this thread may very well be deleted during execution. We need
    // to protect it from crashes here
    m_canceled = true;
    wait();
}

void MetaDataWidget::Private::LoadFilesThread::loadFiles(const KUrl::List& urls)
{
    QMutexLocker locker( m_mutex );
    m_urls = urls;
    m_canceled = false;
    start();
}

void MetaDataWidget::Private::LoadFilesThread::run()
{
    QMutexLocker locker( m_mutex );
    const KUrl::List urls = m_urls;
    locker.unlock();

    bool first = true;
    QList<Nepomuk::Resource> fileRes;
    QMap<KUrl, Nepomuk::Resource> files;
    unsigned int rating = 0;
    QString comment;
    Q_FOREACH( const KUrl &url, urls ) {
        if ( m_canceled )
            return;
        Nepomuk::Resource file( url, Soprano::Vocabulary::Xesam::File() );
        files.insert( url, file );
        fileRes.append( file );

        if ( !first && rating != file.rating() ) {
            rating = 0; // reset rating
        }
        else if ( first ) {
            rating = file.rating();
        }

        if ( !first && comment != file.description() ) {
            comment.clear();
        }
        else if ( first ) {
            comment = file.description();
        }
        first = false;
    }

    locker.relock();
    m_sharedData->rating = rating;
    m_sharedData->comment = comment;
    m_sharedData->fileRes = fileRes;
    m_sharedData->files = files;
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

    d->sharedData.rating = 0;
    d->loadFilesThread = new Private::LoadFilesThread(&d->sharedData, &d->mutex);
    connect(d->loadFilesThread, SIGNAL(finished()), this, SLOT(slotLoadingFinished()));

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setMargin(0);
    lay->addWidget(d->ratingWidget);
    lay->addWidget(d->editComment);
    lay->addWidget( d->tagWidget );
#else
    d = 0;
#endif
}


MetaDataWidget::~MetaDataWidget()
{
#ifdef HAVE_NEPOMUK
    delete d->loadFilesThread;
#endif
    delete d;
}

void MetaDataWidget::setRatingVisible(bool visible)
{
#ifdef HAVE_NEPOMUK
    d->ratingWidget->setVisible(visible);
#else
    Q_UNUSED(visible);
#endif
}


bool MetaDataWidget::isRatingVisible() const
{
#ifdef HAVE_NEPOMUK
    return d->ratingWidget->isVisible();
#else
    return false;
#endif
}


void MetaDataWidget::setCommentVisible(bool visible)
{
#ifdef HAVE_NEPOMUK
    d->editComment->setVisible(visible);
#else
    Q_UNUSED(visible);
#endif
}


bool MetaDataWidget::isCommentVisible() const
{
#ifdef HAVE_NEPOMUK
    return d->editComment->isVisible();
#else
    return false;
#endif
}


void MetaDataWidget::setTagsVisible(bool visible)
{
#ifdef HAVE_NEPOMUK
    d->tagWidget->setVisible(visible);
#else
    Q_UNUSED(visible);
#endif
}


bool MetaDataWidget::areTagsVisible() const
{
#ifdef HAVE_NEPOMUK
    return d->tagWidget->isVisible();
#else
    return false;
#endif
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
    d->loadFilesThread->loadFiles( urls );
#else
    Q_UNUSED( urls );
#endif
}


void MetaDataWidget::slotCommentChanged( const QString& s )
{
#ifdef HAVE_NEPOMUK
    QMutexLocker locker( &d->mutex );
    Nepomuk::MassUpdateJob* job = Nepomuk::MassUpdateJob::commentResources( d->sharedData.files.values(), s );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( metadataUpdateDone() ) );
    setEnabled( false ); // no updates during execution
    job->start();
#else
    Q_UNUSED( s );
#endif
}


void MetaDataWidget::slotRatingChanged(unsigned int rating)
{
#ifdef HAVE_NEPOMUK
    QMutexLocker locker( &d->mutex );
    Nepomuk::MassUpdateJob* job = Nepomuk::MassUpdateJob::rateResources( d->sharedData.files.values(), rating );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( metadataUpdateDone() ) );
    setEnabled( false ); // no updates during execution
    job->start();
#else
    Q_UNUSED( rating );
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
    Q_UNUSED( tag );
#ifdef HAVE_NEPOMUK
    d->tagWidget->showTagPopup( QCursor::pos() );
#endif
}

void MetaDataWidget::slotLoadingFinished()
{
#ifdef HAVE_NEPOMUK
    QMutexLocker locker( &d->mutex );
    d->ratingWidget->setRating( d->sharedData.rating );
    d->loadComment( d->sharedData.comment );
    d->tagWidget->setResources( d->sharedData.fileRes );
#endif
}

#include "metadatawidget.moc"
