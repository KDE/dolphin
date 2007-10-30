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

#include <klocale.h>
#include <kvbox.h>

#include <QCheckBox>

AdditionalInfoDialog::AdditionalInfoDialog(QWidget* parent,
                                           KFileItemDelegate::InformationList info) :
    KDialog(parent),
    m_info(info),
    m_size(0),
    m_date(0),
    m_permissions(0),
    m_owner(0),
    m_group(0),
    m_type(0)
{
    setCaption(i18nc("@title:window", "Additional Information"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    KVBox* box = new KVBox(this);

    m_size = new QCheckBox(i18nc("@option:check Additional Information", "Size"), box);
    m_date = new QCheckBox(i18nc("@option:check Additional Information", "Date"), box);
    m_permissions = new QCheckBox(i18nc("@option:check Additional Information", "Permissions"), box);
    m_owner = new QCheckBox(i18nc("@option:check Additional Information", "Owner"), box);
    m_group = new QCheckBox(i18nc("@option:check Additional Information", "Group"), box);
    m_type = new QCheckBox(i18nc("@option:check Additional Information", "Type"), box);
    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));

    m_size->setChecked(info.contains(KFileItemDelegate::Size));
    m_date->setChecked(info.contains(KFileItemDelegate::ModificationTime));
    m_permissions->setChecked(info.contains(KFileItemDelegate::Permissions));
    m_owner->setChecked(info.contains(KFileItemDelegate::Owner));
    m_group->setChecked(info.contains(KFileItemDelegate::OwnerAndGroup));
    m_type->setChecked(info.contains(KFileItemDelegate::FriendlyMimeType));

    setMainWidget(box);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                                    "AdditionalInfoDialog");
    restoreDialogSize(dialogConfig);

}

AdditionalInfoDialog::~AdditionalInfoDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "AdditionalInfoDialog");
    saveDialogSize(dialogConfig, KConfigBase::Persistent);

}

KFileItemDelegate::InformationList AdditionalInfoDialog::additionalInfo() const
{
    return m_info;
}

void AdditionalInfoDialog::slotOk()
{
    m_info.clear();

    if (m_size->isChecked()) {
        m_info.append(KFileItemDelegate::Size);
    }
    if (m_date->isChecked()) {
        m_info.append(KFileItemDelegate::ModificationTime);
    }
    if (m_permissions->isChecked()) {
        m_info.append(KFileItemDelegate::Permissions);
    }
    if (m_owner->isChecked()) {
        m_info.append(KFileItemDelegate::Owner);
    }
    if (m_group->isChecked()) {
        m_info.append(KFileItemDelegate::OwnerAndGroup);
    }
    if (m_type->isChecked()) {
        m_info.append(KFileItemDelegate::FriendlyMimeType);
    }
}

#include "additionalinfodialog.moc"
