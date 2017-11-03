/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef HAVE_BALOO
#include <kfilemetadataconfigurationwidget.h>
#else
#include <Baloo/FileMetaDataConfigWidget>
#endif
#include <KSharedConfig>
#include <KLocalizedString>
#include <QLabel>
#include <QVBoxLayout>
#include <KConfigGroup>
#include <KWindowConfig>
#include <QDialogButtonBox>
#include <QPushButton>

FileMetaDataConfigurationDialog::FileMetaDataConfigurationDialog(QWidget* parent) :
    QDialog(parent),
    m_descriptionLabel(nullptr),
    m_configWidget(nullptr)

{
    setWindowTitle(i18nc("@title:window", "Configure Shown Data"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FileMetaDataConfigurationDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &FileMetaDataConfigurationDialog::reject);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    m_descriptionLabel = new QLabel(i18nc("@label::textbox",
                                          "Select which data should "
                                          "be shown:"), this);
    m_descriptionLabel->setWordWrap(true);

#ifndef HAVE_BALOO
    m_configWidget = new KFileMetaDataConfigurationWidget(this);
#else
    m_configWidget = new Baloo::FileMetaDataConfigWidget(this);
#endif


    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);
    topLayout->addWidget(m_descriptionLabel);
    topLayout->addWidget(m_configWidget);
    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);


    const KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")),
                                    "FileMetaDataConfigurationDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), dialogConfig);
}

FileMetaDataConfigurationDialog::~FileMetaDataConfigurationDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")),
                              "FileMetaDataConfigurationDialog");
    KWindowConfig::saveWindowSize(windowHandle(), dialogConfig);
}

void FileMetaDataConfigurationDialog::setItems(const KFileItemList& items)
{
    m_configWidget->setItems(items);
}

KFileItemList FileMetaDataConfigurationDialog::items() const
{
    return m_configWidget->items();
}

void FileMetaDataConfigurationDialog::slotAccepted()
{
    m_configWidget->save();
    accept();
}

void FileMetaDataConfigurationDialog::setDescription(const QString& description)
{
    m_descriptionLabel->setText(description);
}

QString FileMetaDataConfigurationDialog::description() const
{
    return m_descriptionLabel->text();
}

