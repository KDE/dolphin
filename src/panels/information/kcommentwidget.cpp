/*****************************************************************************
 * Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                     *
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

#include "kcommentwidget_p.h"

#include <kdialog.h>
#include <kglobalsettings.h>
#include <klocale.h>

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

KCommentWidget::KCommentWidget(QWidget* parent) :
    QWidget(parent),
    m_label(0),
    m_comment()
{
    m_label = new QLabel(this);
    m_label->setFont(KGlobalSettings::smallestReadableFont());
    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignTop);
    connect(m_label, SIGNAL(linkActivated(const QString&)), this, SLOT(slotLinkActivated(const QString&)));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_label);

    setText(m_comment);
}

KCommentWidget::~KCommentWidget()
{
}

void KCommentWidget::setText(const QString& comment)
{
    if (comment.isEmpty()) {
        m_label->setText("<a href=\"addComment\">" + i18nc("@label", "Add Comment...") + "</a>");
    } else {
        m_label->setText("<p>" + comment + " <a href=\"changeComment\">" + i18nc("@label", "Change...") + "</a></p>");
    }
    m_comment = comment;
}

QString KCommentWidget::text() const
{
    return m_comment;
}

void KCommentWidget::slotLinkActivated(const QString& link)
{
    KDialog dialog(this, Qt::Dialog);

    QTextEdit* editor = new QTextEdit(&dialog);
    editor->setText(m_comment);

    dialog.setMainWidget(editor);

    const QString caption = (link == "changeComment") ?
                            i18nc("@title:window", "Change Comment") :
                            i18nc("@title:window", "Add Comment");
    dialog.setCaption(caption);
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);
    dialog.setDefaultButton(KDialog::Ok);

    KConfigGroup dialogConfig(KGlobal::config(), "Nepomuk KEditCommentDialog");
    dialog.restoreDialogSize(dialogConfig);

    if (dialog.exec() == QDialog::Accepted) {
        const QString oldText = m_comment;
        setText(editor->toPlainText());
        if (oldText != m_comment) {
            emit commentChanged(m_comment);
        }
    }

    dialog.saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

#include "kcommentwidget_p.moc"
