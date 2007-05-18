/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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

#include <klineedit.h>
#include <klocale.h>

#include <QtGui/QLabel>
#include <QtGui/QBoxLayout>

RenameDialog::RenameDialog(const KUrl::List& items) :
    KDialog(),
    m_renameOneItem(false)
{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(320, minSize.height()));

    const int itemCount = items.count();
    Q_ASSERT(itemCount >= 1);
    m_renameOneItem = (itemCount == 1);

    setCaption(m_renameOneItem ? i18n("Rename Item") : i18n("Rename Items"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    setButtonGuiItem(Ok, KGuiItem(i18n("Rename"), "dialog-apply"));

    QWidget* page = new QWidget(this);
    setMainWidget(page);

    QVBoxLayout* topLayout = new QVBoxLayout(page);
    topLayout->setMargin(KDialog::marginHint());

    QLabel* editLabel = 0;
    if (m_renameOneItem) {
        const KUrl& url = items.first();
        m_newName = url.fileName();
        editLabel = new QLabel(i18n("Rename the item '%1' to:", m_newName),
                               page);
    } else {
        m_newName = i18n("New name #");
        editLabel = new QLabel(i18n("Rename the %1 selected items to:", itemCount),
                               page);
    }

    m_lineEdit = new KLineEdit(page);
    QString extension = extensionString(items[0].prettyUrl());
    if (extension.length() > 0) {
        // The first item seems to have a extension (e. g. '.jpg' or '.txt'). Now
        // check whether all other items have the same extension. If this is the
        // case, add this extension to the name suggestion.
        for (int i = 1; i < itemCount; ++i) {
            if (!items[i].prettyUrl().contains(extension)) {
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
        QLabel* infoLabel = new QLabel(i18n("(# will be replaced by ascending numbers)"), page);
        topLayout->addWidget(infoLabel);
    }
}

RenameDialog::~RenameDialog()
{}

void RenameDialog::slotButtonClicked(int button)
{
    if (button == Ok) {
        m_newName = m_lineEdit->text();
        if (m_newName.isEmpty()) {
            m_errorString = i18n("The new name is empty. A name with at least one character must be entered.");
        } else if (!m_renameOneItem && m_newName.contains('#') != 1) {
            m_newName.truncate(0);
            m_errorString = i18n("The name must contain exactly one # character.");
        }
    }

    KDialog::slotButtonClicked(button);
}

QString RenameDialog::extensionString(const QString& name)
{
    QString extension;
    bool foundExtension = false;  // true if at least one valid file extension
                                  // like "gif", "txt", ... has been found

    QStringList strings = name.split('.');
    const int size = strings.size();
    for (int i = 1; i < size; ++i) {
        const QString& str = strings.at(i);
        if (!foundExtension) {
            // Sub strings like "9", "12", "99", ... which contain only
            // numbers don't count as extension. Usually they are version
            // numbers like in "cmake-2.4.5".
            bool ok = false;
            str.toInt(&ok);
            foundExtension = !ok;
        }
        if (foundExtension) {
            extension += '.' + str;
        }
    }

    return extension;
}

#include "renamedialog.moc"
