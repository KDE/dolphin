/***************************************************************************
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

#include "metadataconfigurationdialog.h"

#include "metadatawidget.h"

#include <klocale.h>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #define DISABLE_NEPOMUK_LEGACY
    #include <Nepomuk/Resource>
    #include <Nepomuk/Types/Property>
    #include <Nepomuk/Variant>
#endif

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

class MetaDataConfigurationDialog::Private
{
public:
    Private(MetaDataConfigurationDialog* parent);
    ~Private();

    void loadMetaData();

    KUrl m_url;
    QListWidget* m_metaDataList;

private:
    MetaDataConfigurationDialog* const q;
};

MetaDataConfigurationDialog::Private::Private(MetaDataConfigurationDialog* parent) :
    q(parent)
{
}

MetaDataConfigurationDialog::Private::~Private()
{
}

void MetaDataConfigurationDialog::Private::loadMetaData()
{
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    // Add fixed meta data items where the visibility does not
    // depend on the currently used URL.
    typedef QPair<QString, QString> FixedItem;
    QList<FixedItem> fixedItems;
    fixedItems.append(FixedItem("type",        i18nc("@item::inlistbox", "Type")));
    fixedItems.append(FixedItem("size",        i18nc("@item::inlistbox", "Size")));
    fixedItems.append(FixedItem("modified",    i18nc("@item::inlistbox", "Modified")));
    fixedItems.append(FixedItem("owner",       i18nc("@item::inlistbox", "Owner")));
    fixedItems.append(FixedItem("permissions", i18nc("@item::inlistbox", "Permissions")));
#ifdef HAVE_NEPOMUK
    fixedItems.append(FixedItem("rating",      i18nc("@item::inlistbox", "Rating")));
    fixedItems.append(FixedItem("tags",        i18nc("@item::inlistbox", "Tags")));
    fixedItems.append(FixedItem("comment",     i18nc("@item::inlistbox", "Comment")));
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
    if (m_url.isEmpty()) {
        // TODO: in this case all available meta data from the system
        // should be added.
        return;
    }
    Nepomuk::Resource res(m_url);
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
            // const QString label = tunedLabel(prop.label());
            const QString label = prop.label() + " --- " + key;
            QListWidgetItem* item = new QListWidgetItem(label, m_metaDataList);
            item->setData(Qt::UserRole, key);
            const bool show = settings.readEntry(key, true);
            item->setCheckState(show ? Qt::Checked : Qt::Unchecked);
        }

        ++it;
    }
#endif
}

MetaDataConfigurationDialog::MetaDataConfigurationDialog(const KUrl& url,
                                                         QWidget* parent,
                                                         Qt::WFlags flags) :
    KDialog(parent, flags),
    d(new Private(this))
{
    d->m_url = url;

    setCaption(i18nc("@title:window", "Configure Shown Data"));
    setButtons(KDialog::Ok | KDialog::Cancel);
    setDefaultButton(KDialog::Ok);

    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);

    QLabel* label = new QLabel(i18nc("@label:textbox",
                                     "Configure which data should "
                                     "be shown."), this);

    d->m_metaDataList = new QListWidget(this);
    d->m_metaDataList->setSelectionMode(QAbstractItemView::NoSelection);

    topLayout->addWidget(label);
    topLayout->addWidget(d->m_metaDataList);

    setMainWidget(mainWidget);

    d->loadMetaData();

    const KConfigGroup dialogConfig(KGlobal::config(), "Nepomuk MetaDataConfigurationDialog");
    restoreDialogSize(dialogConfig);
}

MetaDataConfigurationDialog::~MetaDataConfigurationDialog()
{
    KConfigGroup dialogConfig(KGlobal::config(), "Nepomuk MetaDataConfigurationDialog");
    saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

void MetaDataConfigurationDialog::slotButtonClicked(int button)
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

        accept();
    } else {
        KDialog::slotButtonClicked(button);
    }
}

#include "metadataconfigurationdialog.moc"
