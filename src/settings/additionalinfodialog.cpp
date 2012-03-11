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

#include <KLocale>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

#include "views/rolesaccessor.h"

AdditionalInfoDialog::AdditionalInfoDialog(QWidget* parent,
                                           const QList<QByteArray>& visibleRoles) :
    KDialog(parent),
    m_visibleRoles(visibleRoles),
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
    header->setText(i18nc("@label", "Select which additional information should be shown:"));
    header->setWordWrap(true);
    layout->addWidget(header);

    // Add checkboxes
    const RolesAccessor& rolesAccessor = RolesAccessor::instance();
    const QList<QByteArray> roles = rolesAccessor.roles();
    foreach (const QByteArray& role, roles) {
        QCheckBox* checkBox = new QCheckBox(rolesAccessor.translation(role), mainWidget);
        checkBox->setChecked(visibleRoles.contains(role));
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

QList<QByteArray> AdditionalInfoDialog::visibleRoles() const
{
    return m_visibleRoles;
}

void AdditionalInfoDialog::slotOk()
{
    m_visibleRoles.clear();

    const QList<QByteArray> roles = RolesAccessor::instance().roles();
    int index = 0;
    foreach (const QByteArray& role, roles) {
        if (m_checkBoxes[index]->isChecked()) {
            m_visibleRoles.append(role);
        }
        ++index;
    }
}

#include "additionalinfodialog.moc"
