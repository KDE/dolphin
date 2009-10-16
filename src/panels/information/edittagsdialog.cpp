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

#include "edittagsdialog_p.h"

#include <klineedit.h>
#include <klocale.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

EditTagsDialog::EditTagsDialog(const QList<Nepomuk::Tag>& tags,
                               QWidget* parent,
                               Qt::WFlags flags) :
    KDialog(parent, flags),
    m_tags(tags),
    m_tagsList(0),
    m_newTagItem(0),
    m_newTagEdit(0)
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
    m_tagsList->setSortingEnabled(true);
    m_tagsList->setSelectionMode(QAbstractItemView::NoSelection);

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
}

EditTagsDialog::~EditTagsDialog()
{
}

QList<Nepomuk::Tag> EditTagsDialog::tags() const
{
    return m_tags;
}

void EditTagsDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        // update m_tags with the checked values, so
        // that the caller of the EditTagsDialog can
        // receive the tags by EditTagsDialog::tags()
        m_tags.clear();

        const int count = m_tagsList->count();
        for (int i = 0; i < count; ++i) {
            QListWidgetItem* item = m_tagsList->item(i);
            if (item->checkState() == Qt::Checked) {
                Nepomuk::Tag tag;
                tag.setLabel(item->data(Qt::UserRole).toString());
                m_tags.append(tag);
            }
        }

        accept();
    } else {
        KDialog::slotButtonClicked(button);
    }
}

void EditTagsDialog::slotTextEdited(const QString& text)
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

void EditTagsDialog::loadTags()
{
    // load all available tags and mark those tags as checked
    // that have been passed to the EditTagsDialog
    QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
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

void EditTagsDialog::removeNewTagItem()
{
    if (m_newTagItem != 0) {
        const int row = m_tagsList->row(m_newTagItem);
        m_tagsList->takeItem(row);
        delete m_newTagItem;
        m_newTagItem = 0;
    }
}

#include "edittagsdialog_p.moc"
