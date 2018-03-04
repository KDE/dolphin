/***************************************************************************
 *   Copyright (C) 2007-2012 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "kitemviews/kfileitemmodel.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <config-baloo.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#ifdef HAVE_BALOO
    #include <Baloo/IndexerConfig>
#endif

AdditionalInfoDialog::AdditionalInfoDialog(QWidget* parent,
                                           const QList<QByteArray>& visibleRoles) :
    QDialog(parent),
    m_visibleRoles(visibleRoles),
    m_listWidget(nullptr)
{
    setWindowTitle(i18nc("@title:window", "Additional Information"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    auto layout = new QVBoxLayout(this);
    setLayout(layout);

    // Add header
    auto header = new QLabel(this);
    header->setText(i18nc("@label", "Select which additional information should be shown:"));
    header->setWordWrap(true);
    layout->addWidget(header);

    // Add checkboxes
    bool indexingEnabled = false;
#ifdef HAVE_BALOO
    Baloo::IndexerConfig config;
    indexingEnabled = config.fileIndexingEnabled();
#endif

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
        QListWidgetItem* item = new QListWidgetItem(info.translation, m_listWidget);
        item->setCheckState(visibleRoles.contains(info.role) ? Qt::Checked : Qt::Unchecked);

        const bool enable = ((!info.requiresBaloo && !info.requiresIndexer) ||
                            (info.requiresBaloo) ||
                            (info.requiresIndexer && indexingEnabled)) && info.role != "text";

        if (!enable) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }
    layout->addWidget(m_listWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AdditionalInfoDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AdditionalInfoDialog::reject);
    layout->addWidget(buttonBox);

    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setShortcut(Qt::CTRL + Qt::Key_Return);
    okButton->setDefault(true);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")), "AdditionalInfoDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), dialogConfig);
}

AdditionalInfoDialog::~AdditionalInfoDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")), "AdditionalInfoDialog");
    KWindowConfig::saveWindowSize(windowHandle(), dialogConfig);
}

QList<QByteArray> AdditionalInfoDialog::visibleRoles() const
{
    return m_visibleRoles;
}

void AdditionalInfoDialog::accept()
{
    m_visibleRoles.clear();

    int index = 0;
    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
        const QListWidgetItem* item = m_listWidget->item(index);
        if (item->checkState() == Qt::Checked) {
            m_visibleRoles.append(info.role);
        }
        ++index;
    }

    QDialog::accept();
}
