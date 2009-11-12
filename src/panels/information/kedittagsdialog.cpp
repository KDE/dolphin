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

#include "kedittagsdialog_p.h"

#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

KEditTagsDialog::KEditTagsDialog(const QList<Nepomuk::Tag>& tags,
                                 QWidget* parent,
                                 Qt::WFlags flags) :
    KDialog(parent, flags),
    m_tags(tags),
    m_tagsList(0),
    m_newTagItem(0),
    m_deleteCandidate(0),
    m_newTagEdit(0),
    m_deleteButtonTimer(0)
{

    const QString caption = (tags.count() > 0) ?
                            i18nc("@title:window", "Change Tags") :
                            i18nc("@title:window", "Add Tags");
    setCaption(caption);
    setButtons(KDialog::Ok | KDialog::Cancel);
    setDefaultButton(KDialog::Ok);

    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);

    QLabel* label = new QLabel(i18nc("@label:textbox",
                                     "Configure which tags should "
                                     "be applied."), this);

    m_tagsList = new QListWidget(this);
    m_tagsList->setMouseTracking(true);
    m_tagsList->setSortingEnabled(true);
    m_tagsList->setSelectionMode(QAbstractItemView::NoSelection);
    m_tagsList->installEventFilter(this);
    connect(m_tagsList, SIGNAL(itemEntered(QListWidgetItem*)),
            this, SLOT(slotItemEntered(QListWidgetItem*)));
    connect(m_tagsList, SIGNAL(itemEntered(QListWidgetItem*)),
            this, SLOT(slotItemEntered(QListWidgetItem*)));

    QLabel* newTagLabel = new QLabel(i18nc("@label", "Create new tag:"));
    m_newTagEdit = new KLineEdit(this);
    m_newTagEdit->setClearButtonShown(true);
    connect(m_newTagEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(slotTextEdited(const QString&)));

    QHBoxLayout* newTagLayout = new QHBoxLayout();
    newTagLayout->addWidget(newTagLabel);
    newTagLayout->addWidget(m_newTagEdit, 1);

    topLayout->addWidget(label);
    topLayout->addWidget(m_tagsList);
    topLayout->addLayout(newTagLayout);

    setMainWidget(mainWidget);

    loadTags();

    // create the delete button, which is shown when
    // hovering the items
    m_deleteButton = new QPushButton(m_tagsList->viewport());
    m_deleteButton->setIcon(KIcon("edit-delete"));
    m_deleteButton->setToolTip(i18nc("@info", "Delete tag"));
    m_deleteButton->hide();
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(deleteTag()));

    m_deleteButtonTimer = new QTimer(this);
    m_deleteButtonTimer->setSingleShot(true);
    m_deleteButtonTimer->setInterval(500);
    connect(m_deleteButtonTimer, SIGNAL(timeout()), this, SLOT(showDeleteButton()));
}

KEditTagsDialog::~KEditTagsDialog()
{
}

QList<Nepomuk::Tag> KEditTagsDialog::tags() const
{
    return m_tags;
}

bool KEditTagsDialog::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_tagsList) && (event->type() == QEvent::Leave)) {
        m_deleteButtonTimer->stop();
        m_deleteButton->hide();
    }
    return KDialog::eventFilter(watched, event);
}

void KEditTagsDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        // update m_tags with the checked values, so
        // that the caller of the KEditTagsDialog can
        // receive the tags by KEditTagsDialog::tags()
        m_tags.clear();

        const int count = m_tagsList->count();
        for (int i = 0; i < count; ++i) {
            QListWidgetItem* item = m_tagsList->item(i);
            if (item->checkState() == Qt::Checked) {
                const QString label = item->data(Qt::UserRole).toString();
                Nepomuk::Tag tag(label);
                tag.setLabel(label);
                m_tags.append(tag);
            }
        }

        accept();
    } else {
        KDialog::slotButtonClicked(button);
    }
}

void KEditTagsDialog::slotTextEdited(const QString& text)
{
    // Remove unnecessary spaces from a new tag is
    // mandatory, as the user cannot see the difference
    // between a tag "Test" and "Test ".
    const QString tagText = text.simplified();
    if (tagText.isEmpty()) {
        removeNewTagItem();
        return;
    }

    // Check whether the new tag already exists. If this
    // is the case, remove the new tag item.
    const int count = m_tagsList->count();
    for (int i = 0; i < count; ++i) {
        const QListWidgetItem* item = m_tagsList->item(i);
        const bool remove = (item->text() == tagText) &&
                            ((m_newTagItem == 0) || (m_newTagItem != item));
        if (remove) {
            m_tagsList->scrollToItem(item);
            removeNewTagItem();
            return;
        }
    }

    // There is no tag in the list with the the passed text.
    if (m_newTagItem == 0) {
        m_newTagItem = new QListWidgetItem(tagText, m_tagsList);
    } else {
        m_newTagItem->setText(tagText);
    }
    m_newTagItem->setData(Qt::UserRole, tagText);
    m_newTagItem->setCheckState(Qt::Checked);
    m_tagsList->scrollToItem(m_newTagItem);
}

void KEditTagsDialog::slotItemEntered(QListWidgetItem* item)
{
    // align the delete-button to stay on the right border
    // of the item
    const QRect rect = m_tagsList->visualItemRect(item);
    const int size = rect.height();
    const int x = rect.right() - size;
    const int y = rect.top();
    m_deleteButton->move(x, y);
    m_deleteButton->resize(size, size);

    m_deleteCandidate = item;
    m_deleteButtonTimer->start();
}

void KEditTagsDialog::showDeleteButton()
{
    m_deleteButton->show();
}

void KEditTagsDialog::deleteTag()
{
    Q_ASSERT(m_deleteCandidate != 0);
    const QString text = i18nc("@info",
                               "Should the tag <resource>%1</resource> really be deleted for all files?",
                               m_deleteCandidate->text());
    const QString caption = i18nc("@title", "Delete tag");
    const KGuiItem deleteItem(i18nc("@action:button", "Delete"), KIcon("edit-delete"));
    const KGuiItem cancelItem(i18nc("@action:button", "Cancel"), KIcon("dialog-cancel"));
    if (KMessageBox::warningYesNo(this, text, caption, deleteItem, cancelItem) == KMessageBox::Yes) {
        const QString label = m_deleteCandidate->data(Qt::UserRole).toString();
        Nepomuk::Tag tag(label);
        tag.remove();

        // clear list and reload it
        for (int i = m_tagsList->count() - 1; i >= 0; --i) {
            QListWidgetItem* item = m_tagsList->takeItem(i);
            delete item;
        }
        loadTags();
    }
}

void KEditTagsDialog::loadTags()
{
    // load all available tags and mark those tags as checked
    // that have been passed to the KEditTagsDialog
    const QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
    foreach (const Nepomuk::Tag& tag, tags) {
        const QString label = tag.label();

        QListWidgetItem* item = new QListWidgetItem(label, m_tagsList);
        item->setData(Qt::UserRole, label);

        bool check = false;
        foreach (const Nepomuk::Tag& selectedTag, m_tags) {
            if (selectedTag.label() == label) {
                check = true;
                break;
            }
        }
        item->setCheckState(check ? Qt::Checked : Qt::Unchecked);
    }
}

void KEditTagsDialog::removeNewTagItem()
{
    if (m_newTagItem != 0) {
        const int row = m_tagsList->row(m_newTagItem);
        m_tagsList->takeItem(row);
        delete m_newTagItem;
        m_newTagItem = 0;
    }
}

#include "kedittagsdialog_p.moc"
