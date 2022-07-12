/*
 * SPDX-FileCopyrightText: 2009-2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "vcssettingstab.h"

#include "dolphin_generalsettings.h"
#include "dolphin_versioncontrolsettings.h"
#include "settings/serviceitemdelegate.h"
#include "settings/servicemodel.h"
#include "global.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QApplication>
#include <QLabel>
#include <QListView>
#include <QScroller>
#include <QShowEvent>
#include <QLineEdit>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
    bool laterSelected = false;
}

VcsSettingsTab::VcsSettingsTab(const QVector<KPluginMetaData>& plugins, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* label = new QLabel(i18nc("@label:textbox",
                                     "Select for which version control systems the "
                                     "status of files and folders should be indicated "
                                     "in the view:"), this);
    label->setWordWrap(true);

    m_listView = new QListView(this);
    QScroller::grabGesture(m_listView->viewport(), QScroller::TouchGesture);

    auto *delegate = new ServiceItemDelegate(m_listView, m_listView);
    m_serviceModel = new ServiceModel(this);
    m_listView->setModel(m_serviceModel);
    m_listView->setItemDelegate(delegate);
    m_listView->setVerticalScrollMode(QListView::ScrollPerPixel);
    connect(m_listView, &QListView::clicked, this, &VcsSettingsTab::changed);

    topLayout->addWidget(label);
    topLayout->addWidget(m_listView);

    m_enabledVcsPlugins = VersionControlSettings::enabledPlugins();
    std::sort(m_enabledVcsPlugins.begin(), m_enabledVcsPlugins.end());

    const QStringList enabledPlugins = VersionControlSettings::enabledPlugins();

    QSet<QString> loadedPlugins;

    for (const auto &plugin : plugins) {
        const QString pluginName = plugin.name();
        addRow(QStringLiteral("code-class"),
               pluginName,
               pluginName,
               enabledPlugins.contains(pluginName));
        loadedPlugins += pluginName;
    }
}

VcsSettingsTab::~VcsSettingsTab() = default;

void VcsSettingsTab::applySettings()
{
    QStringList enabledPlugins;

    const QAbstractItemModel *model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        const bool checked = model->data(index, Qt::CheckStateRole).toBool();

        if (checked) {
            enabledPlugins.append(model->data(index, Qt::DisplayRole).toString());
        }
    }

    if (m_enabledVcsPlugins != enabledPlugins) {
        VersionControlSettings::setEnabledPlugins(enabledPlugins);
        VersionControlSettings::self()->save();

        if (!laterSelected) {
            KMessageBox::ButtonCode promptRestart = KMessageBox::questionYesNo(window(),
                                    i18nc("@info", "Dolphin must be restarted to apply the "
                                                "updated version control system settings."),
                                    i18nc("@info", "Restart now?"),
                                    KGuiItem(QApplication::translate("KStandardGuiItem", "&Restart"), QStringLiteral("dialog-restart")),
                                    KGuiItem(QApplication::translate("KStandardGuiItem", "&Later"), QStringLiteral("dialog-later"))
                        );
            if (promptRestart == KMessageBox::ButtonCode::Yes) {
                Dolphin::openNewWindow();
                qApp->quit();
            } else {
                laterSelected = true;
            }
        }
    }
}

void VcsSettingsTab::restoreDefaultSettings()
{
    QAbstractItemModel* model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        // No VCS plug-ins are enabled by default.
        model->setData(index, false, Qt::CheckStateRole);
    }
}

void VcsSettingsTab::addRow(const QString &icon,
                                     const QString &text,
                                     const QString &value,
                                     bool checked)
{
    const int row = m_serviceModel->rowCount();

    m_serviceModel->insertRow(row);

    const QModelIndex index = m_serviceModel->index(row, 0);
    m_serviceModel->setData(index, icon, Qt::DecorationRole);
    m_serviceModel->setData(index, text, Qt::DisplayRole);
    m_serviceModel->setData(index, value, ServiceModel::DesktopEntryNameRole);
    m_serviceModel->setData(index, checked, Qt::CheckStateRole);
}
