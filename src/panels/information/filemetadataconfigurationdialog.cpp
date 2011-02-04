/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "filemetadataconfigurationdialog.h"

#include <kfilemetadataconfigurationwidget.h>
#include <KLocale>
#include <QLabel>
#include <QVBoxLayout>

FileMetaDataConfigurationDialog::FileMetaDataConfigurationDialog(QWidget* parent) :
    KDialog(parent),
    m_descriptionLabel(0),
    m_configWidget(0)

{
    setCaption(i18nc("@title:window", "Configure Shown Data"));
    setButtons(KDialog::Ok | KDialog::Cancel);
    setDefaultButton(KDialog::Ok);


    m_descriptionLabel = new QLabel(i18nc("@label::textbox",
                                          "Select which data should "
                                          "be shown:"), this);
    m_descriptionLabel->setWordWrap(true);

    m_configWidget = new KFileMetaDataConfigurationWidget(this);

    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);
    topLayout->addWidget(m_descriptionLabel);
    topLayout->addWidget(m_configWidget);
    setMainWidget(mainWidget);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                                    "FileMetaDataConfigurationDialog");
    restoreDialogSize(dialogConfig);
}

FileMetaDataConfigurationDialog::~FileMetaDataConfigurationDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "FileMetaDataConfigurationDialog");
    saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

void FileMetaDataConfigurationDialog::setItems(const KFileItemList& items)
{
    m_configWidget->setItems(items);
}

KFileItemList FileMetaDataConfigurationDialog::items() const
{
    return m_configWidget->items();
}

void FileMetaDataConfigurationDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        m_configWidget->save();
        accept();
    } else {
        KDialog::slotButtonClicked(button);
    }
}

void FileMetaDataConfigurationDialog::setDescription(const QString& description)
{
    m_descriptionLabel->setText(description);
}

QString FileMetaDataConfigurationDialog::description() const
{
    return m_descriptionLabel->text();
}

#include "filemetadataconfigurationdialog.moc"
