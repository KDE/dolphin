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

#include <config-nepomuk.h>

#include <KLocale>
#include "kitemviews/kfileitemmodel.h"
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

#ifdef HAVE_NEPOMUK
    #include <Nepomuk2/ResourceManager>
#endif

AdditionalInfoDialog::AdditionalInfoDialog(QWidget* parent,
                                           const QList<QByteArray>& visibleRoles) :
    KDialog(parent),
    m_visibleRoles(visibleRoles),
    m_listWidget(0)
{
    setCaption(i18nc("@title:window", "Additional Information"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // Add header
    QLabel* header = new QLabel(mainWidget);
    header->setText(i18nc("@label", "Select which additional information should be shown:"));
    header->setWordWrap(true);

    // Add checkboxes
    bool nepomukRunning = false;
    bool indexingEnabled = false;
#ifdef HAVE_NEPOMUK
    nepomukRunning = (Nepomuk2::ResourceManager::instance()->initialized());
    if (nepomukRunning) {
        KConfig config("nepomukserverrc");
        indexingEnabled = config.group("Service-nepomukfileindexer").readEntry("autostart", true);
    }
#endif

    m_listWidget = new QListWidget(mainWidget);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
        QListWidgetItem* item = new QListWidgetItem(info.translation, m_listWidget);
        item->setCheckState(visibleRoles.contains(info.role) ? Qt::Checked : Qt::Unchecked);

        const bool enable = (!info.requiresNepomuk && !info.requiresIndexer) ||
                            (info.requiresNepomuk && nepomukRunning) ||
                            (info.requiresIndexer && indexingEnabled);

        if (!enable) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }

    QVBoxLayout* layout = new QVBoxLayout(mainWidget);
    layout->addWidget(header);
    layout->addWidget(m_listWidget);
    layout->addStretch(1);

    setMainWidget(mainWidget);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"), "AdditionalInfoDialog");
    restoreDialogSize(dialogConfig);

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
}

AdditionalInfoDialog::~AdditionalInfoDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"), "AdditionalInfoDialog");
    saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

QList<QByteArray> AdditionalInfoDialog::visibleRoles() const
{
    return m_visibleRoles;
}

void AdditionalInfoDialog::slotOk()
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
}

#include "additionalinfodialog.moc"
