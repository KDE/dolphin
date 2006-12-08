/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
#ifndef EDITBOOKMARKDIALOG_H
#define EDITBOOKMARKDIALOG_H

#include <kbookmark.h>
#include <kdialog.h>

class Bookmark;
class QLineEdit;
class QPushButton;

/**
 * @brief Allows to edit the icon, Url and name of a bookmark.
 *
 * The default usage is like this:
 * \code
 * KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add Bookmark"),
 *                                                      i18n("New bookmark"),
 *                                                      KUrl(),
 *                                                      "bookmark");
 * if (!bookmark.isNull()) {
 *     // ...
 * }
 * \endcode
 */
class EditBookmarkDialog : public KDialog
{
    Q_OBJECT

public:
    virtual ~EditBookmarkDialog();

    /**
     * Opens a dialog where the current icon, Url and name of
     * an Url are editable. The title of the dialog is set to \a title.
     * @return A valid bookmark, if the user has pressed OK. Otherwise
     *         a null bookmark is returned (see Bookmark::isNull()).
     */
    static KBookmark getBookmark(const QString& title,
                                 const QString& name,
                                 const KUrl& url,
                                 const QString& icon);

protected slots:
    virtual void slotButtonClicked(int button);

protected:
    EditBookmarkDialog(const QString& title,
                       const QString& name,
                       const KUrl& url,
                       const QString& icon);

private slots:
    void selectIcon();
    void selectLocation();

private:
    QString m_iconName;
    QPushButton* m_iconButton;
    QLineEdit* m_name;
    QLineEdit* m_location;
    KBookmark m_bookmark;
};
#endif
