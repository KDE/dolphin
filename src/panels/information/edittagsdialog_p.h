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

#ifndef EDIT_TAGS_DIALOG_H
#define EDIT_TAGS_DIALOG_H

#include <kdialog.h>

#include <Nepomuk/Tag>

/**
 * @brief Dialog to edit a list of Nepomuk tags.
 *
 * It is possible for the user to add existing tags,
 * create new tags or to remove tags.
 */
class EditTagsDialog : public KDialog
{
    Q_OBJECT

public:
    EditTagsDialog(const QList<Nepomuk::Tag>& tags,
                   QWidget* parent = 0,
                   Qt::WFlags flags = 0);

    virtual ~EditTagsDialog();

    QList<Nepomuk::Tag> tags() const;

private:
    QList<Nepomuk::Tag> m_tags;
};

#endif
