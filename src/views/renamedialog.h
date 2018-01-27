/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz (peter.penz@gmx.at)             *
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

#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include "dolphin_export.h"

#include <QDialog>
#include <KFileItem>
#include <QString>

class QLineEdit;
class QSpinBox;
class QPushButton;
class KJob;
/**
 * @brief Dialog for renaming a variable number of files.
 */
class DOLPHIN_EXPORT RenameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameDialog(QWidget* parent, const KFileItemList& items);
    ~RenameDialog() override;

signals:
    void renamingFinished(const QList<QUrl>& urls);

private slots:
    void slotAccepted();
    void slotTextChanged(const QString& newName);
    void slotFileRenamed(const QUrl& oldUrl, const QUrl& newUrl);
    void slotResult(KJob* job);

protected:
    void showEvent(QShowEvent* event) override;

private:
    bool m_renameOneItem;
    QList<QUrl> m_renamedItems;
    QString m_newName;
    QLineEdit* m_lineEdit;
    KFileItemList m_items;
    bool m_allExtensionsDifferent;
    QSpinBox* m_spinBox;
    QPushButton* m_okButton;
};

#endif
