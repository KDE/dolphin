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

#include "ktaggingwidget_p.h"

#include "kedittagsdialog_p.h"

#include <kglobalsettings.h>
#include <klocale.h>
#include <kurl.h>

#include <QEvent>
#include <QLabel>
#include <QVBoxLayout>

KTaggingWidget::KTaggingWidget(QWidget* parent) :
    QWidget(parent),
    m_readOnly(false),
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

KTaggingWidget::~KTaggingWidget()
{
}

void KTaggingWidget::setTags(const QList<Nepomuk::Tag>& tags)
{
    m_tags = tags;

    m_tagsText.clear();
    bool first = true;
    foreach (const Nepomuk::Tag& tag, m_tags) {
        if (!first) {
            m_tagsText += ", ";
        }
        if (m_readOnly) {
            m_tagsText += tag.genericLabel();
        } else {
            // use the text color for the tag-links, to create a visual difference
            // to the semantically different "Change..." link
            const QColor linkColor =palette().text().color();
            const char* link = "<a style=\"color:%1;\" href=\"%2\">%3</a>";
            m_tagsText += QString::fromLatin1(link).arg(linkColor.name(),
                                                        KUrl(tag.resourceUri()).url(),
                                                        tag.genericLabel());
        }
        first = false;
    }

    QString text;
    if (m_tagsText.isEmpty()) {
        if (m_readOnly) {
            text = "-";
        } else {
            text = "<a href=\"changeTags\">" + i18nc("@label", "Add Tags...") + "</a>";
        }
    } else {
        if (m_readOnly) {
            text = m_tagsText;
        } else {
            text += m_tagsText + " <a href=\"changeTags\">" + i18nc("@label", "Change...") + "</a>";
        }
    }
    m_label->setText(text);
}

QList<Nepomuk::Tag> KTaggingWidget::tags() const
{
    return m_tags;
}

void KTaggingWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    setTags(m_tags);
}

bool KTaggingWidget::isReadOnly() const
{
    return m_readOnly;
}

bool KTaggingWidget::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        m_label->setForegroundRole(foregroundRole());
    }
    return QWidget::event(event);
}

void KTaggingWidget::slotLinkActivated(const QString& link)
{
    if (link != QLatin1String("changeTags")) {
        emit tagActivated(Nepomuk::Tag(KUrl(link)));
        return;
    }

    KEditTagsDialog dialog(m_tags, this, Qt::Dialog);
    KConfigGroup dialogConfig(KGlobal::config(), "Nepomuk KEditTagsDialog");
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

#include "ktaggingwidget_p.moc"
