/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinsettingsdialog.h"

#include "config-dolphin.h"
#include "contextmenu/contextmenusettingspage.h"
#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"
#include "interface/interfacesettingspage.h"
#include "trash/trashsettingspage.h"
#include "viewmodes/viewsettingspage.h"
#if HAVE_KUSERFEEDBACK
#include "userfeedback/dolphinfeedbackprovider.h"
#include "userfeedback/userfeedbacksettingspage.h"
#endif

#include <KAuthorized>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRuntimePlatform>
#include <KWindowConfig>

#include <kwidgetsaddons_version.h>

#include <QCloseEvent>
#include <QPushButton>

DolphinSettingsDialog::DolphinSettingsDialog(const QUrl &url, QWidget *parent, KActionCollection *actions)
    : KPageDialog(parent)
    , m_pages()
    , m_unsavedChanges(false)

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(540, minSize.height()));

    setFaceType(KRuntimePlatform::runtimePlatform().contains(QLatin1String("phone")) ? Tabbed : List);
    setWindowTitle(i18nc("@title:window", "Configure"));

    // Interface
    InterfaceSettingsPage *interfaceSettingsPage = new InterfaceSettingsPage(this);
    KPageWidgetItem *interfaceSettingsFrame = addPage(interfaceSettingsPage, i18nc("@title:group Interface settings", "Interface"));
    interfaceSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("org.kde.dolphin")));
    connect(interfaceSettingsPage, &InterfaceSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // View
    ViewSettingsPage *viewSettingsPage = new ViewSettingsPage(url, this);
    KPageWidgetItem *viewSettingsFrame = addPage(viewSettingsPage, i18nc("@title:group", "View"));
    viewSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-icons")));
    connect(viewSettingsPage, &ViewSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Context Menu
    auto contextMenuSettingsPage = new ContextMenuSettingsPage(this,
                                                               actions,
                                                               {QStringLiteral("add_to_places"),
                                                                QStringLiteral("sort"),
                                                                QStringLiteral("view_mode"),
                                                                QStringLiteral("open_in_new_tab"),
                                                                QStringLiteral("open_in_new_window"),
                                                                QStringLiteral("open_in_split_view"),
                                                                QStringLiteral("copy_location"),
                                                                QStringLiteral("duplicate"),
                                                                QStringLiteral("open_terminal_here"),
                                                                QStringLiteral("copy_to_inactive_split_view"),
                                                                QStringLiteral("move_to_inactive_split_view")});
    KPageWidgetItem *contextMenuSettingsFrame = addPage(contextMenuSettingsPage, i18nc("@title:group", "Context Menu"));
    contextMenuSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-menu-edit")));
    connect(contextMenuSettingsPage, &ContextMenuSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Trash
    SettingsPageBase *trashSettingsPage = nullptr;
#ifndef Q_OS_WIN
    trashSettingsPage = createTrashSettingsPage(this);
#endif
    if (trashSettingsPage) {
        trashSettings = addPage(trashSettingsPage, i18nc("@title:group", "Trash"));
        trashSettings->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
        connect(trashSettingsPage, &TrashSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);
    }

#if HAVE_KUSERFEEDBACK
    // User Feedback
    UserFeedbackSettingsPage *feedbackSettingsPage = nullptr;
    if (DolphinFeedbackProvider::instance()->isEnabled()) {
        feedbackSettingsPage = new UserFeedbackSettingsPage(this);
        auto feedbackSettingsFrame = addPage(feedbackSettingsPage, i18nc("@title:group", "User Feedback"));
        feedbackSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-locale")));
        connect(feedbackSettingsPage, &UserFeedbackSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);
    }
#endif

    m_pages.append(interfaceSettingsPage);
    m_pages.append(viewSettingsPage);
    m_pages.append(contextMenuSettingsPage);
    if (trashSettingsPage) {
        m_pages.append(trashSettingsPage);
    }
#if HAVE_KUSERFEEDBACK
    if (feedbackSettingsPage) {
        m_pages.append(feedbackSettingsPage);
    }
#endif

    // Create the buttons last so they are also last in the keyboard Tab focus order.
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    box->button(QDialogButtonBox::Apply)->setEnabled(false);
    box->button(QDialogButtonBox::Ok)->setDefault(true);
    setButtonBox(box);

    connect(box->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, &DolphinSettingsDialog::applySettings);
    connect(box->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &DolphinSettingsDialog::applySettings);
    connect(box->button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, this, &DolphinSettingsDialog::restoreDefaults);

    const KConfigGroup dialogConfig(KSharedConfig::openStateConfig(), QStringLiteral("SettingsDialog"));
    KWindowConfig::restoreWindowSize(windowHandle(), dialogConfig);
}

DolphinSettingsDialog::~DolphinSettingsDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openStateConfig(), QStringLiteral("SettingsDialog"));
    KWindowConfig::saveWindowSize(windowHandle(), dialogConfig);
}

void DolphinSettingsDialog::enableApply()
{
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
    m_unsavedChanges = true;
}

void DolphinSettingsDialog::applySettings()
{
    for (SettingsPageBase *page : std::as_const(m_pages)) {
        page->applySettings();
    }

    Q_EMIT settingsChanged();

    GeneralSettings *settings = GeneralSettings::self();
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
    for (SettingsPageBase *page : std::as_const(m_pages)) {
        page->restoreDefaults();
    }
}

void DolphinSettingsDialog::closeEvent(QCloseEvent *event)
{
    if (!m_unsavedChanges) {
        event->accept();
        return;
    }

    const auto response = KMessageBox::warningTwoActionsCancel(this,
                                                               i18n("You have unsaved changes. Do you want to apply the changes or discard them?"),
                                                               i18n("Warning"),
                                                               KStandardGuiItem::save(),
                                                               KStandardGuiItem::discard(),
                                                               KStandardGuiItem::cancel());
    switch (response) {
    case KMessageBox::PrimaryAction:
        applySettings();
        Q_FALLTHROUGH();
    case KMessageBox::SecondaryAction:
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

#include "moc_dolphinsettingsdialog.cpp"
