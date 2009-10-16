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

#include "taggingwidget_p.h"

#include "edittagsdialog_p.h"

#include <kglobalsettings.h>
#include <klocale.h>

#include <QLabel>
#include <QVBoxLayout>

TaggingWidget::TaggingWidget(QWidget* parent) :
    QWidget(parent),
    m_label(0),
    m_tags(),
    m_tagsText()
{
    m_label = new QLabel(this);
    m_label->setFont(KGlobalSettings::smallestReadableFont());
    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignTop);
    connect(m_label, SIGNAL(linkActivated(const QString&)), this, SLOT(slotLinkActivated(const QString&)));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_label);

    setTags(QList<Nepomuk::Tag>());
}

TaggingWidget::~TaggingWidget()
{
}

void TaggingWidget::setTags(const QList<Nepomuk::Tag>& tags)
{
    m_tags = tags;

    m_tagsText.clear();
    bool first = true;
    foreach (const Nepomuk::Tag& tag, m_tags) {
        if (!first) {
            m_tagsText += ", ";
        }
        m_tagsText += tag.genericLabel();
        first = false;
    }

    if (m_tagsText.isEmpty()) {
        m_label->setText("<a href=\"addTags\">" + i18nc("@label", "Add Tags...") + "</a>");
    } else {
        m_label->setText("<p>" + m_tagsText + " <a href=\"changeTags\">" + i18nc("@label", "Change...") + "</a></p>");
    }

}

QList<Nepomuk::Tag> TaggingWidget::tags() const
{
    return m_tags;
}

void TaggingWidget::slotLinkActivated(const QString& link)
{
    Q_UNUSED(link);

    EditTagsDialog dialog(m_tags, this, Qt::Dialog);
    KConfigGroup dialogConfig(KGlobal::config(), "Nepomuk EditTagsDialog");
    dialog.restoreDialogSize(dialogConfig);

    if (dialog.exec() == QDialog::Accepted) {
        const QList<Nepomuk::Tag> oldTags = m_tags;
        m_tags = dialog.tags();

        if (oldTags.count() != m_tags.count()) {
            emit tagsChanged(m_tags);
        } else {
            // The number of tags is equal. Check whether the
            // content of the tags are also equal:
            const int tagsCount = m_tags.count();
            for (int i = 0; i < tagsCount; ++i) {
                if (oldTags[i].genericLabel() != m_tags[i].genericLabel()) {
                    // at least one tag has been changed
                    emit tagsChanged(m_tags);
                    break;
                }
            }
        }
    }

    dialog.saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

#include "taggingwidget_p.moc"
