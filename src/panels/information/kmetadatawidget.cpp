/*****************************************************************************
 * Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                     *
 * Copyright (C) 2009-2010 by Peter Penz <peter.penz@gmx.at>                 *
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
#include "kmetadatamodel.h"
#include "knfotranslator_p.h"

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
    #include "ktaggingwidget_p.h"

    #include <nepomuk/kratingwidget.h>
    #include <nepomuk/resource.h>
    #include <nepomuk/resourcemanager.h>
    #include <nepomuk/property.h>
    #include <nepomuk/tag.h>
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
        QLabel* defaultValueWidget;
        QWidget* customValueWidget;
    };

    Private(KMetaDataWidget* parent);
    ~Private();

    void addRow(QLabel* label, QLabel* valueWidget);
    void setCustomValueWidget(int rowIndex, QWidget* valueWidget);
    void setRowVisible(QWidget* valueWidget, bool visible);

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

    QList<KUrl> sortedKeys(const QHash<KUrl, Nepomuk::Variant>& data) const;
    QList<Nepomuk::Resource> resourceList() const;
#endif

    bool m_sizeVisible;
    bool m_readOnly;
    bool m_nepomukActivated;
    int m_fixedRowCount;
    MetaDataTypes m_visibleDataTypes;
    QList<KFileItem> m_fileItems;
    QList<Row> m_rows;

    KMetaDataModel* m_model;

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
    m_model(0),
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
        m_ratingWidget->setVisible(false);

        m_taggingWidget = new KTaggingWidget(parent);
        connect(m_taggingWidget, SIGNAL(tagsChanged(const QList<Nepomuk::Tag>&)),
                q, SLOT(slotTagsChanged(const QList<Nepomuk::Tag>&)));
        connect(m_taggingWidget, SIGNAL(tagActivated(const Nepomuk::Tag&)),
                q, SLOT(slotTagActivated(const Nepomuk::Tag&)));
        m_taggingWidget->setVisible(false);

        m_commentWidget = new KCommentWidget(parent);
        connect(m_commentWidget, SIGNAL(commentChanged(const QString&)),
                q, SLOT(slotCommentChanged(const QString&)));
        m_commentWidget->setVisible(false);
    }
#endif

    initMetaInfoSettings();
}

KMetaDataWidget::Private::~Private()
{
}

void KMetaDataWidget::Private::addRow(QLabel* label, QLabel* valueWidget)
{
    Row row;
    row.label = label;
    row.defaultValueWidget = valueWidget;
    row.customValueWidget = 0;
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

    valueWidget->setForegroundRole(role);
    valueWidget->setFont(smallFont);
    valueWidget->setWordWrap(true);
    valueWidget->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // add the row to grid layout
    const int rowIndex = m_rows.count() - 1;
    m_gridLayout->addWidget(label, rowIndex, 0, Qt::AlignRight);
    const int spacerWidth = QFontMetrics(smallFont).size(Qt::TextSingleLine, " ").width();
    m_gridLayout->addItem(new QSpacerItem(spacerWidth, 1), rowIndex, 1);
    m_gridLayout->addWidget(valueWidget, rowIndex, 2, Qt::AlignLeft);
}

void KMetaDataWidget::Private::setCustomValueWidget(int rowIndex, QWidget* valueWidget)
{
    Row& row = m_rows[rowIndex];

    if (valueWidget == 0) {
        // remove current custom value widget from the grid and replace it
        // by the default value widget
        if (row.customValueWidget != 0) {
            row.customValueWidget->setVisible(false);
            m_gridLayout->removeWidget(row.customValueWidget);
        }
        m_gridLayout->addWidget(row.defaultValueWidget, rowIndex, 2, Qt::AlignLeft);
        row.defaultValueWidget->setVisible(true);
    } else {
        // remove the default value widget from the grid and replace it
        // by the custom value widget
        row.defaultValueWidget->setVisible(false);
        m_gridLayout->removeWidget(row.defaultValueWidget);
        m_gridLayout->addWidget(valueWidget, rowIndex, 2, Qt::AlignLeft);
        valueWidget->setVisible(true);
    }

    row.customValueWidget = valueWidget;
}

void KMetaDataWidget::Private::setRowVisible(QWidget* valueWidget, bool visible)
{
    foreach (const Row& row, m_rows) {
        const bool found = (row.defaultValueWidget == valueWidget) ||
                           (row.customValueWidget == valueWidget);
        if (found) {
            row.label->setVisible(visible);
            if (row.customValueWidget != 0) {
                row.customValueWidget->setVisible(visible);
                row.defaultValueWidget->setVisible(false);
            } else {
                row.defaultValueWidget->setVisible(visible);
            }
            return;
        }
    }
}

void KMetaDataWidget::Private::initMetaInfoSettings()
{
    const int currentVersion = 3; // increase version, if the blacklist of disabled
                                  // properties should be updated

    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    if (config.group("Misc").readEntry("version", 0) < currentVersion) {
        // The resource file is read the first time. Assure
        // that some meta information is disabled per default.

        // clear old info
        config.deleteGroup("Show");
        KConfigGroup settings = config.group("Show");

        static const char* const disabledProperties[] = {
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#comment",
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentSize",
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#depends",
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#isPartOf",
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#lastModified",
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#mimeType",
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#plainTextContent",
            "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#url",
            "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#averageBitrate",
            "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#channels",
            "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#fileName",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#apertureValue",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#exposureBiasValue",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#exposureTime",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#flash",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#focalLength",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#focalLengthIn35mmFilm",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#isoSpeedRatings",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#make",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#meteringMode",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#model",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#orientation",
            "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#whiteBalance",
            "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#description",
            "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#hasTag",
            "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#lastModified",
            "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#numericRating",
            "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
            "kfileitem#owner",
            "kfileitem#permissions",
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
                  settings.readEntry("kfileitem#type", true));

    // Cache in m_sizeVisible whether the size should be shown. This
    // is necessary as the size is temporary hidden when the target
    // file item is a directory.
    m_sizeVisible = (m_visibleDataTypes & KMetaDataWidget::SizeData) &&
                      settings.readEntry("kfileitem#size", true);
    bool visible = m_sizeVisible;
    if (visible && (m_fileItems.count() == 1)) {
        // don't show the size information, if one directory is shown
        const KFileItem item = m_fileItems.first();
        visible = !item.isNull() && !item.isDir();
    }
    setRowVisible(m_sizeInfo, visible);

    setRowVisible(m_modifiedInfo,
                  (m_visibleDataTypes & KMetaDataWidget::ModifiedData) &&
                  settings.readEntry("kfileitem#modified", true));

    setRowVisible(m_ownerInfo,
                  (m_visibleDataTypes & KMetaDataWidget::OwnerData) &&
                  settings.readEntry("kfileitem#owner", true));

    setRowVisible(m_permissionsInfo,
                  (m_visibleDataTypes & KMetaDataWidget::PermissionsData) &&
                  settings.readEntry("kfileitem#permissions", true));

#ifdef HAVE_NEPOMUK
    if (m_nepomukActivated) {
        setRowVisible(m_ratingWidget,
                      (m_visibleDataTypes & KMetaDataWidget::RatingData) &&
                      settings.readEntry("kfileitem#rating", true));

        setRowVisible(m_taggingWidget,
                      (m_visibleDataTypes & KMetaDataWidget::TagsData) &&
                      settings.readEntry("kfileitem#tags", true));

        setRowVisible(m_commentWidget,
                      (m_visibleDataTypes & KMetaDataWidget::CommentData) &&
                      settings.readEntry("kfileitem#comment", true));
    }
#endif
}

void KMetaDataWidget::Private::slotLoadingFinished()
{
#ifdef HAVE_NEPOMUK
    // Show the remaining meta information as text. The number
    // of required rows may very. Existing rows are reused to
    // prevent flickering and to increase the performance.
    int rowIndex = m_fixedRowCount;

    const QHash<KUrl, Nepomuk::Variant> data = m_model->data();
    const QList<KUrl> keys = sortedKeys(data);

    foreach (const KUrl& key, keys) {
        const Nepomuk::Variant value = data[key];
        const QString itemLabel = q->label(key);

        const bool valueApplied = q->setValue(key, value);
        if (rowIndex >= m_rows.count()) {
            // a new row must get created
            QLabel* label = new QLabel(itemLabel, q);
            QLabel* valueWidget = new QLabel(q);
            connect(valueWidget, SIGNAL(linkActivated(QString)),
                    q, SLOT(slotLinkActivated(QString)));
            addRow(label, valueWidget);
        }

        Q_ASSERT(m_rows[rowIndex].label != 0);
        Q_ASSERT(m_rows[rowIndex].defaultValueWidget != 0);

        // set label
        m_rows[rowIndex].label->setText(itemLabel);

        // set value
        if (valueApplied) {
            setCustomValueWidget(rowIndex, q->valueWidget(key));
        } else {
            QLabel* valueWidget = m_rows[rowIndex].defaultValueWidget;
            valueWidget->setText(value.toString());
            setCustomValueWidget(rowIndex, 0);
        }
        ++rowIndex;
    }

    // remove rows that are not needed anymore
    for (int i = m_rows.count() - 1; i >= rowIndex; --i) {
        delete m_rows[i].label;
        delete m_rows[i].defaultValueWidget;
        m_rows.pop_back();
    }
#endif

    q->updateGeometry();
}

void KMetaDataWidget::Private::slotRatingChanged(unsigned int rating)
{
#ifdef HAVE_NEPOMUK
    Nepomuk::MassUpdateJob* job =
        Nepomuk::MassUpdateJob::rateResources(resourceList(), rating);
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
        Nepomuk::MassUpdateJob::tagResources(resourceList(), tags);
    startChangeDataJob(job);
#else
    Q_UNUSED(tags);
#endif
}

void KMetaDataWidget::Private::slotCommentChanged(const QString& comment)
{
#ifdef HAVE_NEPOMUK
    Nepomuk::MassUpdateJob* job =
        Nepomuk::MassUpdateJob::commentResources(resourceList(), comment);
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

QList<KUrl> KMetaDataWidget::Private::sortedKeys(const QHash<KUrl, Nepomuk::Variant>& data) const
{
    // Create a map, where the translated label prefixed with the
    // sort priority acts as key. The data of each entry is the URI
    // of the data. By this the all URIs are sorted by the sort priority
    // and sub sorted by the translated labels.
    QMap<QString, KUrl> map;
    QHash<KUrl, Nepomuk::Variant>::const_iterator hashIt = data.constBegin();
    while (hashIt != data.constEnd()) {
        const KUrl uri = hashIt.key();

        QString key = q->model()->group(uri);
        key += q->label(uri);

        map.insert(key, uri);
        ++hashIt;
    }

    // Apply the URIs from the map to the list that will get returned.
    // The list will then be alphabetically ordered by the translated labels of the URIs.
    QList<KUrl> list;
    QMap<QString, KUrl>::const_iterator mapIt = map.constBegin();
    while (mapIt != map.constEnd()) {
        list.append(mapIt.value());
        ++mapIt;
    }

    return list;
}

QList<Nepomuk::Resource> KMetaDataWidget::Private::resourceList() const
{
    QList<Nepomuk::Resource> list;
    foreach (const KFileItem& item, m_fileItems) {
        const KUrl url = item.url();
        list.append(Nepomuk::Resource(url));
    }
    return list;
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
    if (d->m_model != 0) {
        d->m_model->setItems(items);
    }

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

void KMetaDataWidget::setModel(KMetaDataModel* model)
{
    if (d->m_model != 0) {
        disconnect(d->m_model, SIGNAL(loadingFinished()));
    }
    d->m_model = model;
    connect(d->m_model, SIGNAL(loadingFinished()), this, SLOT(slotLoadingFinished()));
}

KMetaDataModel* KMetaDataWidget::model() const
{
    return d->m_model;
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
        QWidget* valueWidget = row.defaultValueWidget;
        if (valueWidget != 0) {
            if (row.customValueWidget != 0) {
                valueWidget = row.customValueWidget;
            }

            int rowHeight = valueWidget->heightForWidth(fixedWidth / 2);
            if (rowHeight <= 0) {
                rowHeight = valueWidget->sizeHint().height();
            }
            height += rowHeight;
        }
    }

    return QSize(fixedWidth, height);
}

#ifdef HAVE_NEPOMUK
QString KMetaDataWidget::label(const KUrl& metaDataUri) const
{
    QString label;
    const QString uri = metaDataUri.url();
    if (uri == QLatin1String("kfileitem#rating")) {
        label = i18nc("@label", "Rating");
    } else if (uri == QLatin1String("kfileitem#tags")) {
        label = i18nc("@label", "Tags");
    } else if (uri == QLatin1String("kfileitem#comment")) {
        label = i18nc("@label", "Comment");
    } else {
        label = KNfoTranslator::instance().translation(metaDataUri);
    }

    return label;
}

QWidget* KMetaDataWidget::valueWidget(const KUrl& metaDataUri) const
{
    QWidget* widget = 0;

    if (d->m_nepomukActivated) {
        const QString uri = metaDataUri.url();
        if (uri == QLatin1String("kfileitem#rating")) {
            widget = d->m_ratingWidget;
        } else if (uri == QLatin1String("kfileitem#tags")) {
            widget = d->m_taggingWidget;
        } else if (uri == QLatin1String("kfileitem#comment")) {
            widget = d->m_commentWidget;
        }
    }

    return widget;
}

bool KMetaDataWidget::setValue(const KUrl& metaDataUri, const Nepomuk::Variant& value)
{
    if (d->m_nepomukActivated) {
        QWidget* widget = valueWidget(metaDataUri);
        if (widget == d->m_ratingWidget) {
            d->m_ratingWidget->setRating(value.toInt());
            return true;
        }

        if (widget == d->m_taggingWidget) {
            QList<Nepomuk::Variant> variants = value.toVariantList();
            QList<Nepomuk::Tag> tags;
            foreach (const Nepomuk::Variant& variant, variants) {
                const Nepomuk::Resource resource = variant.toResource();
                tags.append(static_cast<Nepomuk::Tag>(resource));
            }
            d->m_taggingWidget->setTags(tags);
            return true;
        }

        if (widget == d->m_commentWidget) {
            d->m_commentWidget->setText(value.toString());
            return true;
        }
    }
    return false;
}
#endif

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

        // The current number of rows represents meta data, that will be shown for
        // all files. Dynamic meta data will be appended after those rows (see
        // slotLoadingFinished()).
        d->m_fixedRowCount = d->m_rows.count();

        d->updateRowsVisibility();
    }

    return QWidget::event(event);
}

#include "kmetadatawidget.moc"
