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
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
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
    org::kde::KDirNotify::emitFilesChanged({m_url});
#endif
}

class ButtonsWithSubMenuWidgetAction : public QWidgetAction
{
public:
    ButtonsWithSubMenuWidgetAction(QMenu *subMenu, QWidget *parentWidget)
        : QWidgetAction(parentWidget)
        , m_subMenu(subMenu)
    {
    }

    void setActions(const QList<QAction *> actions)
    {
        m_actions = actions;
    }

    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) {
            const QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            auto widget = qobject_cast<QWidget *>(object);

            if (keyEvent->keyCombination() == QKeyCombination(Qt::Modifier::SHIFT, Qt::Key_Backtab) || keyEvent->key() == Qt::Key_Left
                || keyEvent->key() == Qt::Key_Up) {
                auto previous = widget->previousInFocusChain();
                if (previous == widget->parentWidget()) {
                    // the next object is the parent, let the focus bubble up
                    return false;
                }

                previous->setFocus(Qt::BacktabFocusReason);
                event->accept();
                return true;
            }

            if (keyEvent->keyCombination() == QKeyCombination(Qt::Key_Tab) || keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Down) {
                auto next = widget->nextInFocusChain();
                if (next->parentWidget() != widget->parentWidget()) {
                    // the next object is not a sibling, let the focus bubble up
                    return false;
                }

                next->setFocus(Qt::TabFocusReason);
                event->accept();
                return true;
            }
        }

        // TODO implement proper SHIFT+TAB
        // See https://bugreports.qt.io/browse/QTBUG-137298

        return false;
    }

    QWidget *createWidget(QWidget *parent) override
    {
        QWidget *widget = new QWidget(parent);
        auto layout = new QHBoxLayout(widget);

        bool firstAction = false;
        for (const auto action : std::as_const(m_actions)) {
            if (!action->parent()) {
                action->setParent(widget);
            }

            auto p = new QPushButton(widget);

            p->setIcon(action->icon());
            p->setCheckable(true);
            p->setChecked(action->isChecked());
            p->setToolTip(action->toolTip());
            p->installEventFilter(this);

            connect(p, &QPushButton::clicked, action, &QAction::triggered);
            connect(action, &QAction::toggled, p, &QPushButton::setChecked);

            layout->addWidget(p);

            if (!firstAction) {
                widget->setFocusProxy(p);
                firstAction = true;
            }
        }

        auto p = new QPushButton(widget);
        p->setText(i18nc("@action open a submenu with additional entries", "Other"));
        p->setToolTip(i18nc("@label", "Other folder icon options"));
        p->setMenu(m_subMenu);
        layout->addWidget(p);
        p->installEventFilter(this);

        widget->setFocusPolicy(Qt::StrongFocus);

        return widget;
    }

    QList<QAction *> m_actions;
    QMenu *m_subMenu;
};

QList<QAction *> SetFolderIconItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    if (fileItemInfos.items().count() != 1) {
        return {};
    }

    auto fileItem = fileItemInfos.items().at(0);
    m_url = fileItem.url();

    bool local;
    m_localUrl = fileItem.mostLocalUrl(&local);
    if (!local || !fileItemInfos.supportsWriting() || !fileItem.isWritable()) {
        return {};
    }
    const short s_numberOfEntriesVisible = 5;

    using StringPair = QPair<KLocalizedString, QString>;
    // keep in sync with kio/src/filewidgets/knewfilemenu.cpp
    // default folder icon goes here.
    const QList<StringPair> icons = {// colors.
                                     StringPair{ki18nc("@label as in default folder color", "Red"), QStringLiteral("folder-red")},
                                     StringPair{ki18nc("@label as in default folder color", "Yellow"), QStringLiteral("folder-yellow")},
                                     StringPair{ki18nc("@label as in default folder color", "Orange"), QStringLiteral("folder-orange")},
                                     StringPair{ki18nc("@label as in default folder color", "Green"), QStringLiteral("folder-green")},
                                     StringPair{ki18nc("@label as in default folder color", "Cyan"), QStringLiteral("folder-cyan")},
                                     // must match s_numberOfEntriesVisible
                                     StringPair{ki18nc("@label: as in default folder icon", "Default"), QStringLiteral("inode-directory")},

                                     StringPair{ki18nc("@label as in default folder color", "Blue"), QStringLiteral("folder-blue")},
                                     StringPair{ki18nc("@label as in default folder color", "Violet"), QStringLiteral("folder-violet")},
                                     StringPair{ki18nc("@label as in default folder color", "Brown"), QStringLiteral("folder-brown")},
                                     StringPair{ki18nc("@label as in default folder color", "Grey"), QStringLiteral("folder-grey")},

                                     // emblems.
                                     StringPair{ki18nc("@label as in default folder color", "Bookmark"), QStringLiteral("folder-bookmark")},
                                     StringPair{ki18nc("@label as in default folder color", "Cloud"), QStringLiteral("folder-cloud")},
                                     StringPair{ki18nc("@label as in default folder color", "Development"), QStringLiteral("folder-development")},
                                     StringPair{ki18nc("@label as in default folder color", "Games"), QStringLiteral("folder-games")},
                                     StringPair{ki18nc("@label as in default folder color", "Mail"), QStringLiteral("folder-mail")},
                                     StringPair{ki18nc("@label as in default folder color", "Music"), QStringLiteral("folder-music")},
                                     StringPair{ki18nc("@label as in default folder color", "Print"), QStringLiteral("folder-print")},
                                     StringPair{ki18nc("@label as in default folder color", "Compressed"), QStringLiteral("folder-tar")},
                                     StringPair{ki18nc("@label as in default folder color", "Temporary"), QStringLiteral("folder-temp")},
                                     StringPair{ki18nc("@label as in default folder color", "Important"), QStringLiteral("folder-important")}};

    QActionGroup *actiongroup = new QActionGroup(this);
    actiongroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);

    QMenu *subMenu = new QMenu();
    auto action = new ButtonsWithSubMenuWidgetAction(subMenu, parentWidget);

    int i = 0;
    QList<QAction *> actions;
    const auto fileIconName = fileItem.iconName();
    for (const auto &[name, iconName] : icons) {
        auto icon = QIcon::fromTheme(iconName);
        if (icon.isNull()) {
            qCWarning(DolphinDebug) << "SetFolderIconItemAction Missing icon:" << iconName;
            continue;
        }

        QAction *folderIconAction = new QAction(KLocalizedString(name).toString(), parentWidget);
        folderIconAction->setIcon(icon);
        folderIconAction->setCheckable(true);
        folderIconAction->setChecked(fileIconName == iconName);
        folderIconAction->setToolTip(i18nc("@label %1 is a folder icon name (Red, Music...) etc", "Set folder icon to %1", folderIconAction->iconText()));
        actiongroup->addAction(folderIconAction);

        connect(folderIconAction, &QAction::triggered, this, &SetFolderIconItemAction::setFolderIcon);
        connect(folderIconAction, &QAction::triggered, action, &QAction::triggered);

        ++i;
        if (i < s_numberOfEntriesVisible + 1) {
            actions.append(folderIconAction);
        } else {
            folderIconAction->setParent(subMenu);
            subMenu->addAction(folderIconAction);
        }
    }
    action->setActions(actions);

    return {action};
}

#include "moc_setfoldericonitemaction.cpp"
#include "setfoldericonitemaction.moc"
