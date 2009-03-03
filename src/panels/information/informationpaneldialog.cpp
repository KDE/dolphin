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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "informationpaneldialog.h"

#include <klocale.h>
#include <kiconloader.h>

#include <QVBoxLayout>

InformationPanelDialog::InformationPanelDialog(QWidget* parent) :
    KDialog(parent),
    m_isDirty(false)
{
    setCaption(i18nc("@title:window", "Information Panel"));
    setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

    QWidget* main = new QWidget();
    // ...
    Q_UNUSED(main);
    QVBoxLayout* topLayout = new QVBoxLayout();
    // ...
    Q_UNUSED(topLayout);

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));

    main->setLayout(topLayout);
    setMainWidget(main);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                                    "InformationPanelDialog");
    restoreDialogSize(dialogConfig);

    loadSettings();
}

InformationPanelDialog::~InformationPanelDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "InformationPanelDialog");
    saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

void InformationPanelDialog::slotOk()
{
    // ...
    accept();
}

void InformationPanelDialog::slotApply()
{
    // ...
    markAsDirty(false);
}

void InformationPanelDialog::markAsDirty(bool isDirty)
{
    m_isDirty = isDirty;
    enableButtonApply(isDirty);
}

void InformationPanelDialog::loadSettings()
{
}

#include "informationpaneldialog.moc"
