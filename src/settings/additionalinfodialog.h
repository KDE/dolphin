/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz (peter.penz@gmx.at)                  *
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

#ifndef ADDITIONALINFODIALOG_H
#define ADDITIONALINFODIALOG_H

#include <KDialog>
#include <KFileItemDelegate>
#include <QList>

class QCheckBox;

/**
 * @brief Dialog for changing the additional information properties of a directory.
 */
class AdditionalInfoDialog : public KDialog
{
    Q_OBJECT

public:
    AdditionalInfoDialog(QWidget* parent, KFileItemDelegate::InformationList infoList);
    virtual ~AdditionalInfoDialog();
    KFileItemDelegate::InformationList informationList() const;

private slots:
    void slotOk();

private:
    KFileItemDelegate::InformationList m_infoList;
    QList<QCheckBox*> m_checkBoxes;
};

#endif
