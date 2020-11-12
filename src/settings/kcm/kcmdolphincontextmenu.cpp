/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcmdolphincontextmenu.h"

#include "settings/contextmenu/contextmenusettingspage.h"

#include <kconfigwidgets_version.h>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinContextMenuConfigFactory, registerPlugin<DolphinContextMenuConfigModule>(QStringLiteral("dolphincontextmenu"));)

DolphinContextMenuConfigModule::DolphinContextMenuConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(parent, args),
    m_contextMenu(nullptr)
{
    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    m_contextMenu = new ContextMenuSettingsPage(this);
    connect(m_contextMenu, &ContextMenuSettingsPage::changed, this, &DolphinContextMenuConfigModule::markAsChanged);
    topLayout->addWidget(m_contextMenu, 0, {});
}

DolphinContextMenuConfigModule::~DolphinContextMenuConfigModule()
{
}

void DolphinContextMenuConfigModule::save()
{
    m_contextMenu->applySettings();
}

void DolphinContextMenuConfigModule::defaults()
{
    m_contextMenu->restoreDefaults();
}

#include "kcmdolphincontextmenu.moc"
