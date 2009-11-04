/*****************************************************************************
 * Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                     *
 * Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#include "kmetadatawidget.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <klocale.h>

#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QString>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #define DISABLE_NEPOMUK_LEGACY

    #include "kcommentwidget_p.h"
    #include "ktaggingwidget_p.h"

    #include <nepomuk/kratingwidget.h>
    #include <nepomuk/resource.h>
    #include <nepomuk/resourcemanager.h>
    #include <nepomuk/property.h>
    #include <nepomuk/variant.h>
    #include "nepomukmassupdatejob_p.h"

    #include <QMutex>
    #include <QSpacerItem>
    #include <QThread>
#endif

class KMetaDataWidget::Private
{
public:
    struct Row
    {
        QLabel* label;
        QWidget* infoWidget;
    };

    Private(KMetaDataWidget* parent);
    ~Private();

    void addRow(QLabel* label, QWidget* infoWidget);
    void removeMetaInfoRows();
    void setRowVisible(QWidget* infoWidget, bool visible);

    /**
     * Initializes the configuration file "kmetainformationrc"
     * with proper default settings for the first start in
     * an uninitialized environment.
     */
    void initMetaInfoSettings();

    /**
     * Parses the configuration file "kmetainformationrc" and
     * updates the visibility of all rows.
     */
    void updateRowsVisibility();

    void slotLoadingFinished();
    void slotRatingChanged(unsigned int rating);
    void slotTagsChanged(const QList<Nepomuk::Tag>& tags);
    void slotCommentChanged(const QString& comment);
    void slotMetaDataUpdateDone();

    /**
     * Disables the metadata widget and starts the job that
     * changes the meta data asynchronously. After the job
     * has been finished, the metadata widget gets enabled again.
     */
    void startChangeDataJob(KJob* job);

    bool m_isSizeVisible;
    MetaDataTypes m_visibleDataTypes;
    QList<KFileItem> m_fileItems;
    QList<Row> m_rows;

    QGridLayout* m_gridLayout;

    QLabel* m_typeInfo;
    QLabel* m_sizeLabel;
    QLabel* m_sizeInfo;
    QLabel* m_modifiedInfo;
    QLabel* m_ownerInfo;
    QLabel* m_permissionsInfo;

#ifdef HAVE_NEPOMUK
    KRatingWidget* m_ratingWidget;
    KTaggingWidget* m_taggingWidget;
    KCommentWidget* m_commentWidget;

    // shared data between the GUI-thread and
    // the loader-thread (see LoadFilesThread):
    QMutex m_mutex;
    struct SharedData
    {
        int rating;
        QString comment;
        QList<Nepomuk::Tag> tags;
        QList<QString> metaInfoLabels;
        QList<QString> metaInfoValues;
        QMap<KUrl, Nepomuk::Resource> files;
    } m_sharedData;

    /**
     * Loads the meta data of files and writes
     * the result into a shared data pool that
     * can be used by the widgets in the GUI thread.
     */
    class LoadFilesThread : public QThread
    {
    public:
        LoadFilesThread(SharedData* m_sharedData, QMutex* m_mutex);
        virtual ~LoadFilesThread();
        void loadFiles(const KUrl::List& urls);
        virtual void run();

    private:
        /**
         * Assures that the settings for the meta information
         * are initialized with proper default values.
         */
        void initMetaInfoSettings(KConfigGroup& group);

        /**
         * Temporary helper method for KDE 4.3 as we currently don't get
         * translated labels for Nepmok literals: Replaces camelcase labels
         * like "fileLocation" by "File Location:".
         */
        QString tunedLabel(const QString& label) const;

    private:
        SharedData* m_sharedData;
        QMutex* m_mutex;
        KUrl::List m_urls;
        bool m_canceled;
    };

    LoadFilesThread* m_loadFilesThread;
#endif

private:
    KMetaDataWidget* const q;
};

KMetaDataWidget::Private::Private(KMetaDataWidget* parent) :
    m_isSizeVisible(true),
    m_visibleDataTypes(TypeData | SizeData | ModifiedData | OwnerData |
                PermissionsData | RatingData | TagsData | CommentData),
    m_fileItems(),
    m_rows(),
    m_gridLayout(0),
    m_typeInfo(0),
    m_sizeLabel(0),
    m_sizeInfo(0),
    m_modifiedInfo(0),
    m_ownerInfo(0),
    m_permissionsInfo(0),
#ifdef HAVE_NEPOMUK
    m_ratingWidget(0),
    m_taggingWidget(0),
    m_commentWidget(0),
    m_loadFilesThread(0),
#endif
    q(parent)
{
    m_gridLayout = new QGridLayout(parent);
    m_gridLayout->setMargin(0);

    m_typeInfo = new QLabel(parent);
    m_sizeLabel = new QLabel(parent);
    m_sizeInfo = new QLabel(parent);
    m_modifiedInfo = new QLabel(parent);
    m_ownerInfo = new QLabel(parent);
    m_permissionsInfo = new QLabel(parent);

    addRow(new QLabel(i18nc("@label", "Type:"), parent), m_typeInfo);
    addRow(m_sizeLabel, m_sizeInfo);
    addRow(new QLabel(i18nc("@label", "Modified:"), parent), m_modifiedInfo);
    addRow(new QLabel(i18nc("@label", "Owner:"), parent), m_ownerInfo);
    addRow(new QLabel(i18nc("@label", "Permissions:"), parent), m_permissionsInfo);

#ifdef HAVE_NEPOMUK
    if (Nepomuk::ResourceManager::instance()->init() == 0) {
        const QFontMetrics fontMetrics(KGlobalSettings::smallestReadableFont());
        m_ratingWidget = new KRatingWidget(parent);
        m_ratingWidget->setFixedHeight(fontMetrics.height());
        connect(m_ratingWidget, SIGNAL(ratingChanged(unsigned int)),
                q, SLOT(slotRatingChanged(unsigned int)));

        m_taggingWidget = new KTaggingWidget(parent);
        connect(m_taggingWidget, SIGNAL(tagsChanged(const QList<Nepomuk::Tag>&)),
                q, SLOT(slotTagsChanged(const QList<Nepomuk::Tag>&)));

        m_commentWidget = new KCommentWidget(parent);
        connect(m_commentWidget, SIGNAL(commentChanged(const QString&)),
                q, SLOT(slotCommentChanged(const QString&)));

        addRow(new QLabel(i18nc("@label", "Rating:"), parent), m_ratingWidget);
        addRow(new QLabel(i18nc("@label", "Tags:"), parent), m_taggingWidget);
        addRow(new QLabel(i18nc("@label", "Comment:"), parent), m_commentWidget);

        m_loadFilesThread = new LoadFilesThread(&m_sharedData, &m_mutex);
        connect(m_loadFilesThread, SIGNAL(finished()), q, SLOT(slotLoadingFinished()));
    }

    m_sharedData.rating = 0;
#endif

    initMetaInfoSettings();
    updateRowsVisibility();
}

KMetaDataWidget::Private::~Private()
{
#ifdef HAVE_NEPOMUK
    delete m_loadFilesThread;
#endif
}

void KMetaDataWidget::Private::addRow(QLabel* label, QWidget* infoWidget)
{
    Row row;
    row.label = label;
    row.infoWidget = infoWidget;
    m_rows.append(row);

    const QFont smallFont = KGlobalSettings::smallestReadableFont();
    // use a brighter color for the label and a small font size
    QPalette palette = label->palette();
    QColor textColor = palette.color(QPalette::Text);
    textColor.setAlpha(128);
    palette.setColor(QPalette::WindowText, textColor);
    label->setPalette(palette);
    label->setFont(smallFont);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignTop | Qt::AlignRight);

    QLabel* infoLabel = qobject_cast<QLabel*>(infoWidget);
    if (infoLabel != 0) {
        infoLabel->setFont(smallFont);
        infoLabel->setWordWrap(true);
        infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    }

    // add the row to grid layout
    const int rowIndex = m_rows.count();
    m_gridLayout->addWidget(label, rowIndex, 0, Qt::AlignRight);
    const int spacerWidth = QFontMetrics(smallFont).size(Qt::TextSingleLine, " ").width();
    m_gridLayout->addItem(new QSpacerItem(spacerWidth, 1), rowIndex, 1);
    m_gridLayout->addWidget(infoWidget, rowIndex, 2, Qt::AlignLeft);
}

void KMetaDataWidget::Private::setRowVisible(QWidget* infoWidget, bool visible)
{
    foreach (const Row& row, m_rows) {
        if (row.infoWidget == infoWidget) {
            row.label->setVisible(visible);
            row.infoWidget->setVisible(visible);
            return;
        }
    }
}


void KMetaDataWidget::Private::initMetaInfoSettings()
{
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");
    if (!settings.readEntry("initialized", false)) {
        // The resource file is read the first time. Assure
        // that some meta information is disabled per default.

        static const char* disabledProperties[] = {
            "asText", "contentSize", "created", "depth", "description", "fileExtension",
            "fileName", "fileSize", "hasTag", "isPartOf", "lastModified", "mimeType", "name",
            "numericRating", "parentUrl", "permissions", "plainTextContent", "owner",
            "sourceModified", "url",
            0 // mandatory last entry
        };

        int i = 0;
        while (disabledProperties[i] != 0) {
            settings.writeEntry(disabledProperties[i], false);
            ++i;
        }

        // mark the group as initialized
        settings.writeEntry("initialized", true);
    }
}

void KMetaDataWidget::Private::updateRowsVisibility()
{
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    setRowVisible(m_typeInfo,
                  (m_visibleDataTypes & KMetaDataWidget::TypeData) &&
                  settings.readEntry("type", true));

    // Cache in m_isSizeVisible whether the size should be shown. This
    // is necessary as the size is temporary hidden when the target
    // file item is a directory.
    m_isSizeVisible = (m_visibleDataTypes & KMetaDataWidget::SizeData) &&
                      settings.readEntry("size", true);
    setRowVisible(m_sizeInfo, m_isSizeVisible);

    setRowVisible(m_modifiedInfo,
                  (m_visibleDataTypes & KMetaDataWidget::ModifiedData) &&
                  settings.readEntry("modified", true));

    setRowVisible(m_ownerInfo,
                  (m_visibleDataTypes & KMetaDataWidget::OwnerData) &&
                  settings.readEntry("owner", true));

    setRowVisible(m_permissionsInfo,
                  (m_visibleDataTypes & KMetaDataWidget::PermissionsData) &&
                  settings.readEntry("permissions", true));

#ifdef HAVE_NEPOMUK
    if (Nepomuk::ResourceManager::instance()->init() == 0) {
        setRowVisible(m_ratingWidget,
                      (m_visibleDataTypes & KMetaDataWidget::RatingData) &&
                      settings.readEntry("rating", true));

        setRowVisible(m_taggingWidget,
                      (m_visibleDataTypes & KMetaDataWidget::TagsData) &&
                      settings.readEntry("tags", true));

        setRowVisible(m_commentWidget,
                      (m_visibleDataTypes & KMetaDataWidget::CommentData) &&
                      settings.readEntry("comment", true));
    }
#endif
}

void KMetaDataWidget::Private::slotLoadingFinished()
{
#ifdef HAVE_NEPOMUK
    QMutexLocker locker(&m_mutex);
    m_ratingWidget->setRating(m_sharedData.rating);
    m_commentWidget->setText(m_sharedData.comment);
    m_taggingWidget->setTags(m_sharedData.tags);

    // Show the remaining meta information as text. The number
    // of required rows may very. Existing rows are reused to
    // prevent flickering.
    int index = 8;  // TODO: don't hardcode this value here
    const int rowCount = m_rows.count();
    Q_ASSERT(rowCount >= index);

    Q_ASSERT(m_sharedData.metaInfoLabels.count() == m_sharedData.metaInfoValues.count());
    const int metaInfoCount = m_sharedData.metaInfoLabels.count();
    for (int i = 0; i < metaInfoCount; ++i) {
        if (index < rowCount) {
            // adjust texts of the current row
            m_rows[index].label->setText(m_sharedData.metaInfoLabels[i]);
            QLabel* infoValueLabel = qobject_cast<QLabel*>(m_rows[index].infoWidget);
            Q_ASSERT(infoValueLabel != 0);
            infoValueLabel->setText(m_sharedData.metaInfoValues[i]);
        } else {
            // create new row
            QLabel* infoLabel = new QLabel(m_sharedData.metaInfoLabels[i], q);
            QLabel* infoValue = new QLabel(m_sharedData.metaInfoValues[i], q);
            addRow(infoLabel, infoValue);
        }
        ++index;
    }
    if (metaInfoCount > 0) {
        --index;
    }

    // remove rows that are not needed anymore
    for (int i = rowCount - 1; i > index; --i) {
        delete m_rows[i].label;
        delete m_rows[i].infoWidget;
        m_rows.pop_back();
    }
#endif
    q->updateGeometry();
}

void KMetaDataWidget::Private::slotRatingChanged(unsigned int rating)
{
#ifdef HAVE_NEPOMUK
    QMutexLocker locker(&m_mutex);
    Nepomuk::MassUpdateJob* job =
            Nepomuk::MassUpdateJob::rateResources(m_sharedData.files.values(), rating);
    locker.unlock();
    startChangeDataJob(job);
#endif
}

void KMetaDataWidget::Private::slotTagsChanged(const QList<Nepomuk::Tag>& tags)
{
#ifdef HAVE_NEPOMUK
    m_taggingWidget->setTags(tags);

    QMutexLocker locker(&m_mutex);
    Nepomuk::MassUpdateJob* job =
            Nepomuk::MassUpdateJob::tagResources(m_sharedData.files.values(), tags);
    locker.unlock();
    startChangeDataJob(job);
#endif
}

void KMetaDataWidget::Private::slotCommentChanged(const QString& comment)
{
#ifdef HAVE_NEPOMUK
    QMutexLocker locker(&m_mutex);
    Nepomuk::MassUpdateJob* job =
            Nepomuk::MassUpdateJob::commentResources(m_sharedData.files.values(), comment);
    locker.unlock();
    startChangeDataJob(job);
#endif
}

void KMetaDataWidget::Private::slotMetaDataUpdateDone()
{
    q->setEnabled(true);
}

void KMetaDataWidget::Private::startChangeDataJob(KJob* job)
{
    connect(job, SIGNAL(result(KJob*)),
            q, SLOT(slotMetaDataUpdateDone()));
    q->setEnabled(false); // no updates during execution
    job->start();
}

#ifdef HAVE_NEPOMUK
KMetaDataWidget::Private::LoadFilesThread::LoadFilesThread(
                            KMetaDataWidget::Private::SharedData* m_sharedData,
                            QMutex* m_mutex) :
    m_sharedData(m_sharedData),
    m_mutex(m_mutex),
    m_urls(),
    m_canceled(false)
{
}

KMetaDataWidget::Private::LoadFilesThread::~LoadFilesThread()
{
    // This thread may very well be deleted during execution. We need
    // to protect it from crashes here.
    m_canceled = true;
    wait();
}

void KMetaDataWidget::Private::LoadFilesThread::loadFiles(const KUrl::List& urls)
{
    QMutexLocker locker(m_mutex);
    m_urls = urls;
    m_canceled = false;
    start();
}

void KMetaDataWidget::Private::LoadFilesThread::run()
{
    QMutexLocker locker(m_mutex);
    const KUrl::List urls = m_urls;
    locker.unlock();

    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    bool first = true;
    unsigned int rating = 0;
    QString comment;
    QList<Nepomuk::Tag> tags;
    QList<QString> metaInfoLabels;
    QList<QString> metaInfoValues;
    QMap<KUrl, Nepomuk::Resource> files;
    foreach (const KUrl& url, urls) {
        if (m_canceled) {
            return;
        }

        Nepomuk::Resource file(url);
        files.insert(url, file);

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

        if (first && (urls.count() == 1)) {
            // TODO: show shared meta information instead
            // of not showing anything on multiple selections
            QHash<QUrl, Nepomuk::Variant> properties = file.properties();
            QHash<QUrl, Nepomuk::Variant>::const_iterator it = properties.constBegin();
            while (it != properties.constEnd()) {
                Nepomuk::Types::Property prop(it.key());
                if (settings.readEntry(prop.name(), true)) {
                    // TODO #1: use Nepomuk::formatValue(res, prop) if available
                    // instead of it.value().toString()
                    // TODO #2: using tunedLabel() is a workaround for KDE 4.3 (4.4?) until
                    // we get translated labels
                    metaInfoLabels.append(tunedLabel(prop.label()));
                    metaInfoValues.append(it.value().toString());
                }
                ++it;
            }
        }

        first = false;
    }

    locker.relock();
    m_sharedData->rating = rating;
    m_sharedData->comment = comment;
    m_sharedData->tags = tags;
    m_sharedData->metaInfoLabels = metaInfoLabels;
    m_sharedData->metaInfoValues = metaInfoValues;
    m_sharedData->files = files;
}

QString KMetaDataWidget::Private::LoadFilesThread::tunedLabel(const QString& label) const
{
    QString tunedLabel;
    const int labelLength = label.length();
    if (labelLength > 0) {
        tunedLabel.reserve(labelLength);
        tunedLabel = label[0].toUpper();
        for (int i = 1; i < labelLength; ++i) {
            if (label[i].isUpper() && !label[i - 1].isSpace() && !label[i - 1].isUpper()) {
                tunedLabel += ' ';
                tunedLabel += label[i].toLower();
            } else {
                tunedLabel += label[i];
            }
        }
    }
    return tunedLabel + ':';
}

#endif // HAVE_NEPOMUK

KMetaDataWidget::KMetaDataWidget(QWidget* parent) :
    QWidget(parent),
    d(new Private(this))
{
}

KMetaDataWidget::~KMetaDataWidget()
{
    delete d;
}

void KMetaDataWidget::setItem(const KFileItem& item)
{
    // update values for "type", "size", "modified",
    // "owner" and "permissions" synchronously
    d->m_sizeLabel->setText(i18nc("@label", "Size:"));
    if (item.isDir()) {
        d->m_typeInfo->setText(i18nc("@label", "Folder"));
        d->setRowVisible(d->m_sizeInfo, false);
    } else {
        d->m_typeInfo->setText(item.mimeComment());
        d->m_sizeInfo->setText(KIO::convertSize(item.size()));
        d->setRowVisible(d->m_sizeInfo, d->m_isSizeVisible);
    }
    d->m_modifiedInfo->setText(item.timeString());
    d->m_ownerInfo->setText(item.user());
    d->m_permissionsInfo->setText(item.permissionsString());

    setItems(KFileItemList() << item);
}

void KMetaDataWidget::setItems(const KFileItemList& items)
{
    d->m_fileItems = items;

    if (items.count() > 1) {
        // calculate the size of all items and show this
        // information to the user
        d->m_sizeLabel->setText(i18nc("@label", "Total Size:"));
        d->setRowVisible(d->m_sizeInfo, d->m_isSizeVisible);

        quint64 totalSize = 0;
        foreach (const KFileItem& item, items) {
            if (!item.isDir() && !item.isLink()) {
                totalSize += item.size();
            }
        }
        d->m_sizeInfo->setText(KIO::convertSize(totalSize));
    }

#ifdef HAVE_NEPOMUK
    if (Nepomuk::ResourceManager::instance()->init() == 0) {
        QList<KUrl> urls;
        foreach (const KFileItem& item, items) {
            const KUrl url = item.nepomukUri();
            if (url.isValid()) {
                urls.append(url);
            }
        }
        d->m_loadFilesThread->loadFiles(urls);
    }
#endif
}

void KMetaDataWidget::setItem(const KUrl& url)
{
    KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
    item.refresh();
    setItem(item);
}

void KMetaDataWidget::setItems(const QList<KUrl>& urls)
{
    KFileItemList items;
    foreach (const KUrl& url, urls) {
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
        item.refresh();
        items.append(item);
    }
    setItems(items);
}

KFileItemList KMetaDataWidget::items() const
{
    return d->m_fileItems;
}

void KMetaDataWidget::setVisibleDataTypes(MetaDataTypes data)
{
    d->m_visibleDataTypes = data;
    d->updateRowsVisibility();
}

KMetaDataWidget::MetaDataTypes KMetaDataWidget::visibleDataTypes() const
{
    return d->m_visibleDataTypes;
}

#include "kmetadatawidget.moc"
