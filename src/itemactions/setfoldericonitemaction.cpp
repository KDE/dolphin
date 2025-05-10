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

#include <QMenu>
#include <QUrl>

K_PLUGIN_CLASS_WITH_JSON(SetFolderIconItemAction, "setfoldericonitemaction.json")

namespace
{
bool isDefaultFolderIcon(const QString &iconName)
{
    return iconName.isEmpty() || iconName == QLatin1String("folder") || iconName == QLatin1String("inode-directory");
}
}

class SetFolderIconItemActionPrivate : public QObject
{
    Q_OBJECT

public:
    SetFolderIconItemActionPrivate(QObject *parent)
        : QObject(parent)
    {
    }

    void setFolderIcon(bool check);
    QUrl m_localUrl;
};

SetFolderIconItemAction::SetFolderIconItemAction(QObject *parent)
    : KAbstractFileItemActionPlugin(parent)
    , d(new SetFolderIconItemActionPrivate(this))
{
}

void SetFolderIconItemActionPrivate::setFolderIcon(bool check)
{
    QAction *action = qobject_cast<QAction *>(sender());
    Q_ASSERT(action);

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

QList<QAction *> SetFolderIconItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    if (fileItemInfos.items().count() != 1) {
        return {};
    }

    auto fileItem = fileItemInfos.items().at(0);

    bool local;
    d->m_localUrl = fileItem.mostLocalUrl(&local);
    if (!local) {
        return {};
    }

    QAction *setFolderIconMenu = new QAction(i18nc("@action:inmenu", "Set folder Icon"), parentWidget);
    setFolderIconMenu->setIcon(QIcon::fromTheme(QStringLiteral("folder")));

    setFolderIconMenu->setMenu(new QMenu(parentWidget));

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

    const auto fileIconName = fileItem.iconName();
    for (const auto &[name, iconName] : icons) {
        auto icon = QIcon::fromTheme(iconName);
        if (icon.isNull()) {
            qCWarning(DolphinDebug) << "SetFolderIconItemAction Missing icon:" << iconName;
            continue;
        }

        QAction *folderIcon = new QAction(name, parentWidget);
        folderIcon->setIcon(icon);
        folderIcon->setCheckable(true);
        folderIcon->setChecked(fileIconName == iconName);

        setFolderIconMenu->menu()->addAction(folderIcon);
        connect(folderIcon, &QAction::triggered, d, &SetFolderIconItemActionPrivate::setFolderIcon);
    }

    return {setFolderIconMenu};
}

#include "moc_setfoldericonitemaction.cpp"
#include "setfoldericonitemaction.moc"
