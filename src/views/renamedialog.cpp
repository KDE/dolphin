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

#include <KLocalizedString>
#include <KJobWidgets>
#include <KIO/CopyJob>
#include <KIO/FileUndoManager>
#include <KJobUiDelegate>

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QMimeDatabase>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <KGuiItem>

RenameDialog::RenameDialog(QWidget *parent, const KFileItemList& items) :
    QDialog(parent),
    m_renameOneItem(false),
    m_newName(),
    m_lineEdit(0),
    m_items(items),
    m_allExtensionsDifferent(true),
    m_spinBox(0)
{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(320, minSize.height()));

    const int itemCount = items.count();
    Q_ASSERT(itemCount >= 1);
    m_renameOneItem = (itemCount == 1);

    setWindowTitle(m_renameOneItem ?
               i18nc("@title:window", "Rename Item") :
               i18nc("@title:window", "Rename Items"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RenameDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RenameDialog::reject);
    m_okButton->setDefault(true);

    KGuiItem::assign(m_okButton, KGuiItem(i18nc("@action:button", "&Rename"), QStringLiteral("dialog-ok-apply")));

    QWidget* page = new QWidget(this);
    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);

    QVBoxLayout* topLayout = new QVBoxLayout(page);

    QLabel* editLabel = 0;
    if (m_renameOneItem) {
        m_newName = items.first().name();
        editLabel = new QLabel(xi18nc("@label:textbox", "Rename the item <filename>%1</filename> to:", m_newName),
                               page);
        editLabel->setTextFormat(Qt::PlainText);
    } else {
        m_newName = i18nc("@info:status", "New name #");
        editLabel = new QLabel(i18ncp("@label:textbox",
                                      "Rename the %1 selected item to:",
                                      "Rename the %1 selected items to:", itemCount),
                               page);
    }

    m_lineEdit = new QLineEdit(page);
    mainLayout->addWidget(m_lineEdit);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &RenameDialog::slotTextChanged);

    int selectionLength = m_newName.length();
    if (m_renameOneItem) {
        const QString fileName = items.first().url().toDisplayString();
        QMimeDatabase db;
        const QString extension = db.suffixForFileName(fileName.toLower());

        // If the current item is a directory, select the whole file name.
        if ((extension.length() > 0) && !items.first().isDir()) {
            // Don't select the extension
            selectionLength -= extension.length() + 1;
        }
    } else {
         // Don't select the # character
        --selectionLength;
    }

    m_lineEdit->setText(m_newName);
    m_lineEdit->setSelection(0, selectionLength);

    topLayout->addWidget(editLabel);
    topLayout->addWidget(m_lineEdit);

    if (!m_renameOneItem) {
        QSet<QString> extensions;
        foreach (const KFileItem& item, m_items) {
            QMimeDatabase db;
            const QString extension = db.suffixForFileName(item.url().toDisplayString().toLower());

            if (extensions.contains(extension)) {
                m_allExtensionsDifferent = false;
                break;
            }

            extensions.insert(extension);
        }

        QLabel* infoLabel = new QLabel(i18nc("@info", "# will be replaced by ascending numbers starting with:"), page);
        mainLayout->addWidget(infoLabel);
        m_spinBox = new QSpinBox(page);
        m_spinBox->setMaximum(10000);
        m_spinBox->setMinimum(0);
        m_spinBox->setSingleStep(1);
        m_spinBox->setValue(1);
        m_spinBox->setDisplayIntegerBase(10);

        QHBoxLayout* horizontalLayout = new QHBoxLayout(page);
        horizontalLayout->setMargin(0);
        horizontalLayout->addWidget(infoLabel);
        horizontalLayout->addWidget(m_spinBox);

        topLayout->addLayout(horizontalLayout);
    }
}

RenameDialog::~RenameDialog()
{
}

void RenameDialog::renameItem(const KFileItem &item, const QString& newName)
{
    const QUrl oldUrl = item.url();
    QUrl newUrl = oldUrl.adjusted(QUrl::RemoveFilename);
    newUrl.setPath(newUrl.path() + KIO::encodeFileName(newName));

    QWidget* widget = parentWidget();
    if (!widget) {
        widget = this;
    }

    KIO::Job * job = KIO::moveAs(oldUrl, newUrl, KIO::HideProgressInfo);
    KJobWidgets::setWindow(job, widget);
    KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Rename, {oldUrl}, newUrl, job);

    if (!job->error()) {
        m_renamedItems << newUrl;
    }

    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
}

void RenameDialog::slotAccepted()
{
    m_newName = m_lineEdit->text();

    if (m_renameOneItem) {
        Q_ASSERT(m_items.count() == 1);
        renameItem(m_items.first(), m_newName);
    } else {
        renameItems();
    }
    accept();
}

void RenameDialog::slotTextChanged(const QString& newName)
{
    bool enable = !newName.isEmpty() && (newName != QLatin1String("..")) && (newName != QLatin1String("."));
    if (enable && !m_renameOneItem) {
        const int count = newName.count(QLatin1Char('#'));
        if (count == 0) {
            // Renaming multiple files without '#' will only work if all extensions are different.
            enable = m_allExtensionsDifferent;
        } else {
            // Assure that the new name contains exactly one # (or a connected sequence of #'s)
            const int first = newName.indexOf(QLatin1Char('#'));
            const int last = newName.lastIndexOf(QLatin1Char('#'));
            enable = (last - first + 1 == count);
        }
    }
    m_okButton->setEnabled(enable);
}

void RenameDialog::showEvent(QShowEvent* event)
{
    m_lineEdit->setFocus();

    QDialog::showEvent(event);
}

void RenameDialog::renameItems()
{
    // Iterate through all items and rename them...
    int index = m_spinBox->value();
    foreach (const KFileItem& item, m_items) {
        QString newName = indexedName(m_newName, index, QLatin1Char('#'));
        ++index;

        const QUrl oldUrl = item.url();
        QMimeDatabase db;
        const QString extension = db.suffixForFileName(oldUrl.path().toLower());
        if (!extension.isEmpty()) {
            newName.append(QLatin1Char('.'));
            newName.append(extension);
        }

        if (oldUrl.fileName() != newName) {
            renameItem(item, newName);
        }
    }

    if (!m_items.empty()) {
        emit renamingFinished(m_renamedItems);
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

