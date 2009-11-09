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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "servicessettingspage.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdesktopfileactions.h>
#include <kicon.h>
#include <klocale.h>
#include <knewstuff2/engine.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kstandarddirs.h>

#include <QEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

ServicesSettingsPage::ServicesSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_initialized(false),
    m_servicesList(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* label = new QLabel(i18nc("@label:textbox",
                                     "Configure which services should "
                                     "be shown in the context menu."), this);
    label->setWordWrap(true);

    m_servicesList = new QListWidget(this);
    m_servicesList->setSortingEnabled(true);
    m_servicesList->setSelectionMode(QAbstractItemView::NoSelection);
    connect(m_servicesList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SIGNAL(changed()));

    QPushButton* downloadButton = new QPushButton(i18nc("@action:button", "Download New Services..."));
    downloadButton->setIcon(KIcon("get-hot-new-stuff"));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadNewServices()));

    topLayout->addWidget(label);
    topLayout->addWidget(m_servicesList);
    topLayout->addWidget(downloadButton);
}

ServicesSettingsPage::~ServicesSettingsPage()
{
}

void ServicesSettingsPage::applySettings()
{
    KConfig config("kservicemenurc", KConfig::NoGlobals);
    KConfigGroup showGroup = config.group("Show");

    const int count = m_servicesList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_servicesList->item(i);
        const bool show = (item->checkState() == Qt::Checked);
        const QString service = item->data(Qt::UserRole).toString();
        showGroup.writeEntry(service, show);
    }

    showGroup.sync();
}

void ServicesSettingsPage::restoreDefaults()
{
    const int count = m_servicesList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_servicesList->item(i);
        item->setCheckState(Qt::Checked);
    }
}

bool ServicesSettingsPage::event(QEvent* event)
{
    if ((event->type() == QEvent::Polish) && !m_initialized) {
        QMetaObject::invokeMethod(this, "loadServices", Qt::QueuedConnection);
        m_initialized = true;
    }
    return SettingsPageBase::event(event);
}

void ServicesSettingsPage::loadServices()
{
    const KConfig config("kservicemenurc", KConfig::NoGlobals);
    const KConfigGroup showGroup = config.group("Show");

    const KService::List entries = KServiceTypeTrader::self()->query("KonqPopupMenu/Plugin");
    foreach (const KSharedPtr<KService>& service, entries) {
        const QString file = KStandardDirs::locate("services", service->entryPath());
        const QList<KServiceAction> serviceActions =
                                    KDesktopFileActions::userDefinedServices(file, true);

        foreach (const KServiceAction& action, serviceActions) {
            const QString service = action.name();
            const bool addService = !action.noDisplay()
                                    && !action.isSeparator()
                                    && !isInServicesList(service);

            if (addService) {
                QListWidgetItem* item = new QListWidgetItem(KIcon(action.icon()),
                                                            action.text(),
                                                            m_servicesList);
                item->setData(Qt::UserRole, service);
                const bool show = showGroup.readEntry(service, true);
                item->setCheckState(show ? Qt::Checked : Qt::Unchecked);
            }
        }
    }
}

void ServicesSettingsPage::downloadNewServices()
{
    KNS::Engine khns(this);
    khns.init("servicemenu.knsrc");
    khns.downloadDialogModal(this);
    loadServices();
}

bool ServicesSettingsPage::isInServicesList(const QString& service) const
{
    const int count = m_servicesList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_servicesList->item(i);
        if (item->data(Qt::UserRole).toString() == service) {
            return true;
        }
    }
    return false;
}

#include "servicessettingspage.moc"
