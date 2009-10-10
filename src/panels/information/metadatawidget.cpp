/***************************************************************************
 *   Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                 *
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include <kfileitem.h>
#include <klocale.h>

#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QString>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #include "commentwidget_p.h"
    #include "nepomukmassupdatejob_p.h"
    #include "taggingwidget_p.h"

    #include <nepomuk/kratingwidget.h>
    #include <nepomuk/resource.h>
    #include <nepomuk/tag.h>
    #include <Soprano/Vocabulary/Xesam>
    #include <QMutex>
    #include <QThread>
#endif

class MetaDataWidget::Private
{
public:
    struct Row
    {
        QLabel* label;
        QWidget* infoWidget;
    };

    Private(MetaDataWidget* parent);
    ~Private();

    void addRow(QLabel* label, QWidget* infoWidget);
    void setRowVisible(QWidget* infoWidget, bool visible);

    void slotLoadingFinished();

    QList<Row> rows;

    QGridLayout* gridLayout;

    QLabel* typeInfo;
    QLabel* sizeLabel;
    QLabel* sizeInfo;
    QLabel* modifiedInfo;
    QLabel* ownerInfo;
    QLabel* permissionsInfo;

#ifdef HAVE_NEPOMUK
    KRatingWidget* ratingWidget;
    TaggingWidget* taggingWidget;
    CommentWidget* commentWidget;

    // shared data between the GUI-thread and
    // the loader-thread (see LoadFilesThread):
    QMutex mutex;
    struct SharedData
    {
        int rating;
        QString comment;
        QList<Nepomuk::Tag> tags;
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
        virtual ~LoadFilesThread();
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

private:
    MetaDataWidget* const q;
};

MetaDataWidget::Private::Private(MetaDataWidget* parent) :
    rows(),
    gridLayout(0),
    typeInfo(0),
    sizeLabel(0),
    sizeInfo(0),
    modifiedInfo(0),
    ownerInfo(0),
    permissionsInfo(0),
#ifdef HAVE_NEPOMUK
    ratingWidget(0),
    taggingWidget(0),
    commentWidget(0),
    loadFilesThread(0),
#endif
    q(parent)
{
    gridLayout = new QGridLayout(parent);

    typeInfo = new QLabel(parent);
    sizeLabel = new QLabel(parent);
    sizeInfo = new QLabel(parent);
    modifiedInfo = new QLabel(parent);
    ownerInfo = new QLabel(parent);
    permissionsInfo = new QLabel(parent);
#ifdef HAVE_NEPOMUK
    ratingWidget = new KRatingWidget(parent);
    taggingWidget = new TaggingWidget(parent);
    commentWidget = new CommentWidget(parent);
#endif

    addRow(new QLabel(i18nc("@label", "Type:"), parent), typeInfo);
    addRow(sizeLabel, sizeInfo);
    addRow(new QLabel(i18nc("@label", "Modified:"), parent), modifiedInfo);
    addRow(new QLabel(i18nc("@label", "Owner:"), parent), ownerInfo);
    addRow(new QLabel(i18nc("@label", "Permissions:"), parent), permissionsInfo);
#ifdef HAVE_NEPOMUK
    addRow(new QLabel(i18nc("@label", "Rating:"), parent), ratingWidget);
    addRow(new QLabel(i18nc("@label", "Tags:"), parent), taggingWidget);
    addRow(new QLabel(i18nc("@label", "Comment:"), parent), commentWidget);

    sharedData.rating = 0;
    loadFilesThread = new LoadFilesThread(&sharedData, &mutex);
    connect(loadFilesThread, SIGNAL(finished()), q, SLOT(slotLoadingFinished()));
#endif
}

MetaDataWidget::Private::~Private()
{
#ifdef HAVE_NEPOMUK
    delete loadFilesThread;
#endif
}

void MetaDataWidget::Private::addRow(QLabel* label, QWidget* infoWidget)
{
    Row row;
    row.label = label;
    row.infoWidget = infoWidget;
    rows.append(row);

    // use a brighter color for the label
    QPalette palette = label->palette();
    QColor textColor = palette.color(QPalette::Text);
    textColor.setAlpha(128);
    palette.setColor(QPalette::WindowText, textColor);
    label->setPalette(palette);

    // add the row to grid layout
    const int rowIndex = rows.count();
    gridLayout->addWidget(label, rowIndex, 0, Qt::AlignLeft);
    gridLayout->addWidget(infoWidget, rowIndex, 1, Qt::AlignRight);
}

void MetaDataWidget::Private::setRowVisible(QWidget* infoWidget, bool visible)
{
    foreach (const Row& row, rows) {
        if (row.infoWidget == infoWidget) {
            row.label->setVisible(visible);
            row.infoWidget->setVisible(visible);
            return;
        }
    }
}

void MetaDataWidget::Private::slotLoadingFinished()
{
#ifdef HAVE_NEPOMUK
    QMutexLocker locker(&mutex);
    ratingWidget->setRating(sharedData.rating);
    commentWidget->setText(sharedData.comment);
    taggingWidget->setTags(sharedData.tags);
#endif
}

#ifdef HAVE_NEPOMUK
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
    // This thread may very well be deleted during execution. We need
    // to protect it from crashes here.
    m_canceled = true;
    wait();
}

void MetaDataWidget::Private::LoadFilesThread::loadFiles(const KUrl::List& urls)
{
    QMutexLocker locker(m_mutex);
    m_urls = urls;
    m_canceled = false;
    start();
}

void MetaDataWidget::Private::LoadFilesThread::run()
{
    QMutexLocker locker(m_mutex);
    const KUrl::List urls = m_urls;
    locker.unlock();

    bool first = true;
    unsigned int rating = 0;
    QString comment;
    QList<Nepomuk::Tag> tags;
    foreach (const KUrl& url, urls) {
        if (m_canceled) {
            return;
        }

        Nepomuk::Resource file(url, Soprano::Vocabulary::Xesam::File());

        if (!first && (rating != file.rating())) {
            rating = 0; // reset rating
        } else if (first) {
            rating = file.rating();
        }

        if (!first && (comment != file.description())) {
            comment.clear(); // reset comment
        } else if (first) {
            comment = file.description();
        }

        if (!first && (tags != file.tags())) {
            tags.clear(); // reset tags
        } else if (first) {
            tags = file.tags();
        }

        first = false;
    }

    locker.relock();
    m_sharedData->rating = rating;
    m_sharedData->comment = comment;
    m_sharedData->tags = tags;
}
#endif

MetaDataWidget::MetaDataWidget(QWidget* parent) :
    QWidget(parent),
    d(new Private(this))
{
}

MetaDataWidget::~MetaDataWidget()
{
    delete d;
}

void MetaDataWidget::setItem(const KFileItem& item)
{
    // update values for "type", "size", "modified",
    // "owner" and "permissions" synchronously
    d->sizeLabel->setText(i18nc("@label", "Size:"));
    if (item.isDir()) {
        d->typeInfo->setText(i18nc("@label", "Folder"));
        d->setRowVisible(d->sizeInfo, false);
    } else {
        d->typeInfo->setText(item.mimeComment());
        d->sizeInfo->setText(KIO::convertSize(item.size()));
        d->setRowVisible(d->sizeInfo, true);
    }
    d->modifiedInfo->setText(item.timeString());
    d->ownerInfo->setText(item.user());
    d->permissionsInfo->setText(item.permissionsString());

    setItems(KFileItemList() << item);
}

void MetaDataWidget::setItems(const KFileItemList& items)
{
    if (items.count() > 1) {
        // calculate the size of all items and show this
        // information to the user
        d->sizeLabel->setText(i18nc("@label", "Total Size:"));
        d->setRowVisible(d->sizeInfo, true);

        quint64 totalSize = 0;
        foreach (const KFileItem& item, items) {
            if (!item.isDir() && !item.isLink()) {
                totalSize += item.size();
            }
        }
        d->sizeInfo->setText(KIO::convertSize(totalSize));
    }

#ifdef HAVE_NEPOMUK
    QList<KUrl> urls;
    foreach (const KFileItem& item, items) {
        const KUrl url = item.nepomukUri();
        if (url.isValid()) {
            urls.append(url);
        }
    }
    d->loadFilesThread->loadFiles(urls);
#endif
}

#include "metadatawidget.moc"
