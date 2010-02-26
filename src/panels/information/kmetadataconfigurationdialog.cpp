/*****************************************************************************
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

#include "kmetadataconfigurationdialog.h"

#include "nfotranslator.h"

#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>
#include "kmetadatawidget.h"
#include <klocale.h>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #define DISABLE_NEPOMUK_LEGACY
    #include <nepomuk/resource.h>
    #include <nepomuk/resourcemanager.h>
    #include <nepomuk/property.h>
    #include <nepomuk/variant.h>
#endif

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

class KMetaDataConfigurationDialog::Private
{
public:
    Private(KMetaDataConfigurationDialog* parent, KMetaDataWidget* metaDataWidget);
    ~Private();

    void init();
    void loadMetaData();
    void addItem(const QUrl& uri);

    int m_visibleDataTypes;
    QLabel* m_descriptionLabel;
    KMetaDataWidget* m_metaDataWidget;
    QListWidget* m_metaDataList;

private:
    KMetaDataConfigurationDialog* const q;
};

KMetaDataConfigurationDialog::Private::Private(KMetaDataConfigurationDialog* parent,
                                               KMetaDataWidget* metaDataWidget) :
    q(parent)
{
    m_visibleDataTypes = 0;
    m_metaDataWidget = metaDataWidget;

    q->setCaption(i18nc("@title:window", "Configure Shown Data"));
    q->setButtons(KDialog::Ok | KDialog::Cancel);
    q->setDefaultButton(KDialog::Ok);

    QWidget* mainWidget = new QWidget(q);
    QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);

    m_descriptionLabel = new QLabel(i18nc("@label::textbox",
                                          "Configure which data should "
                                          "be shown"), q);
    m_descriptionLabel->setWordWrap(true);

    m_metaDataList = new QListWidget(q);
    m_metaDataList->setSelectionMode(QAbstractItemView::NoSelection);

    topLayout->addWidget(m_descriptionLabel);
    topLayout->addWidget(m_metaDataList);

    q->setMainWidget(mainWidget);

    loadMetaData();

    const KConfigGroup dialogConfig(KGlobal::config(), "Nepomuk KMetaDataConfigurationDialog");
    q->restoreDialogSize(dialogConfig);
}

KMetaDataConfigurationDialog::Private::~Private()
{
    KConfigGroup dialogConfig(KGlobal::config(), "Nepomuk KMetaDataConfigurationDialog");
    q->saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

void KMetaDataConfigurationDialog::Private::loadMetaData()
{
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    // Add fixed meta data items where the visibility does not
    // depend on the currently used URL.
    KMetaDataWidget::MetaDataTypes visibleDataTypes = KMetaDataWidget::TypeData |
                                                      KMetaDataWidget::SizeData |
                                                      KMetaDataWidget::ModifiedData |
                                                      KMetaDataWidget::OwnerData |
                                                      KMetaDataWidget::PermissionsData |
                                                      KMetaDataWidget::RatingData |
                                                      KMetaDataWidget::TagsData |
                                                      KMetaDataWidget::CommentData;
    if (m_metaDataWidget != 0) {
        visibleDataTypes = m_metaDataWidget->visibleDataTypes();
    }

    typedef QPair<QString, QString> FixedItem;
    QList<FixedItem> fixedItems;
    if (visibleDataTypes & KMetaDataWidget::TypeData) {
        fixedItems.append(FixedItem("kfileitem#type", i18nc("@item::inlistbox", "Type")));
    }
    if (visibleDataTypes & KMetaDataWidget::SizeData) {
        fixedItems.append(FixedItem("kfileitem#size", i18nc("@item::inlistbox", "Size")));
    }
    if (visibleDataTypes & KMetaDataWidget::ModifiedData) {
        fixedItems.append(FixedItem("kfileitem#modified", i18nc("@item::inlistbox", "Modified")));
    }
    if (visibleDataTypes & KMetaDataWidget::OwnerData) {
        fixedItems.append(FixedItem("kfileitem#owner", i18nc("@item::inlistbox", "Owner")));
    }
    if (visibleDataTypes & KMetaDataWidget::PermissionsData) {
        fixedItems.append(FixedItem("kfileitem#permissions", i18nc("@item::inlistbox", "Permissions")));
    }
#ifdef HAVE_NEPOMUK
    if (Nepomuk::ResourceManager::instance()->init() == 0) {
        if (visibleDataTypes & KMetaDataWidget::RatingData) {
            fixedItems.append(FixedItem("kfileitem#rating", i18nc("@item::inlistbox", "Rating")));
        }
        if (visibleDataTypes & KMetaDataWidget::TagsData) {
            fixedItems.append(FixedItem("kfileitem#tags", i18nc("@item::inlistbox", "Tags")));
        }
        if (visibleDataTypes & KMetaDataWidget::CommentData) {
            fixedItems.append(FixedItem("kfileitem#comment", i18nc("@item::inlistbox", "Comment")));
        }
    }
#endif

    foreach (const FixedItem& fixedItem, fixedItems) {
        const QString key = fixedItem.first;
        const QString label = fixedItem.second;
        QListWidgetItem* item = new QListWidgetItem(label, m_metaDataList);
        item->setData(Qt::UserRole, key);
        const bool show = settings.readEntry(key, true);
        item->setCheckState(show ? Qt::Checked : Qt::Unchecked);
    }

#ifdef HAVE_NEPOMUK
    // Get all meta information labels that are available for
    // the currently shown file item and add them to the list.
    if (m_metaDataWidget == 0) {
        // TODO: in this case all available meta data from the system
        // should be added.
        return;
    }

    const KFileItemList items = m_metaDataWidget->items();
    if (items.count() != 1) {
        // TODO: handle als usecases for more than one item:
        return;
    }

    Nepomuk::Resource res(items.first().nepomukUri());
    QHash<QUrl, Nepomuk::Variant> properties = res.properties();
    if (properties.isEmpty()) {
        // the file is not indexed or Nepomuk is disabled
        KFileMetaInfo metaInfo(items.first().url());
        const QHash<QString, KFileMetaInfoItem> metaInfoItems = metaInfo.items();
        foreach (const KFileMetaInfoItem& metaInfoItem, metaInfoItems) {
            addItem(metaInfoItem.name());
        }

    } else {
        // show meta information provided by Nepomuk
        QHash<QUrl, Nepomuk::Variant>::const_iterator it = properties.constBegin();
        while (it != properties.constEnd()) {
            Nepomuk::Types::Property prop(it.key());
            addItem(prop.uri());
            ++it;
        }
    }
#endif
}

void KMetaDataConfigurationDialog::Private::addItem(const QUrl& uri)
{
    // Meta information provided by Nepomuk that is already
    // available from KFileItem as "fixed item" (see above)
    // should not be shown as second entry.
    static const char* const hiddenProperties[] = {
        "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#comment",         // = fixed item kfileitem#comment
        "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentSize",     // = fixed item kfileitem#size
        "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#lastModified",    // = fixed item kfileitem#modified
        "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#plainTextContent" // hide this property always
        "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#mimeType",        // = fixed item kfileitem#type
        "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#fileName",        // hide this property always
        "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",                          // = fixed item kfileitem#type
        0 // mandatory last entry
    };

    int i = 0;
    const QString key = uri.toString();
    while (hiddenProperties[i] != 0) {
        if (key == QLatin1String(hiddenProperties[i])) {
            // the item is hidden
            return;
        }
        ++i;
    }

    // the item is not hidden, add it to the list
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    const QString label = NfoTranslator::instance().translation(uri);
    QListWidgetItem* item = new QListWidgetItem(label, m_metaDataList);
    item->setData(Qt::UserRole, key);
    const bool show = settings.readEntry(key, true);
    item->setCheckState(show ? Qt::Checked : Qt::Unchecked);
}

KMetaDataConfigurationDialog::KMetaDataConfigurationDialog(QWidget* parent,
                                                         Qt::WFlags flags) :
    KDialog(parent, flags),
    d(new Private(this, 0))
{
}

KMetaDataConfigurationDialog::KMetaDataConfigurationDialog(KMetaDataWidget* metaDataWidget,
                                                         QWidget* parent,
                                                         Qt::WFlags flags) :
    KDialog(parent, flags),
    d(new Private(this, metaDataWidget))
{
}

KMetaDataConfigurationDialog::~KMetaDataConfigurationDialog()
{
    delete d;
}

void KMetaDataConfigurationDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        KConfig config("kmetainformationrc", KConfig::NoGlobals);
        KConfigGroup showGroup = config.group("Show");

        const int count = d->m_metaDataList->count();
        for (int i = 0; i < count; ++i) {
            QListWidgetItem* item = d->m_metaDataList->item(i);
            const bool show = (item->checkState() == Qt::Checked);
            const QString key = item->data(Qt::UserRole).toString();
            showGroup.writeEntry(key, show);
        }

        showGroup.sync();

        if (d->m_metaDataWidget != 0) {
            // trigger an update
            d->m_metaDataWidget->setVisibleDataTypes(d->m_metaDataWidget->visibleDataTypes());
        }
        accept();
    } else {
        KDialog::slotButtonClicked(button);
    }
}

void KMetaDataConfigurationDialog::setDescription(const QString& description)
{
    d->m_descriptionLabel->setText(description);
}

QString KMetaDataConfigurationDialog::description() const
{
    return d->m_descriptionLabel->text();
}

#include "kmetadataconfigurationdialog.moc"
