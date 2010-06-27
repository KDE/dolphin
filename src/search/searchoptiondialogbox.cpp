/*****************************************************************************
 * Copyright (C) 2010 by Laurent Montel <montel@kde.org>                     *
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

#include "searchoptiondialogbox.h"

#include <KConfigGroup>
#include <KLineEdit>
#include <KLocale>
#include <QLabel>
#include <QHBoxLayout>

SearchOptionDialogBox::SearchOptionDialogBox(QWidget* parent) :
    KDialog(parent, Qt::Dialog)
{
    QWidget* container = new QWidget(this);

    QLabel* label = new QLabel(i18nc("@label", "Name:"), container);
    m_lineEdit = new KLineEdit(container);
    m_lineEdit->setMinimumWidth(250);
    m_lineEdit->setClearButtonShown(true);

    connect(m_lineEdit, SIGNAL(textChanged(const QString&)), SLOT(slotTextChanged(const QString&)));
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->addWidget(label, Qt::AlignRight);
    layout->addWidget(m_lineEdit);

    setMainWidget(container);
    setCaption(i18nc("@title:window", "Save Search Options"));
    setButtons(KDialog::Ok | KDialog::Cancel);
    setDefaultButton(KDialog::Ok);
    setButtonText(KDialog::Ok, i18nc("@action:button", "Save"));

    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "SaveSearchOptionsDialog");
    restoreDialogSize(dialogConfig);
    enableButtonOk(false);
}

SearchOptionDialogBox::~SearchOptionDialogBox()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "SaveSearchOptionsDialog");
    saveDialogSize(dialogConfig);
}

QString SearchOptionDialogBox::text() const
{
    return m_lineEdit->text();
}

void SearchOptionDialogBox::slotTextChanged(const QString& text)
{
    enableButtonOk(!text.isEmpty());
}

#include "searchoptiondialogbox.moc"
