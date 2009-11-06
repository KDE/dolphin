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
    QString tunedLabel(const QString& label) const;

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
                                          "Configure which data should"
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
        fixedItems.append(FixedItem("type", i18nc("@item::inlistbox", "Type")));
    }
    if (visibleDataTypes & KMetaDataWidget::SizeData) {
        fixedItems.append(FixedItem("size", i18nc("@item::inlistbox", "Size")));
    }
    if (visibleDataTypes & KMetaDataWidget::ModifiedData) {
        fixedItems.append(FixedItem("modified", i18nc("@item::inlistbox", "Modified")));
    }
    if (visibleDataTypes & KMetaDataWidget::OwnerData) {
        fixedItems.append(FixedItem("owner", i18nc("@item::inlistbox", "Owner")));
    }
    if (visibleDataTypes & KMetaDataWidget::PermissionsData) {
        fixedItems.append(FixedItem("permissions", i18nc("@item::inlistbox", "Permissions")));
    }
#ifdef HAVE_NEPOMUK
    if (Nepomuk::ResourceManager::instance()->init() == 0) {
        if (visibleDataTypes & KMetaDataWidget::RatingData) {
            fixedItems.append(FixedItem("rating", i18nc("@item::inlistbox", "Rating")));
        }
        if (visibleDataTypes & KMetaDataWidget::TagsData) {
            fixedItems.append(FixedItem("tags", i18nc("@item::inlistbox", "Tags")));
        }
        if (visibleDataTypes & KMetaDataWidget::CommentData) {
            fixedItems.append(FixedItem("comment", i18nc("@item::inlistbox", "Comment")));
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
    if (Nepomuk::ResourceManager::instance()->init() != 0) {
        return;
    }

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
    QHash<QUrl, Nepomuk::Variant>::const_iterator it = properties.constBegin();
    while (it != properties.constEnd()) {
        Nepomuk::Types::Property prop(it.key());
        const QString key = prop.name();

        // Meta information provided by Nepomuk that is already
        // available from KFileItem as "fixed item" (see above)
        // should not be shown as second entry.
        static const char* hiddenProperties[] = {
            "contentSize",   // = fixed item "size"
            "description",   // = fixed item "comment"
            "fileExtension", // ~ fixed item "type"
            "hasTag",        // = fixed item "tags"
            "name",          // not shown as part of the meta data widget
            "lastModified",  // = fixed item "modified"
            "size",          // = fixed item "size"
            "mimeType",      // = fixed item "type"
            "numericRating", // = fixed item "rating"
            0 // mandatory last entry
        };
        bool skip = false;
        int i = 0;
        while (hiddenProperties[i] != 0) {
            if (key == hiddenProperties[i]) {
                skip = true;
                break;
            }
            ++i;
        }

        if (!skip) {
            // TODO #1: use Nepomuk::formatValue(res, prop) if available
            // instead of it.value().toString()
            // TODO #2: using tunedLabel() is a workaround for KDE 4.3 (4.4?) until
            // we get translated labels
            const QString label = tunedLabel(prop.label());
            QListWidgetItem* item = new QListWidgetItem(label, m_metaDataList);
            item->setData(Qt::UserRole, key);
            const bool show = settings.readEntry(key, true);
            item->setCheckState(show ? Qt::Checked : Qt::Unchecked);
        }

        ++it;
    }
#endif
}

QString KMetaDataConfigurationDialog::Private::tunedLabel(const QString& label) const
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
    return tunedLabel;
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
