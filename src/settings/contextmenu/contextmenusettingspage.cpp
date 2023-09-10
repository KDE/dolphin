/*
 * SPDX-FileCopyrightText: 2009-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "contextmenusettingspage.h"

#include "dolphin_contextmenusettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_versioncontrolsettings.h"
#include "global.h"
#include "settings/servicemodel.h"

#include <KDesktopFile>
#include <KDesktopFileActions>
#include <KFileUtils>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginMetaData>
#include <KService>
#include <kiocore_export.h>
#include <kservice_export.h>
#include <kwidgetsaddons_version.h>

#include <KNSWidgets/Button>
#include <QtGlobal>

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScroller>
#include <QShowEvent>
#include <QSortFilterProxyModel>

namespace
{
const bool ShowDeleteDefault = false;
const char VersionControlServicePrefix[] = "_version_control_";
const char DeleteService[] = "_delete";
const char CopyToMoveToService[] = "_copy_to_move_to";

bool laterSelected = false;
}

ContextMenuSettingsPage::ContextMenuSettingsPage(QWidget *parent, const KActionCollection *actions, const QStringList &actionIds)
    : SettingsPageBase(parent)
    , m_initialized(false)
    , m_serviceModel(nullptr)
    , m_sortModel(nullptr)
    , m_listView(nullptr)
    , m_enabledVcsPlugins()
    , m_actions(actions)
    , m_actionIds(actionIds)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);

    QLabel *label = new QLabel(i18nc("@label:textbox",
                                     "Select which services should "
                                     "be shown in the context menu:"),
                               this);
    label->setWordWrap(true);
    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setPlaceholderText(i18nc("@label:textbox", "Search…"));
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, [this](const QString &filter) {
        m_sortModel->setFilterFixedString(filter);
    });

    m_listView = new QListView(this);
    QScroller::grabGesture(m_listView->viewport(), QScroller::TouchGesture);

    m_serviceModel = new ServiceModel(this);
    m_sortModel = new QSortFilterProxyModel(this);
    m_sortModel->setSourceModel(m_serviceModel);
    m_sortModel->setSortRole(Qt::DisplayRole);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->setFilterRole(Qt::DisplayRole);
    m_sortModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_listView->setModel(m_sortModel);
    m_listView->setVerticalScrollMode(QListView::ScrollPerPixel);
    connect(m_listView, &QListView::clicked, this, &ContextMenuSettingsPage::changed);

    topLayout->addWidget(label);
    topLayout->addWidget(m_searchLineEdit);
    topLayout->addWidget(m_listView);

#ifndef Q_OS_WIN
    using NewStuffButton = KNSWidgets::Button;
    auto *downloadButton = new NewStuffButton(i18nc("@action:button", "Download New Services…"), QStringLiteral("servicemenu.knsrc"), this);
    connect(downloadButton, &NewStuffButton::dialogFinished, this, [this](const auto &changedEntries) {
        if (!changedEntries.isEmpty()) {
            m_serviceModel->clear();
            loadServices();
        }
    });
    topLayout->addWidget(downloadButton);
#endif // Q_OS_WIN

    m_enabledVcsPlugins = VersionControlSettings::enabledPlugins();
    std::sort(m_enabledVcsPlugins.begin(), m_enabledVcsPlugins.end());
}

ContextMenuSettingsPage::~ContextMenuSettingsPage()
{
}

bool ContextMenuSettingsPage::entryVisible(const QString &id)
{
    if (id == "add_to_places") {
        return ContextMenuSettings::showAddToPlaces();
    } else if (id == "sort") {
        return ContextMenuSettings::showSortBy();
    } else if (id == "view_mode") {
        return ContextMenuSettings::showViewMode();
    } else if (id == "open_in_new_tab") {
        return ContextMenuSettings::showOpenInNewTab();
    } else if (id == "open_in_new_window") {
        return ContextMenuSettings::showOpenInNewWindow();
    } else if (id == "copy_location") {
        return ContextMenuSettings::showCopyLocation();
    } else if (id == "duplicate") {
        return ContextMenuSettings::showDuplicateHere();
    } else if (id == "open_terminal_here") {
        return ContextMenuSettings::showOpenTerminal();
    } else if (id == "copy_to_inactive_split_view") {
        return ContextMenuSettings::showCopyToOtherSplitView();
    } else if (id == "move_to_inactive_split_view") {
        return ContextMenuSettings::showMoveToOtherSplitView();
    }
    return false;
}

void ContextMenuSettingsPage::setEntryVisible(const QString &id, bool visible)
{
    if (id == "add_to_places") {
        ContextMenuSettings::setShowAddToPlaces(visible);
    } else if (id == "sort") {
        ContextMenuSettings::setShowSortBy(visible);
    } else if (id == "view_mode") {
        ContextMenuSettings::setShowViewMode(visible);
    } else if (id == "open_in_new_tab") {
        ContextMenuSettings::setShowOpenInNewTab(visible);
    } else if (id == "open_in_new_window") {
        ContextMenuSettings::setShowOpenInNewWindow(visible);
    } else if (id == "copy_location") {
        ContextMenuSettings::setShowCopyLocation(visible);
    } else if (id == "duplicate") {
        ContextMenuSettings::setShowDuplicateHere(visible);
    } else if (id == "open_terminal_here") {
        ContextMenuSettings::setShowOpenTerminal(visible);
    } else if (id == "copy_to_inactive_split_view") {
        ContextMenuSettings::setShowCopyToOtherSplitView(visible);
    } else if (id == "move_to_inactive_split_view") {
        ContextMenuSettings::setShowMoveToOtherSplitView(visible);
    }
}

void ContextMenuSettingsPage::applySettings()
{
    if (!m_initialized) {
        return;
    }

    KConfig config(QStringLiteral("kservicemenurc"), KConfig::NoGlobals);
    KConfigGroup showGroup = config.group("Show");

    QStringList enabledPlugins;

    const QAbstractItemModel *model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        const QString service = model->data(index, ServiceModel::DesktopEntryNameRole).toString();
        const bool checked = model->data(index, Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;

        if (service.startsWith(VersionControlServicePrefix)) {
            if (checked) {
                enabledPlugins.append(model->data(index, Qt::DisplayRole).toString());
            }
        } else if (service == QLatin1String(DeleteService)) {
            KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
            KConfigGroup configGroup(globalConfig, "KDE");
            configGroup.writeEntry("ShowDeleteCommand", checked);
            configGroup.sync();
        } else if (service == QLatin1String(CopyToMoveToService)) {
            ContextMenuSettings::setShowCopyMoveMenu(checked);
            ContextMenuSettings::self()->save();
        } else if (m_actionIds.contains(service)) {
            setEntryVisible(service, checked);
            ContextMenuSettings::self()->save();
        } else {
            showGroup.writeEntry(service, checked);
        }
    }

    showGroup.sync();

    if (m_enabledVcsPlugins != enabledPlugins) {
        VersionControlSettings::setEnabledPlugins(enabledPlugins);
        VersionControlSettings::self()->save();

        if (!laterSelected) {
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            KMessageBox::ButtonCode promptRestart =
                KMessageBox::questionTwoActions(window(),
#else
            KMessageBox::ButtonCode promptRestart =
                KMessageBox::questionYesNo(window(),
#endif
                                                i18nc("@info",
                                                      "Dolphin must be restarted to apply the "
                                                      "updated version control system settings."),
                                                i18nc("@info", "Restart now?"),
                                                KGuiItem(QApplication::translate("KStandardGuiItem", "&Restart"), QStringLiteral("dialog-restart")),
                                                KGuiItem(QApplication::translate("KStandardGuiItem", "&Later"), QStringLiteral("dialog-later")));
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            if (promptRestart == KMessageBox::ButtonCode::PrimaryAction) {
#else
            if (promptRestart == KMessageBox::ButtonCode::Yes) {
#endif
                Dolphin::openNewWindow();
                qApp->quit();
            } else {
                laterSelected = true;
            }
        }
    }
}

void ContextMenuSettingsPage::restoreDefaults()
{
    QAbstractItemModel *model = m_listView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        const QString service = model->data(index, ServiceModel::DesktopEntryNameRole).toString();

        const bool checked =
            !service.startsWith(VersionControlServicePrefix) && service != QLatin1String(DeleteService) && service != QLatin1String(CopyToMoveToService);
        model->setData(index, checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
}

void ContextMenuSettingsPage::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !m_initialized) {
        loadServices();

        loadVersionControlSystems();

        // Add "Show 'Delete' command" as service
        KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::IncludeGlobals);
        KConfigGroup configGroup(globalConfig, "KDE");
        addRow(QStringLiteral("edit-delete"), i18nc("@option:check", "Delete"), DeleteService, configGroup.readEntry("ShowDeleteCommand", ShowDeleteDefault));

        // Add "Show 'Copy To' and 'Move To' commands" as service
        addRow(QStringLiteral("edit-copy"),
               i18nc("@option:check", "'Copy To' and 'Move To' commands"),
               CopyToMoveToService,
               ContextMenuSettings::showCopyMoveMenu());

        if (m_actions) {
            // Add other built-in actions
            for (const QString &id : m_actionIds) {
                const QAction *action = m_actions->action(id);
                if (action) {
                    addRow(action->icon().name(), KLocalizedString::removeAcceleratorMarker(action->text()), id, entryVisible(id));
                }
            }
        }

        m_sortModel->sort(Qt::DisplayRole);

        m_initialized = true;
    }
    SettingsPageBase::showEvent(event);
}

void ContextMenuSettingsPage::loadServices()
{
    const KConfig config(QStringLiteral("kservicemenurc"), KConfig::NoGlobals);
    const KConfigGroup showGroup = config.group("Show");

    // Load generic services
    const auto locations = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("kio/servicemenus"), QStandardPaths::LocateDirectory);
    QStringList files = KFileUtils::findAllUniqueFiles(locations);

    for (const auto &file : std::as_const(files)) {
        const QList<KServiceAction> serviceActions = KDesktopFileActions::userDefinedServices(KService(file), true);

        const KDesktopFile desktopFile(file);
        const QString subMenuName = desktopFile.desktopGroup().readEntry("X-KDE-Submenu");

        for (const KServiceAction &action : serviceActions) {
            const QString serviceName = action.name();
            const bool addService = !action.noDisplay() && !action.isSeparator() && !isInServicesList(serviceName);

            if (addService) {
                const QString itemName = subMenuName.isEmpty() ? action.text() : i18nc("@item:inmenu", "%1: %2", subMenuName, action.text());
                const bool checked = showGroup.readEntry(serviceName, true);
                addRow(action.icon(), itemName, serviceName, checked);
            }
        }
    }

    // Load JSON-based plugins that implement the KFileItemActionPlugin interface
    const auto jsonPlugins = KPluginMetaData::findPlugins(QStringLiteral("kf6/kfileitemaction"));

    for (const auto &jsonMetadata : jsonPlugins) {
        const QString desktopEntryName = jsonMetadata.pluginId();
        if (!isInServicesList(desktopEntryName)) {
            const bool checked = showGroup.readEntry(desktopEntryName, true);
            addRow(jsonMetadata.iconName(), jsonMetadata.name(), desktopEntryName, checked);
        }
    }

    m_sortModel->sort(Qt::DisplayRole);
    m_searchLineEdit->setFocus(Qt::OtherFocusReason);
}

void ContextMenuSettingsPage::loadVersionControlSystems()
{
    const QStringList enabledPlugins = VersionControlSettings::enabledPlugins();

    // Create a checkbox for each available version control plugin
    QSet<QString> loadedPlugins;

    const QVector<KPluginMetaData> plugins = KPluginMetaData::findPlugins(QStringLiteral("dolphin/vcs"));
    for (const auto &plugin : plugins) {
        const QString pluginName = plugin.name();
        addRow(QStringLiteral("code-class"), pluginName, VersionControlServicePrefix + pluginName, enabledPlugins.contains(pluginName));
        loadedPlugins += pluginName;
    }

    m_sortModel->sort(Qt::DisplayRole);
}

bool ContextMenuSettingsPage::isInServicesList(const QString &service) const
{
    for (int i = 0; i < m_serviceModel->rowCount(); ++i) {
        const QModelIndex index = m_serviceModel->index(i, 0);
        if (m_serviceModel->data(index, ServiceModel::DesktopEntryNameRole).toString() == service) {
            return true;
        }
    }
    return false;
}

void ContextMenuSettingsPage::addRow(const QString &icon, const QString &text, const QString &value, bool checked)
{
    m_serviceModel->insertRow(0);

    const QModelIndex index = m_serviceModel->index(0, 0);
    m_serviceModel->setData(index, icon, Qt::DecorationRole);
    m_serviceModel->setData(index, text, Qt::DisplayRole);
    m_serviceModel->setData(index, value, ServiceModel::DesktopEntryNameRole);
    m_serviceModel->setData(index, checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
}

#include "moc_contextmenusettingspage.cpp"
