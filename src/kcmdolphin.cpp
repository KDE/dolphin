/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "kcmdolphin.h"

#include "columnviewsettingspage.h"
#include "detailsviewsettingspage.h"
#include "generalviewsettingspage.h"
#include "iconsviewsettingspage.h"

#include <kdialog.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QPushButton>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinConfigFactory, registerPlugin<DolphinConfigModule>("dolphin");)
K_EXPORT_PLUGIN(KCMDolphinConfigFactory("kcmdolphin"))

DolphinConfigModule::DolphinConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(KCMDolphinConfigFactory::componentData(), parent),
    m_pages()
{
    Q_UNUSED(args);

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    QTabWidget* tabWidget = new QTabWidget(this);

    // initialize 'General' tab
    GeneralViewSettingsPage* generalPage = new GeneralViewSettingsPage(QDir::homePath(), tabWidget);
    tabWidget->addTab(generalPage, KIcon("view-choose"), i18nc("@title:tab General settings", "General"));
    connect(generalPage, SIGNAL(changed()), this, SLOT(changed()));

    // initialize 'Icons' tab
    IconsViewSettingsPage* iconsPage = new IconsViewSettingsPage(tabWidget);
    tabWidget->addTab(iconsPage, KIcon("view-list-icons"), i18nc("@title:tab", "Icons"));
    connect(iconsPage, SIGNAL(changed()), this, SLOT(changed()));

    // initialize 'Details' tab
    DetailsViewSettingsPage* detailsPage = new DetailsViewSettingsPage(tabWidget);
    tabWidget->addTab(detailsPage, KIcon("view-list-details"), i18nc("@title:tab", "Details"));
    connect(detailsPage, SIGNAL(changed()), this, SLOT(changed()));

    // initialize 'Column' tab
    ColumnViewSettingsPage* columnPage = new ColumnViewSettingsPage(tabWidget);
    tabWidget->addTab(columnPage, KIcon("view-file-columns"), i18nc("@title:tab", "Column"));
    connect(columnPage, SIGNAL(changed()), this, SLOT(changed()));

    m_pages.append(generalPage);
    m_pages.append(iconsPage);
    m_pages.append(detailsPage);
    m_pages.append(columnPage);

    topLayout->addWidget(tabWidget, 0, 0);
}

DolphinConfigModule::~DolphinConfigModule()
{
}

void DolphinConfigModule::save()
{
    foreach (ViewSettingsPageBase* page, m_pages) {
        page->applySettings();
    }
    reparseConfiguration();
}

void DolphinConfigModule::defaults()
{
    foreach (ViewSettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }
    reparseConfiguration();
}

void DolphinConfigModule::reparseConfiguration()
{
    QDBusMessage message = QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);
}

#include "kcmdolphin.moc"
