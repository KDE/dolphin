/*
 * SPDX-FileCopyrightText: 2026 cafe quente <luanweslley77@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <servicemenushortcutmanager.h>

#include <KActionCollection>
#include <KDesktopFile>
#include <KDesktopFileAction>
#include <KFileItemActions>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

static constexpr QLatin1String probeFileName("shortcutmanagertest.desktop");
static constexpr QLatin1String probeActionKey("ProbeAction");

class ServiceMenuShortcutManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
#ifdef Q_OS_WIN
        QSKIP("Service menu discovery does not work on Windows");
#endif
        QStandardPaths::setTestModeEnabled(true);

        // Sandbox service menu discovery: KIO's test does the same with
        // QFINDTESTDATA + XDG_DATA_DIRS.
        m_dataDir.reset(new QTemporaryDir());
        QVERIFY(m_dataDir->isValid());
        QDir().mkpath(m_dataDir->path() + QStringLiteral("/kio/servicemenus"));
        QFile desktopFile(m_dataDir->path() + QStringLiteral("/kio/servicemenus/") + probeFileName);
        QVERIFY(desktopFile.open(QIODevice::WriteOnly | QIODevice::Text));
        desktopFile.write(
            "[Desktop Entry]\n"
            "Type=Service\n"
            "X-KDE-ServiceTypes=KonqPopupMenu/Plugin\n"
            "MimeType=text/plain;\n"
            "Actions=ProbeAction;\n"
            "X-KDE-Priority=TopLevel\n"
            "\n"
            "[Desktop Action ProbeAction]\n"
            "Name=Shortcut Manager Probe\n"
            "Icon=system-run\n"
            "Exec=true\n");
        desktopFile.close();
        qputenv("XDG_DATA_DIRS", m_dataDir->path().toUtf8());

        m_desktopFilePath = desktopFile.fileName();
    }

    void testRefreshPreservesShortcuts()
    {
        KActionCollection collection(this);
        ServiceMenuShortcutManager manager(&collection);
        KFileItemActions fileItemActions;
        manager.refresh(&fileItemActions);

        const QString name = registeredProbeName();
        QAction *templateAction = collection.action(name);
        QVERIFY2(templateAction, qPrintable(name));

        // Simulate a user-assigned shortcut, then refresh (e.g. triggered by
        // applying the context menu settings, which rewrites kservicemenurc).
        templateAction->setShortcuts({QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T)});
        manager.refresh(&fileItemActions);

        QAction *replacement = collection.action(name);
        QVERIFY(replacement);
        // The refresh may rebuild the actions or re-register the same ones,
        // depending on the platform; the shortcut must survive either way.
        QCOMPARE(replacement->shortcuts(), QList<QKeySequence>({QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T)}));
    }

private:
    QString registeredProbeName() const
    {
        return QStringLiteral("servicemenu_") + probeFileName + QStringLiteral("::") + probeActionKey;
    }

    QScopedPointer<QTemporaryDir> m_dataDir;
    QString m_desktopFilePath;
};

QTEST_MAIN(ServiceMenuShortcutManagerTest)

#include "servicemenushortcutmanagertest.moc"
