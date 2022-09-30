/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinsettingsdialog.h"

#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"
#include "general/generalsettingspage.h"
#include "navigation/navigationsettingspage.h"
#include "contextmenu/contextmenusettingspage.h"
#include "startup/startupsettingspage.h"
#include "trash/trashsettingspage.h"
#include "viewmodes/viewsettingspage.h"
#include "config-dolphin.h"
#if HAVE_KUSERFEEDBACK
#include "userfeedback/dolphinfeedbackprovider.h"
#include "userfeedback/userfeedbacksettingspage.h"
#endif

#include <KAuthorized>
#include <KLocalizedString>
#include <KWindowConfig>
#include <KMessageBox>

#include <kwidgetsaddons_version.h>

#include <QCloseEvent>
#include <QPushButton>

DolphinSettingsDialog::DolphinSettingsDialog(const QUrl& url, QWidget* parent, KActionCollection* actions) :
    KPageDialog(parent),
    m_pages(),
    m_unsavedChanges(false)

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(540, minSize.height()));

    setFaceType(List);
    setWindowTitle(i18nc("@title:window", "Configure"));
    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok
            | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    box->button(QDialogButtonBox::Apply)->setEnabled(false);
    box->button(QDialogButtonBox::Ok)->setDefault(true);
    setButtonBox(box);

    connect(box->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, &DolphinSettingsDialog::applySettings);
    connect(box->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &DolphinSettingsDialog::applySettings);
    connect(box->button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, this, &DolphinSettingsDialog::restoreDefaults);

    // General
    GeneralSettingsPage* generalSettingsPage = new GeneralSettingsPage(url, this);
    KPageWidgetItem* generalSettingsFrame = addPage(generalSettingsPage,
                                                    i18nc("@title:group General settings", "General"));
    generalSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("system-file-manager")));
    connect(generalSettingsPage, &GeneralSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Startup
    StartupSettingsPage* startupSettingsPage = new StartupSettingsPage(url, this);
    KPageWidgetItem* startupSettingsFrame = addPage(startupSettingsPage,
                                                    i18nc("@title:group", "Startup"));
    startupSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-launch-feedback")));
    connect(startupSettingsPage, &StartupSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // View Modes
    ViewSettingsPage* viewSettingsPage = new ViewSettingsPage(this);
    KPageWidgetItem* viewSettingsFrame = addPage(viewSettingsPage,
                                                 i18nc("@title:group", "View Modes"));
    viewSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-icons")));
    connect(viewSettingsPage, &ViewSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Navigation
    NavigationSettingsPage* navigationSettingsPage = new NavigationSettingsPage(this);
    KPageWidgetItem* navigationSettingsFrame = addPage(navigationSettingsPage,
                                                       i18nc("@title:group", "Navigation"));
    navigationSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-navigation")));
    connect(navigationSettingsPage, &NavigationSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Context Menu
    auto contextMenuSettingsPage = new ContextMenuSettingsPage(this, actions, {
        QStringLiteral("add_to_places"),
        QStringLiteral("sort"),
        QStringLiteral("view_mode"),
        QStringLiteral("open_in_new_tab"),
        QStringLiteral("open_in_new_window"),
        QStringLiteral("copy_location"),
        QStringLiteral("duplicate"),
        QStringLiteral("open_terminal_here")
    });
    KPageWidgetItem* contextMenuSettingsFrame = addPage(contextMenuSettingsPage,
                                                        i18nc("@title:group", "Context Menu"));
    contextMenuSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-menu-edit")));
    connect(contextMenuSettingsPage, &ContextMenuSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Trash
    SettingsPageBase* trashSettingsPage = nullptr;
#ifndef Q_OS_WIN
    trashSettingsPage = createTrashSettingsPage(this);
#endif
    if (trashSettingsPage) {
        trashSettings = addPage(trashSettingsPage,
                                                     i18nc("@title:group", "Trash"));
        trashSettings->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
        connect(trashSettingsPage, &TrashSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);
    }

#if HAVE_KUSERFEEDBACK
    // User Feedback
    UserFeedbackSettingsPage* feedbackSettingsPage = nullptr;
    if (DolphinFeedbackProvider::instance()->isEnabled()) {
        feedbackSettingsPage = new UserFeedbackSettingsPage(this);
        auto feedbackSettingsFrame = addPage(feedbackSettingsPage, i18nc("@title:group", "User Feedback"));
        feedbackSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-locale")));
        connect(feedbackSettingsPage, &UserFeedbackSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);
    }
#endif

    m_pages.append(generalSettingsPage);
    m_pages.append(startupSettingsPage);
    m_pages.append(viewSettingsPage);
    m_pages.append(navigationSettingsPage);
    m_pages.append(contextMenuSettingsPage);
    if (trashSettingsPage) {
        m_pages.append(trashSettingsPage);
    }
#if HAVE_KUSERFEEDBACK
    if (feedbackSettingsPage) {
        m_pages.append(feedbackSettingsPage);
    }
#endif

    const KConfigGroup dialogConfig(KSharedConfig::openStateConfig(), "SettingsDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), dialogConfig);
}

DolphinSettingsDialog::~DolphinSettingsDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openStateConfig(), "SettingsDialog");
    KWindowConfig::saveWindowSize(windowHandle(), dialogConfig);
}

void DolphinSettingsDialog::enableApply()
{
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
    m_unsavedChanges = true;
}

void DolphinSettingsDialog::applySettings()
{
    for (SettingsPageBase* page : qAsConst(m_pages)) {
        page->applySettings();
    }

    Q_EMIT settingsChanged();

    GeneralSettings* settings = GeneralSettings::self();
    if (settings->modifiedStartupSettings()) {
        // Reset the modified startup settings hint. The changed startup settings
        // have been applied already due to emitting settingsChanged().
        settings->setModifiedStartupSettings(false);
        settings->save();
    }
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
    m_unsavedChanges = false;
}

void DolphinSettingsDialog::restoreDefaults()
{
    for (SettingsPageBase* page : qAsConst(m_pages)) {
        page->restoreDefaults();
    }
}

void DolphinSettingsDialog::closeEvent(QCloseEvent* event)
{
    if (!m_unsavedChanges) {
        event->accept();
        return;
    }

#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    const auto response = KMessageBox::warningTwoActionsCancel(this,
#else
    const auto response = KMessageBox::warningYesNoCancel(this,
#endif
                                        i18n("You have unsaved changes. Do you want to apply the changes or discard them?"),
                                        i18n("Warning"),
                                        KStandardGuiItem::save(),
                                        KStandardGuiItem::discard(),
                                        KStandardGuiItem::cancel());
    switch (response) {
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        case KMessageBox::PrimaryAction:
#else
        case KMessageBox::Yes:
#endif
            applySettings();
            Q_FALLTHROUGH();
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        case KMessageBox::SecondaryAction:
#else
        case KMessageBox::No:
#endif
            event->accept();
            break;
        case KMessageBox::Cancel:
            event->ignore();
            break;
        default:
            break;
    }
}


SettingsPageBase *DolphinSettingsDialog::createTrashSettingsPage(QWidget *parent)
{
    if (!KAuthorized::authorizeControlModule(QStringLiteral("kcmtrash.desktop"))) {
        return nullptr;
    }

    return new TrashSettingsPage(parent);
}
