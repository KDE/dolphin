/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz (peter.penz@gmx.at)             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "renamedialog.h"

#include <KLineEdit>
#include <KLocale>
#include <konq_operations.h>
#include <KStringHandler>

#include <QLabel>
#include <QVBoxLayout>

/**
 * Helper function for sorting items with qSort() in
 * DolphinView::renameSelectedItems().
 */
bool lessThan(const KFileItem& item1, const KFileItem& item2)
{
    return KStringHandler::naturalCompare(item1.name(), item2.name()) < 0;
}

RenameDialog::RenameDialog(QWidget *parent, const KFileItemList& items) :
    KDialog(parent),
    m_renameOneItem(false),
    m_newName(),
    m_lineEdit(0),
    m_items(items)
{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(320, minSize.height()));

    const int itemCount = items.count();
    Q_ASSERT(itemCount >= 1);
    m_renameOneItem = (itemCount == 1);

    setCaption(m_renameOneItem ?
               i18nc("@title:window", "Rename Item") :
               i18nc("@title:window", "Rename Items"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    setButtonGuiItem(Ok, KGuiItem(i18nc("@action:button", "&Rename"), "dialog-ok-apply"));

    QWidget* page = new QWidget(this);
    setMainWidget(page);

    QVBoxLayout* topLayout = new QVBoxLayout(page);

    QLabel* editLabel = 0;
    if (m_renameOneItem) {
        m_newName = items.first().name();
        editLabel = new QLabel(i18nc("@label:textbox", "Rename the item <filename>%1</filename> to:", m_newName),
                               page);
    } else {
        m_newName = i18nc("@info:status", "New name #");
        editLabel = new QLabel(i18ncp("@label:textbox",
                                      "Rename the %1 selected item to:",
                                      "Rename the %1 selected items to:", itemCount),
                               page);
    }

    m_lineEdit = new KLineEdit(page);
    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotTextChanged(QString)));

    QString fileName = items[0].url().prettyUrl();
    QString extension = KMimeType::extractKnownExtension(fileName.toLower());
    if (!extension.isEmpty()) {
        extension.insert(0, '.');
        // The first item seems to have a extension (e. g. '.jpg' or '.txt'). Now
        // check whether all other URLs have the same extension. If this is the
        // case, add this extension to the name suggestion.
        for (int i = 1; i < itemCount; ++i) {
            fileName = items[i].url().prettyUrl().toLower();
            if (!fileName.endsWith(extension)) {
                // at least one item does not have the same extension
                extension.truncate(0);
                break;
            }
        }
    }

    int selectionLength = m_newName.length();
    if (!m_renameOneItem) {
        --selectionLength; // don't select the # character
    }

    const int extensionLength = extension.length();
    if (extensionLength > 0) {
        if (m_renameOneItem) {
            selectionLength -= extensionLength;
        } else {
            m_newName.append(extension);
        }
    }

    m_lineEdit->setText(m_newName);
    m_lineEdit->setSelection(0, selectionLength);
    m_lineEdit->setFocus();

    topLayout->addWidget(editLabel);
    topLayout->addWidget(m_lineEdit);

    if (!m_renameOneItem) {
        QLabel* infoLabel = new QLabel(i18nc("@info", "(# will be replaced by ascending numbers)"), page);
        topLayout->addWidget(infoLabel);
    }
}

RenameDialog::~RenameDialog()
{
}

void RenameDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        m_newName = m_lineEdit->text();

        if (m_renameOneItem) {
            Q_ASSERT(m_items.count() == 1);
            const KUrl oldUrl = m_items.first().url();
            KUrl newUrl = oldUrl;
            newUrl.setFileName(KIO::encodeFileName(m_newName));
            KonqOperations::rename(this, oldUrl, newUrl);
        } else {
            renameItems();
        }
    }

    KDialog::slotButtonClicked(button);
}

void RenameDialog::slotTextChanged(const QString& newName)
{
    bool enable = !newName.isEmpty() && (newName != QLatin1String("..")) && (newName != QLatin1String("."));
    if (enable) {
        if (m_renameOneItem) {
            enable = enable && (newName != m_newName);
        } else {
            // Assure that the new name contains exactly one # (or a connected sequence of #'s)
            const int minSplitCount = 1;
            int maxSplitCount = 2;
            if (newName.startsWith(QLatin1Char('#'))) {
                --maxSplitCount;
            }
            if (newName.endsWith(QLatin1Char('#'))) {
                --maxSplitCount;
            }
            const int splitCount = newName.split(QLatin1Char('#'), QString::SkipEmptyParts).count();
            enable = enable && (splitCount >= minSplitCount) && (splitCount <= maxSplitCount);
        }
    }
    enableButtonOk(enable);
}

void RenameDialog::renameItems()
{
    // Currently the items are sorted by the selection order, resort
    // them by the filename. This assures that the new sort order is similar to
    // the current filename sort order.
    qSort(m_items.begin(), m_items.end(), lessThan);

    // Iterate through all items and rename them...
    int index = 1;
    foreach (const KFileItem& item, m_items) {
        const QString newName = indexedName(m_newName, index, QLatin1Char('#'));
        ++index;

        const KUrl oldUrl = item.url();
        if (oldUrl.fileName() != newName) {
            KUrl newUrl = oldUrl;
            newUrl.setFileName(KIO::encodeFileName(newName));
            KonqOperations::rename(this, oldUrl, newUrl);
        }
    }
}

QString RenameDialog::indexedName(const QString& name, int index, const QChar& indexPlaceHolder)
{
    QString newName = name;

    QString indexString = QString::number(index);

    // Insert leading zeros if necessary
    const int minIndexLength = name.count(indexPlaceHolder);
    while (indexString.length() < minIndexLength) {
        indexString.prepend(QLatin1Char('0'));
    }

    // Replace the index placeholders by the indexString
    const int placeHolderStart = newName.indexOf(indexPlaceHolder);
    newName.replace(placeHolderStart, minIndexLength, indexString);

    return newName;
}

#include "renamedialog.moc"
