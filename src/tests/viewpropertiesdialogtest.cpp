/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settings/viewpropertiesdialog.h"
#include "dolphin_generalsettings.h"
#include "settings/applyviewpropsjob.h"
#include "testdir.h"
#include "views/dolphinview.h"
#include "views/viewproperties.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPointer>
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
    void testTheDialogLocksTheCurrentFolderWhileFoldersShareOneStyle();
    void testApplyingToSubFoldersLocksThemWhileFoldersShareOneStyle();
};

namespace
{
// The view mode of the style all folders share, set for a test and put back after it.
DolphinView::Mode setSharedViewMode(DolphinView::Mode mode)
{
    ViewProperties shared{QUrl()};
    const DolphinView::Mode previous = shared.viewMode();
    shared.setViewMode(mode);
    shared.save();
    return previous;
}

QRadioButton *radioButton(const QDialog &dialog, const QString &text)
{
    const auto buttons = dialog.findChildren<QRadioButton *>();
    for (QRadioButton *button : buttons) {
        if (button->text().remove(QLatin1Char('&')) == text) {
            return button;
        }
    }
    return nullptr;
}

void chooseViewMode(const QDialog &dialog, DolphinView::Mode mode)
{
    // The view mode is the first combo box of the dialog.
    auto *viewMode = dialog.findChildren<QComboBox *>().constFirst();
    viewMode->setCurrentIndex(viewMode->findData(mode));
}

void clickApply(const QDialog &dialog)
{
    dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
}
}

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

// While all folders share one style, the dialog of a folder that follows that style changes it by
// default. "Current folder" locks the folder with the style of the dialog instead.
void ViewPropertiesDialogTest::testTheDialogLocksTheCurrentFolderWhileFoldersShareOneStyle()
{
    const bool globalViewProps = GeneralSettings::globalViewProps();
    GeneralSettings::setGlobalViewProps(true);
    const DolphinView::Mode previousShared = setSharedViewMode(DolphinView::IconsView);
    auto restore = qScopeGuard([globalViewProps, previousShared] {
        setSharedViewMode(previousShared);
        GeneralSettings::setGlobalViewProps(globalViewProps);
        GeneralSettings::self()->save();
    });

    TestDir dir(QDir::homePath() + QStringLiteral("/.viewPropertiesDialogTest-"));
    DolphinView view(dir.url(), nullptr);

    {
        ViewPropertiesDialog dialog(&view);
        // "All folders" already is the default style, so there is no toggle for it.
        const auto checkBoxes = dialog.findChildren<QCheckBox *>();
        QVERIFY(std::none_of(checkBoxes.cbegin(), checkBoxes.cend(), [](const QCheckBox *box) {
            return box->text().remove(QLatin1Char('&')) == QStringLiteral("Use as default view settings");
        }));
        QVERIFY(radioButton(dialog, QStringLiteral("All folders"))->isChecked());

        chooseViewMode(dialog, DolphinView::CompactView);
        clickApply(dialog);
    }
    QCOMPARE(ViewProperties(QUrl()).viewMode(), DolphinView::CompactView);
    QVERIFY(!ViewProperties(dir.url()).isLocked());

    {
        ViewPropertiesDialog dialog(&view);
        radioButton(dialog, QStringLiteral("Current folder"))->click();
        chooseViewMode(dialog, DolphinView::DetailsView);
        clickApply(dialog);
    }
    const ViewProperties folderProps(dir.url());
    QVERIFY(folderProps.isLocked());
    QCOMPARE(folderProps.viewMode(), DolphinView::DetailsView);
    QCOMPARE(ViewProperties(QUrl()).viewMode(), DolphinView::CompactView);

    // The dialog of a locked folder changes that folder by default.
    ViewPropertiesDialog dialog(&view);
    QVERIFY(radioButton(dialog, QStringLiteral("Current folder"))->isChecked());
}

// While all folders share one style, the folders that a style is applied to keep it by being locked.
// The shared style does not change.
void ViewPropertiesDialogTest::testApplyingToSubFoldersLocksThemWhileFoldersShareOneStyle()
{
    const bool globalViewProps = GeneralSettings::globalViewProps();
    GeneralSettings::setGlobalViewProps(true);
    const DolphinView::Mode previousShared = setSharedViewMode(DolphinView::IconsView);
    auto restore = qScopeGuard([globalViewProps, previousShared] {
        setSharedViewMode(previousShared);
        GeneralSettings::setGlobalViewProps(globalViewProps);
        GeneralSettings::self()->save();
    });

    TestDir dir(QDir::homePath() + QStringLiteral("/.viewPropertiesDialogTest-"));
    dir.createDir(QStringLiteral("tree/a/deep"));
    dir.createDir(QStringLiteral("other"));
    auto urlOf = [&dir](const QString &path) {
        return QUrl::fromLocalFile(dir.path() + QLatin1Char('/') + path);
    };

    ViewProperties style{QUrl()};
    style.setAutoSaveEnabled(false);
    style.setViewMode(DolphinView::DetailsView);
    QPointer<ApplyViewPropsJob> job = new ApplyViewPropsJob(urlOf(QStringLiteral("tree")), style);
    // The job writes the folder it was given when it is deleted, after its result.
    QTRY_VERIFY_WITH_TIMEOUT(!job, 10000);

    for (const QString &path : {QStringLiteral("tree"), QStringLiteral("tree/a"), QStringLiteral("tree/a/deep")}) {
        const ViewProperties props(urlOf(path));
        QVERIFY2(props.isLocked(), qPrintable(path));
        QCOMPARE(props.viewMode(), DolphinView::DetailsView);
    }
    QCOMPARE(ViewProperties(QUrl()).viewMode(), DolphinView::IconsView);
    const ViewProperties other(urlOf(QStringLiteral("other")));
    QVERIFY(!other.isLocked());
    QCOMPARE(other.viewMode(), DolphinView::IconsView);
}

QTEST_MAIN(ViewPropertiesDialogTest)

#include "viewpropertiesdialogtest.moc"
