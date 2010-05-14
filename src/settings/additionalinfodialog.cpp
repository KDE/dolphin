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

#include "additionalinfodialog.h"

#include "additionalinfoaccessor.h"
#include <klocale.h>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

AdditionalInfoDialog::AdditionalInfoDialog(QWidget* parent,
                                           KFileItemDelegate::InformationList infoList) :
    KDialog(parent),
    m_infoList(infoList),
    m_checkBoxes()
{
    setCaption(i18nc("@title:window", "Additional Information"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    QVBoxLayout* layout = new QVBoxLayout(mainWidget);

    // Add header
    QLabel* header = new QLabel(mainWidget);
    header->setText(i18nc("@label", "Select which additional information should be shown."));
    header->setWordWrap(true);
    layout->addWidget(header);

    // Add checkboxes
    const AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
    const KFileItemDelegate::InformationList keys = infoAccessor.keys();
    foreach (const KFileItemDelegate::Information info, keys) {
        QCheckBox* checkBox = new QCheckBox(infoAccessor.translation(info), mainWidget);
        checkBox->setChecked(infoList.contains(info));
        layout->addWidget(checkBox);
        m_checkBoxes.append(checkBox);
    }

    layout->addStretch(1);

    setMainWidget(mainWidget);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                                    "AdditionalInfoDialog");
    restoreDialogSize(dialogConfig);

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
}

AdditionalInfoDialog::~AdditionalInfoDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "AdditionalInfoDialog");
    saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

KFileItemDelegate::InformationList AdditionalInfoDialog::informationList() const
{
    return m_infoList;
}

void AdditionalInfoDialog::slotOk()
{
    m_infoList.clear();

    const KFileItemDelegate::InformationList keys = AdditionalInfoAccessor::instance().keys();
    int index = 0;
    foreach (const KFileItemDelegate::Information info, keys) {
        if (m_checkBoxes[index]->isChecked()) {
            m_infoList.append(info);
        }
        ++index;
    }
}

#include "additionalinfodialog.moc"
