/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "confirmationssettingspage.h"

#include "dolphin_generalsettings.h"
#include "global.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace
{
enum ScriptExecution { AlwaysAsk = 0, Open = 1, Execute = 2 };

const bool ConfirmEmptyTrash = true;
const bool ConfirmTrash = false;
const bool ConfirmDelete = true;
const int ConfirmScriptExecution = ScriptExecution::AlwaysAsk;
}

ConfirmationsSettingsPage::ConfirmationsSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_confirmMoveToTrash(nullptr)
    , m_confirmEmptyTrash(nullptr)
    , m_confirmDelete(nullptr)
    ,

#if HAVE_TERMINAL
    m_confirmClosingTerminalRunningProgram(nullptr)
    ,
#endif

    m_confirmClosingMultipleTabs(nullptr)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);

    QLabel *confirmLabelKde = new QLabel(i18nc("@title:group", "Ask for confirmation in all KDE applications when:"), this);
    confirmLabelKde->setWordWrap(true);

    m_confirmMoveToTrash = new QCheckBox(i18nc("@option:check Ask for confirmation when", "Moving files or folders to trash"), this);
    m_confirmEmptyTrash = new QCheckBox(i18nc("@option:check Ask for confirmation when", "Emptying trash"), this);
    m_confirmDelete = new QCheckBox(i18nc("@option:check Ask for confirmation when", "Deleting files or folders"), this);

    QLabel *confirmLabelDolphin = new QLabel(i18nc("@title:group", "Ask for confirmation in Dolphin when:"), this);
    confirmLabelDolphin->setWordWrap(true);

    m_confirmClosingMultipleTabs = new QCheckBox(i18nc("@option:check Ask for confirmation in Dolphin when", "Closing windows with multiple tabs"), this);

#if HAVE_TERMINAL
    m_confirmClosingTerminalRunningProgram =
        new QCheckBox(i18nc("@option:check Ask for confirmation when", "Closing windows with a program running in the Terminal panel"), this);
#endif

    QHBoxLayout *executableScriptLayout = new QHBoxLayout();
    QLabel *executableScriptLabel = new QLabel(i18nc("@title:group", "When opening an executable file:"), this);
    confirmLabelKde->setWordWrap(true);
    executableScriptLayout->addWidget(executableScriptLabel);

    m_confirmScriptExecution = new QComboBox(this);
    m_confirmScriptExecution->addItems({i18n("Always ask"), i18n("Open in application"), i18n("Run script")});
    executableScriptLayout->addWidget(m_confirmScriptExecution);

    topLayout->addWidget(confirmLabelKde);
    topLayout->addWidget(m_confirmMoveToTrash);
    topLayout->addWidget(m_confirmEmptyTrash);
    topLayout->addWidget(m_confirmDelete);
    topLayout->addSpacing(Dolphin::VERTICAL_SPACER_HEIGHT);
    topLayout->addWidget(confirmLabelDolphin);
    topLayout->addWidget(m_confirmClosingMultipleTabs);

#if HAVE_TERMINAL
    topLayout->addWidget(m_confirmClosingTerminalRunningProgram);
#endif

    topLayout->addSpacing(Dolphin::VERTICAL_SPACER_HEIGHT);
    topLayout->addLayout(executableScriptLayout);

    topLayout->addStretch();

    loadSettings();

    connect(m_confirmMoveToTrash, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
    connect(m_confirmEmptyTrash, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
    connect(m_confirmDelete, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
    connect(m_confirmScriptExecution, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfirmationsSettingsPage::changed);
    connect(m_confirmClosingMultipleTabs, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);

#if HAVE_TERMINAL
    connect(m_confirmClosingTerminalRunningProgram, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
#endif
}

ConfirmationsSettingsPage::~ConfirmationsSettingsPage()
{
}

void ConfirmationsSettingsPage::applySettings()
{
    KSharedConfig::Ptr kioConfig = KSharedConfig::openConfig(QStringLiteral("kiorc"), KConfig::NoGlobals);
    KConfigGroup confirmationGroup(kioConfig, "Confirmations");
    confirmationGroup.writeEntry("ConfirmTrash", m_confirmMoveToTrash->isChecked());
    confirmationGroup.writeEntry("ConfirmEmptyTrash", m_confirmEmptyTrash->isChecked());
    confirmationGroup.writeEntry("ConfirmDelete", m_confirmDelete->isChecked());

    KConfigGroup scriptExecutionGroup(kioConfig, "Executable scripts");
    const int index = m_confirmScriptExecution->currentIndex();
    switch (index) {
    case ScriptExecution::AlwaysAsk:
        scriptExecutionGroup.writeEntry("behaviourOnLaunch", "alwaysAsk");
        break;
    case ScriptExecution::Open:
        scriptExecutionGroup.writeEntry("behaviourOnLaunch", "open");
        break;
    case ScriptExecution::Execute:
        scriptExecutionGroup.writeEntry("behaviourOnLaunch", "execute");
        break;
    }
    kioConfig->sync();

    GeneralSettings *settings = GeneralSettings::self();
    settings->setConfirmClosingMultipleTabs(m_confirmClosingMultipleTabs->isChecked());

#if HAVE_TERMINAL
    settings->setConfirmClosingTerminalRunningProgram(m_confirmClosingTerminalRunningProgram->isChecked());
#endif

    settings->save();
}

void ConfirmationsSettingsPage::restoreDefaults()
{
    GeneralSettings *settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);

    m_confirmMoveToTrash->setChecked(ConfirmTrash);
    m_confirmEmptyTrash->setChecked(ConfirmEmptyTrash);
    m_confirmDelete->setChecked(ConfirmDelete);
    m_confirmScriptExecution->setCurrentIndex(ConfirmScriptExecution);
}

void ConfirmationsSettingsPage::loadSettings()
{
    KSharedConfig::Ptr kioConfig = KSharedConfig::openConfig(QStringLiteral("kiorc"), KConfig::IncludeGlobals);
    const KConfigGroup confirmationGroup(kioConfig, "Confirmations");
    m_confirmMoveToTrash->setChecked(confirmationGroup.readEntry("ConfirmTrash", ConfirmTrash));
    m_confirmEmptyTrash->setChecked(confirmationGroup.readEntry("ConfirmEmptyTrash", ConfirmEmptyTrash));
    m_confirmDelete->setChecked(confirmationGroup.readEntry("ConfirmDelete", ConfirmDelete));

    const KConfigGroup scriptExecutionGroup(KSharedConfig::openConfig(QStringLiteral("kiorc")), "Executable scripts");
    const QString value = scriptExecutionGroup.readEntry("behaviourOnLaunch", "alwaysAsk");
    if (value == QLatin1String("alwaysAsk")) {
        m_confirmScriptExecution->setCurrentIndex(ScriptExecution::AlwaysAsk);
    } else if (value == QLatin1String("execute")) {
        m_confirmScriptExecution->setCurrentIndex(ScriptExecution::Execute);
    } else /* if (value == QLatin1String("open"))*/ {
        m_confirmScriptExecution->setCurrentIndex(ScriptExecution::Open);
    }

    m_confirmClosingMultipleTabs->setChecked(GeneralSettings::confirmClosingMultipleTabs());

#if HAVE_TERMINAL
    m_confirmClosingTerminalRunningProgram->setChecked(GeneralSettings::confirmClosingTerminalRunningProgram());
#endif
}

#include "moc_confirmationssettingspage.cpp"
