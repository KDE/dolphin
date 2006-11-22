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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <kdialog.h>
#include <kurl.h>
#include <qstring.h>

class KLineEdit;

/**
 * @brief Dialog for renaming a variable number of files.
 *
 * The renaming is not done by the dialog, the invoker
 * must do this itself:
 * \code
 * RenameDialog dialog(...);
 * if (dialog.exec() == QDialog::Accepted) {
 *     const QString& newName = dialog.newName();
 *     // ... rename items corresponding to the new name
 * }
 * \endcode
 * @author Peter Penz
 */
class RenameDialog : public KDialog
{
    Q_OBJECT

public:
    RenameDialog(const KUrl::List& items);
    virtual ~RenameDialog();

    /**
     * Returns the new name of the items. If the returned string is not empty,
     * then it is assured that the string contains exactly one character #,
     * which should be replaced by ascending numbers. An empty string indicates
     * that the user has removed the # character.
     */
    const QString& newName() const { return m_newName; }

protected slots:
    virtual void slotButtonClicked(int button);

private:
    KLineEdit* m_lineEdit;
    QString m_newName;
};

#endif
