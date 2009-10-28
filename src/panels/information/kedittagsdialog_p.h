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

#ifndef KEDIT_TAGS_DIALOG_H
#define KEDIT_TAGS_DIALOG_H

#include <kdialog.h>
#include <nepomuk/tag.h>

class KLineEdit;
class QListWidget;
class QListWidgetItem;

/**
 * @brief Dialog to edit a list of Nepomuk tags.
 *
 * It is possible for the user to add existing tags,
 * create new tags or to remove tags.
 *
 * @see KMetaDataConfigurationDialog
 */
class KEditTagsDialog : public KDialog
{
    Q_OBJECT

public:
    KEditTagsDialog(const QList<Nepomuk::Tag>& tags,
                    QWidget* parent = 0,
                    Qt::WFlags flags = 0);

    virtual ~KEditTagsDialog();

    QList<Nepomuk::Tag> tags() const;

protected slots:
    virtual void slotButtonClicked(int button);

private slots:
    void slotTextEdited(const QString& text);

private:
    void loadTags();
    void removeNewTagItem();

private:
    QList<Nepomuk::Tag> m_tags;
    QListWidget* m_tagsList;
    QListWidgetItem* m_newTagItem;
    KLineEdit* m_newTagEdit;
};

#endif
