/*
 * SPDX-FileCopyrightText: 2025 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "setfoldericonitemaction.h"
#include "../dolphindebug.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KFileItem>
#include <KLocalizedString>
#include <KPluginFactory>
#ifdef QT_DBUS_LIB
#include <KDirNotify>
#endif

#include <QActionGroup>
#include <QBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QUrl>
#include <QWidgetAction>

K_PLUGIN_CLASS_WITH_JSON(SetFolderIconItemAction, "setfoldericonitemaction.json")

namespace
{
bool isDefaultFolderIcon(const QString &iconName)
{
    return iconName.isEmpty() || iconName == QLatin1String("folder") || iconName == QLatin1String("inode-directory");
}
}

SetFolderIconItemAction::SetFolderIconItemAction(QObject *parent)
    : KAbstractFileItemActionPlugin(parent)
{
}

void SetFolderIconItemAction::setFolderIcon(bool check)
{
    QAction *action = qobject_cast<QAction *>(sender());
    Q_ASSERT(action);

    action->setChecked(check);

    auto iconName = action->icon().name();

    // Apply custom folder icon, if applicable.
    const QString fileName = m_localUrl.toLocalFile() + QLatin1String("/.directory");
    KDesktopFile desktopFile{fileName};

    if (check && !isDefaultFolderIcon(iconName)) {
        desktopFile.desktopGroup().writeEntry(QStringLiteral("Icon"), iconName);
    } else {
        desktopFile.desktopGroup().deleteEntry(QStringLiteral("Icon"));
        if (desktopFile.desktopGroup().entryMap().isEmpty() && QFile::exists(fileName)) {
            // clean file
            QFile::remove(fileName);
        }
    }

#ifdef QT_DBUS_LIB
    org::kde::KDirNotify::emitFilesChanged({m_localUrl});
#endif
}

class ButtonsWithSubMenuWidgetAction : public QWidgetAction
{
public:
    ButtonsWithSubMenuWidgetAction(const QList<QAction *> actions, QMenu *subMenu, QObject *parent)
        : QWidgetAction(parent)
        , m_actions(actions)
        , m_subMenu(subMenu)
    {
    }

    QWidget *createWidget(QWidget *parent) override
    {
        QWidget *widget = new QWidget(parent);
        m_layout = new QHBoxLayout(widget);

        for (const auto action : std::as_const(m_actions)) {
            action->setParent(this);

            auto p = new QPushButton(widget);

            p->setIcon(action->icon());
            p->setCheckable(true);
            p->setChecked(action->isChecked());
            p->setToolTip(action->text());

            connect(p, &QPushButton::clicked, action, &QAction::triggered);
            connect(action, &QAction::toggled, p, &QPushButton::setChecked);

            m_layout->addWidget(p);
        }

        auto p = new QPushButton(widget);
        p->setText(i18nc("@action open a submeun with additional entries", "Other"));
        p->setMenu(m_subMenu);
        m_layout->addWidget(p);

        return widget;
    }

    QList<QAction *> m_actions;
    QHBoxLayout *m_layout;
    QMenu *m_subMenu;
};

QList<QAction *> SetFolderIconItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    if (fileItemInfos.items().count() != 1) {
        return {};
    }

    auto fileItem = fileItemInfos.items().at(0);

    bool local;
    m_localUrl = fileItem.mostLocalUrl(&local);
    if (!local) {
        return {};
    }

    using StringPair = QPair<QString, QString>;
    // keep in sync with kio/src/filewidgets/knewfilemenu.cpp
    const QList<StringPair> icons = {// colors.
                                     // default folder icon goes here.
                                     StringPair{i18n("Red"), QStringLiteral("folder-red")},
                                     StringPair{i18n("Yellow"), QStringLiteral("folder-yellow")},
                                     StringPair{i18n("Orange"), QStringLiteral("folder-orange")},
                                     StringPair{i18n("Green"), QStringLiteral("folder-green")},
                                     StringPair{i18n("Cyan"), QStringLiteral("folder-cyan")},
                                     StringPair{i18n("Blue"), QStringLiteral("folder-blue")},
                                     StringPair{i18n("Violet"), QStringLiteral("folder-violet")},
                                     StringPair{i18n("Brown"), QStringLiteral("folder-brown")},
                                     StringPair{i18n("Grey"), QStringLiteral("folder-grey")},

                                     // emblems.
                                     StringPair{i18n("Bookmark"), QStringLiteral("folder-bookmark")},
                                     StringPair{i18n("Cloud"), QStringLiteral("folder-cloud")},
                                     StringPair{i18n("Development"), QStringLiteral("folder-development")},
                                     StringPair{i18n("Games"), QStringLiteral("folder-games")},
                                     StringPair{i18n("Mail"), QStringLiteral("folder-mail")},
                                     StringPair{i18n("Music"), QStringLiteral("folder-music")},
                                     StringPair{i18n("Print"), QStringLiteral("folder-print")},
                                     StringPair{i18n("Compressed"), QStringLiteral("folder-tar")},
                                     StringPair{i18n("Temporary"), QStringLiteral("folder-temp")},
                                     StringPair{i18n("Important"), QStringLiteral("folder-important")}};

    QActionGroup *actiongroup = new QActionGroup(parentWidget);
    actiongroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);

    int i = 0;
    QMenu *subMenu = new QMenu(parentWidget);
    QList<QAction *> actions;
    const auto fileIconName = fileItem.iconName();
    for (const auto &[name, iconName] : icons) {
        auto icon = QIcon::fromTheme(iconName);
        if (icon.isNull()) {
            qCWarning(DolphinDebug) << "SetFolderIconItemAction Missing icon:" << iconName;
            continue;
        }

        QAction *folderIconAction = new QAction(name);
        folderIconAction->setIcon(icon);
        folderIconAction->setCheckable(true);
        folderIconAction->setChecked(fileIconName == iconName);
        actiongroup->addAction(folderIconAction);

        connect(folderIconAction, &QAction::triggered, this, &SetFolderIconItemAction::setFolderIcon);
        connect(folderIconAction, &QAction::triggered, parentWidget, &QWidget::close);

        ++i;
        if (i < 6) {
            actions.append(folderIconAction);
        } else {
            folderIconAction->setParent(subMenu);
            subMenu->addAction(folderIconAction);
        }
    }

    QWidgetAction *action = new ButtonsWithSubMenuWidgetAction(actions, subMenu, this);

    return {action};
}

#include "moc_setfoldericonitemaction.cpp"
#include "setfoldericonitemaction.moc"
