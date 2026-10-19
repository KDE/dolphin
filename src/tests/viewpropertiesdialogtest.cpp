/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settings/viewpropertiesdialog.h"
#include "dolphin_generalsettings.h"
#include "testdir.h"
#include "views/dolphinview.h"
#include "views/viewproperties.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class ViewPropertiesDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testTheDefaultStyleReachesFoldersWithoutAStyleOfTheirOwn();
};

void ViewPropertiesDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

// In per-folder mode, the dialog opened without a view edits the style that a folder is shown with
// until it has one of its own. A folder that has one keeps it.
void ViewPropertiesDialogTest::testTheDefaultStyleReachesFoldersWithoutAStyleOfTheirOwn()
{
    const bool globalViewProps = GeneralSettings::globalViewProps();
    auto restore = qScopeGuard([globalViewProps] {
        GeneralSettings::setGlobalViewProps(globalViewProps);
        GeneralSettings::self()->save();
    });
    GeneralSettings::setGlobalViewProps(false);
    QVERIFY(GeneralSettings::self()->save());

    // The style every folder starts from is kept between runs, so it starts from a known mode here.
    DolphinView::Mode previousDefault;
    {
        ViewProperties defaults{QUrl()};
        previousDefault = defaults.viewMode();
        defaults.setViewMode(DolphinView::IconsView);
        defaults.save();
    }
    auto restoreDefault = qScopeGuard([previousDefault] {
        ViewProperties defaults{QUrl()};
        defaults.setViewMode(previousDefault);
        defaults.save();
    });

    TestDir ownStyleDir(QDir::homePath() + QStringLiteral("/.viewPropertiesDialogTest-"));
    TestDir plainDir(QDir::homePath() + QStringLiteral("/.viewPropertiesDialogTest-"));
    {
        ViewProperties props(ownStyleDir.url());
        props.setViewMode(DolphinView::CompactView);
    }

    ViewPropertiesDialog dialog(static_cast<QWidget *>(nullptr));
    // The choice of the folders that this style applies to only makes sense for one folder.
    QVERIFY(dialog.findChildren<QRadioButton *>().isEmpty());

    // The view mode is the first combo box of the dialog.
    auto *viewMode = dialog.findChildren<QComboBox *>().constFirst();
    const int details = viewMode->findData(DolphinView::DetailsView);
    QVERIFY(details >= 0);
    viewMode->setCurrentIndex(details);

    // Apply writes the style and reports it, so the open views can show it while the dialog stays open.
    QSignalSpy defaultChanged(&dialog, &ViewPropertiesDialog::defaultPropertiesChanged);
    dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
    QCOMPARE(defaultChanged.count(), 1);
    QCOMPARE(ViewProperties(QUrl()).viewMode(), DolphinView::DetailsView);
    QCOMPARE(ViewProperties(plainDir.url()).viewMode(), DolphinView::DetailsView);
    QCOMPARE(ViewProperties(ownStyleDir.url()).viewMode(), DolphinView::CompactView);
}

QTEST_MAIN(ViewPropertiesDialogTest)

#include "viewpropertiesdialogtest.moc"
