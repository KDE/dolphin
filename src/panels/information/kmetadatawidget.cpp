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
#include <kglobal.h>
#include <klocale.h>

#include <QEvent>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QString>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #define DISABLE_NEPOMUK_LEGACY

    #include "kcommentwidget_p.h"
    #include "kloadmetadatathread_p.h"
    #include "ktaggingwidget_p.h"

    #include <nepomuk/kratingwidget.h>
    #include <nepomuk/resource.h>
    #include <nepomuk/resourcemanager.h>
    #include <nepomuk/property.h>
    #include <nepomuk/tag.h>
    #include <nepomuk/variant.h>
    #include "nepomukmassupdatejob_p.h"

    #include <QMutex>
    #include <QSpacerItem>
    #include <QThread>
#else
    namespace Nepomuk
    {
        typedef int Tag;
    }
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
    void slotLinkActivated(const QString& link);

    void slotTagActivated(const Nepomuk::Tag& tag);

#ifdef HAVE_NEPOMUK
    /**
     * Disables the metadata widget and starts the job that
     * changes the meta data asynchronously. After the job
     * has been finished, the metadata widget gets enabled again.
     */
    void startChangeDataJob(KJob* job);

    /**
     * Merges items like 'width' and 'height' as one item.
     */
    QList<KLoadMetaDataThread::Item> mergedItems(const QList<KLoadMetaDataThread::Item>& items);
#endif

    bool m_sizeVisible;
    bool m_readOnly;
    bool m_nepomukActivated;
    int m_fixedRowCount;
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

    QMap<KUrl, Nepomuk::Resource> m_files;

    KLoadMetaDataThread* m_loadMetaDataThread;
#endif

private:
    KMetaDataWidget* const q;
};

KMetaDataWidget::Private::Private(KMetaDataWidget* parent) :
    m_sizeVisible(true),
    m_readOnly(false),
    m_nepomukActivated(false),
    m_fixedRowCount(0),
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
    m_files(),
    m_loadMetaDataThread(0),
#endif
    q(parent)
{
    const QFontMetrics fontMetrics(KGlobalSettings::smallestReadableFont());

    m_gridLayout = new QGridLayout(parent);
    m_gridLayout->setMargin(0);
    m_gridLayout->setSpacing(fontMetrics.height() / 4);

    m_typeInfo = new QLabel(parent);
    m_sizeLabel = new QLabel(parent);
    m_sizeInfo = new QLabel(parent);
    m_modifiedInfo = new QLabel(parent);
    m_ownerInfo = new QLabel(parent);
    m_permissionsInfo = new QLabel(parent);

#ifdef HAVE_NEPOMUK
    m_nepomukActivated = (Nepomuk::ResourceManager::instance()->init() == 0);
    if (m_nepomukActivated) {
        m_ratingWidget = new KRatingWidget(parent);
        m_ratingWidget->setFixedHeight(fontMetrics.height());
        const Qt::Alignment align = (parent->layoutDirection() == Qt::LeftToRight) ?
                                    Qt::AlignLeft : Qt::AlignRight;
        m_ratingWidget->setAlignment(align);
        connect(m_ratingWidget, SIGNAL(ratingChanged(unsigned int)),
                q, SLOT(slotRatingChanged(unsigned int)));

        m_taggingWidget = new KTaggingWidget(parent);
        connect(m_taggingWidget, SIGNAL(tagsChanged(const QList<Nepomuk::Tag>&)),
                q, SLOT(slotTagsChanged(const QList<Nepomuk::Tag>&)));
        connect(m_taggingWidget, SIGNAL(tagActivated(const Nepomuk::Tag&)),
                q, SLOT(slotTagActivated(const Nepomuk::Tag&)));

        m_commentWidget = new KCommentWidget(parent);
        connect(m_commentWidget, SIGNAL(commentChanged(const QString&)),
                q, SLOT(slotCommentChanged(const QString&)));
    }
#endif

    initMetaInfoSettings();
}

KMetaDataWidget::Private::~Private()
{
#ifdef HAVE_NEPOMUK
    if (m_loadMetaDataThread != 0) {
        disconnect(m_loadMetaDataThread, SIGNAL(finished()), q, SLOT(slotLoadingFinished()));
        m_loadMetaDataThread->cancelAndDelete();
        m_loadMetaDataThread = 0;
    }
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
    const QPalette::ColorRole role = q->foregroundRole();
    QColor textColor = palette.color(role);
    textColor.setAlpha(128);
    palette.setColor(role, textColor);
    label->setPalette(palette);
    label->setForegroundRole(role);
    label->setFont(smallFont);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignTop | Qt::AlignRight);

    infoWidget->setForegroundRole(role);
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
    const int currentVersion = 2; // increase version, if the blacklist of disabled
                                  // properties should be updated

    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    if (config.group("Misc").readEntry("version", 0) < currentVersion) {
        // The resource file is read the first time. Assure
        // that some meta information is disabled per default.

        // clear old info
        config.deleteGroup( "Show" );
        KConfigGroup settings = config.group("Show");

        // trueg: KDE 4.5: use a blacklist of actual rdf properties

        static const char* const disabledProperties[] = {
            "asText", "contentSize", "created", "depth", "description", "fileExtension",
            "fileName", "fileSize", "hasTag", "lastModified", "mimeType", "name",
            "numericRating", "parentUrl", "permissions", "plainTextContent", "owner",
            "sourceModified", "url",
            0 // mandatory last entry
        };

        for (int i = 0; disabledProperties[i] != 0; ++i) {
            settings.writeEntry(disabledProperties[i], false);
        }

        // mark the group as initialized
        config.group("Misc").writeEntry("version", currentVersion);
    }
}

void KMetaDataWidget::Private::updateRowsVisibility()
{
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    setRowVisible(m_typeInfo,
                  (m_visibleDataTypes & KMetaDataWidget::TypeData) &&
                  settings.readEntry("type", true));

    // Cache in m_sizeVisible whether the size should be shown. This
    // is necessary as the size is temporary hidden when the target
    // file item is a directory.
    m_sizeVisible = (m_visibleDataTypes & KMetaDataWidget::SizeData) &&
                      settings.readEntry("size", true);
    bool visible = m_sizeVisible;
    if (visible && (m_fileItems.count() == 1)) {
        // don't show the size information, if one directory is shown
        const KFileItem item = m_fileItems.first();
        visible = !item.isNull() && !item.isDir();
    }
    setRowVisible(m_sizeInfo, visible);

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
    if (m_nepomukActivated) {
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
    if (m_loadMetaDataThread == 0) {
        // The signal finished() has been emitted, but the thread has been marked
        // as invalid in the meantime. Just ignore the signal in this case.
        return;
    }

    if (m_nepomukActivated) {
        Q_ASSERT(m_ratingWidget != 0);
        Q_ASSERT(m_commentWidget != 0);
        Q_ASSERT(m_taggingWidget != 0);
        m_ratingWidget->setRating(m_loadMetaDataThread->rating());
        m_commentWidget->setText(m_loadMetaDataThread->comment());
        m_taggingWidget->setTags(m_loadMetaDataThread->tags());
    }

    // Show the remaining meta information as text. The number
    // of required rows may very. Existing rows are reused to
    // prevent flickering.
    int rowIndex = m_fixedRowCount;
    const QList<KLoadMetaDataThread::Item> items = mergedItems(m_loadMetaDataThread->items());
    foreach (const KLoadMetaDataThread::Item& item, items) {
        const QString itemLabel = item.label;
        QString itemValue = item.value;
        if (item.value.startsWith("<a href=")) {
           // use the text color for the value-links, to create a visual difference
           // to the semantically different links like "Change..."
           const QColor linkColor = q->palette().text().color();
           QString decoration;
           if (m_readOnly) {
               decoration = QString::fromLatin1("text-decoration:none;");
           }
           const QString styleText = QString::fromLatin1("style=\"color:%1;%2\" ")
                                                         .arg(linkColor.name())
                                                         .arg(decoration);
           itemValue.insert(3 /* after "<a "*/, styleText);
        }
        if (rowIndex < m_rows.count()) {
            // adjust texts of the current row
            m_rows[rowIndex].label->setText(itemLabel);
            QLabel* infoValueLabel = qobject_cast<QLabel*>(m_rows[rowIndex].infoWidget);
            Q_ASSERT(infoValueLabel != 0);
            infoValueLabel->setText(itemValue);
        } else {
            // create new row
            QLabel* infoLabel = new QLabel(itemLabel, q);
            QLabel* infoValue = new QLabel(itemValue, q);
            connect(infoValue, SIGNAL(linkActivated(QString)),
                    q, SLOT(slotLinkActivated(QString)));
            addRow(infoLabel, infoValue);
        }
        ++rowIndex;
    }

    // remove rows that are not needed anymore
    for (int i = m_rows.count() - 1; i >= rowIndex; --i) {
        delete m_rows[i].label;
        delete m_rows[i].infoWidget;
        m_rows.pop_back();
    }

    m_files = m_loadMetaDataThread->files();

    m_loadMetaDataThread->deleteLater();
    m_loadMetaDataThread = 0;
#endif

    q->updateGeometry();
}

void KMetaDataWidget::Private::slotRatingChanged(unsigned int rating)
{
#ifdef HAVE_NEPOMUK
    Nepomuk::MassUpdateJob* job =
        Nepomuk::MassUpdateJob::rateResources(m_files.values(), rating);
    startChangeDataJob(job);
#else
    Q_UNUSED(rating);
#endif
}

void KMetaDataWidget::Private::slotTagsChanged(const QList<Nepomuk::Tag>& tags)
{
#ifdef HAVE_NEPOMUK
    m_taggingWidget->setTags(tags);

    Nepomuk::MassUpdateJob* job =
        Nepomuk::MassUpdateJob::tagResources(m_files.values(), tags);
    startChangeDataJob(job);
#else
    Q_UNUSED(tags);
#endif
}

void KMetaDataWidget::Private::slotCommentChanged(const QString& comment)
{
#ifdef HAVE_NEPOMUK
    Nepomuk::MassUpdateJob* job =
        Nepomuk::MassUpdateJob::commentResources(m_files.values(), comment);
    startChangeDataJob(job);
#else
    Q_UNUSED(comment);
#endif
}

void KMetaDataWidget::Private::slotTagActivated(const Nepomuk::Tag& tag)
{
#ifdef HAVE_NEPOMUK
    emit q->urlActivated(tag.resourceUri());
#else
    Q_UNUSED(tag);
#endif
}

void KMetaDataWidget::Private::slotMetaDataUpdateDone()
{
#ifdef HAVE_NEPOMUK
    q->setEnabled(true);
#endif
}

void KMetaDataWidget::Private::slotLinkActivated(const QString& link)
{
    emit q->urlActivated(KUrl(link));
}

#ifdef HAVE_NEPOMUK
void KMetaDataWidget::Private::startChangeDataJob(KJob* job)
{
    connect(job, SIGNAL(result(KJob*)),
            q, SLOT(slotMetaDataUpdateDone()));
    q->setEnabled(false); // no updates during execution
    job->start();
}

QList<KLoadMetaDataThread::Item>
    KMetaDataWidget::Private::mergedItems(const QList<KLoadMetaDataThread::Item>& items)
{
    // TODO: Currently only "width" and "height" are merged as "width x height". If
    // other kind of merges should be done too, a more general approach is required.
    QList<KLoadMetaDataThread::Item> mergedItems;

    KLoadMetaDataThread::Item width;
    KLoadMetaDataThread::Item height;

    foreach (const KLoadMetaDataThread::Item& item, items) {
        if (item.name == QLatin1String("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#width")) {
            width = item;
        } else if (item.name == QLatin1String("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#height")) {
            height = item;
        } else {
            // insert the item sorted by the label
            int pos = 0;
            while ((mergedItems.count() > pos) &&
                   (mergedItems[pos].label < item.label)) {
                ++pos;
            }
            mergedItems.insert(pos, item);
        }
    }

    const bool foundWidth = !width.name.isEmpty();
    const bool foundHeight = !height.name.isEmpty();
    if (foundWidth && !foundHeight) {
        mergedItems.insert(0, width);
    } else if (foundHeight && !foundWidth) {
        mergedItems.insert(0, height);
    } else if (foundWidth && foundHeight) {
        KLoadMetaDataThread::Item size;
        size.label = i18nc("@label image width and height", "Width x Height");
        size.value = width.value + " x " + height.value;
        mergedItems.insert(0, size);
    }

    return mergedItems;
}
#endif

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
    d->m_sizeLabel->setText(i18nc("@label", "Size"));
    if (item.isDir()) {
        d->m_typeInfo->setText(i18nc("@label", "Folder"));
        d->setRowVisible(d->m_sizeInfo, false);
    } else {
        d->m_typeInfo->setText(item.mimeComment());
        d->m_sizeInfo->setText(KIO::convertSize(item.size()));
        d->setRowVisible(d->m_sizeInfo, d->m_sizeVisible);
    }
    d->m_modifiedInfo->setText(KGlobal::locale()->formatDateTime(item.time(KFileItem::ModificationTime), KLocale::FancyLongDate));
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
        d->setRowVisible(d->m_sizeInfo, d->m_sizeVisible);

        quint64 totalSize = 0;
        foreach (const KFileItem& item, items) {
            if (!item.isDir() && !item.isLink()) {
                totalSize += item.size();
            }
        }
        d->m_sizeInfo->setText(KIO::convertSize(totalSize));
    }

#ifdef HAVE_NEPOMUK
    QList<KUrl> urls;
    foreach (const KFileItem& item, items) {
        const KUrl url = item.nepomukUri();
        if (url.isValid()) {
            urls.append(url);
        }
    }

    if (d->m_loadMetaDataThread != 0) {
        disconnect(d->m_loadMetaDataThread, SIGNAL(finished()), this, SLOT(slotLoadingFinished()));
        d->m_loadMetaDataThread->cancelAndDelete();
    }

    d->m_loadMetaDataThread = new KLoadMetaDataThread();
    connect(d->m_loadMetaDataThread, SIGNAL(finished()), this, SLOT(slotLoadingFinished()));
    d->m_loadMetaDataThread->load(urls);
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

void KMetaDataWidget::setReadOnly(bool readOnly)
{
    d->m_readOnly = readOnly;
#ifdef HAVE_NEPOMUK
    // TODO: encapsulate this code as part of a metadata-model for KDE 4.5
    if (d->m_taggingWidget)
        d->m_taggingWidget->setReadOnly(readOnly);
    if (d->m_commentWidget)
        d->m_commentWidget->setReadOnly(readOnly);
#endif
}

bool KMetaDataWidget::isReadOnly() const
{
    return d->m_readOnly;
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

QSize KMetaDataWidget::sizeHint() const
{
    const int fixedWidth = 200;

    int height = d->m_gridLayout->margin() * 2 +
                 d->m_gridLayout->spacing() * (d->m_rows.count() - 1);

    foreach (const Private::Row& row, d->m_rows) {
        if (row.infoWidget != 0) {
            int rowHeight = row.infoWidget->heightForWidth(fixedWidth / 2);
            if (rowHeight <= 0) {
                rowHeight = row.infoWidget->sizeHint().height();
            }
            height += rowHeight;
        }
    }

    return QSize(fixedWidth, height);
}

bool KMetaDataWidget::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        // The adding of rows is not done in the constructor. This allows the
        // client of KMetaDataWidget to set a proper foreground role which
        // will be respected by the rows.

        d->addRow(new QLabel(i18nc("@label file type", "Type"), this), d->m_typeInfo);
        d->addRow(d->m_sizeLabel, d->m_sizeInfo);
        d->addRow(new QLabel(i18nc("@label", "Modified"), this), d->m_modifiedInfo);
        d->addRow(new QLabel(i18nc("@label", "Owner"), this), d->m_ownerInfo);
        d->addRow(new QLabel(i18nc("@label", "Permissions"), this), d->m_permissionsInfo);

    #ifdef HAVE_NEPOMUK
        if (d->m_nepomukActivated) {
            d->addRow(new QLabel(i18nc("@label", "Rating"), this), d->m_ratingWidget);
            d->addRow(new QLabel(i18nc("@label", "Tags"), this), d->m_taggingWidget);
            d->addRow(new QLabel(i18nc("@label", "Comment"), this), d->m_commentWidget);
        }
    #endif

        // The current number of rows represents meta data, that will be shown for
        // all files. Dynamic meta data will be appended after those rows (see
        // slotLoadingFinished()).
        d->m_fixedRowCount = d->m_rows.count();

        d->updateRowsVisibility();
    }

    return QWidget::event(event);
}

#include "kmetadatawidget.moc"
