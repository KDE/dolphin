/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "powercopysettingspage.h"

#include "dolphin_powercopysettings.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{
QLabel *hint(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setEnabled(false);
    return label;
}
}

PowerCopySettingsPage::PowerCopySettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
{
    m_enabled = new QCheckBox(i18nc("@option:check", "Copy several files at the same time"), this);

    m_filesInFlight = new QSpinBox(this);
    m_filesInFlight->setRange(1, 64);
    m_filesInFlight->setSuffix(i18ncp("@item:valuesuffix", " file", " files", m_filesInFlight->value()));

    m_minimumFiles = new QSpinBox(this);
    m_minimumFiles->setRange(2, 10000);
    m_minimumFiles->setSuffix(i18ncp("@item:valuesuffix", " file", " files", m_minimumFiles->value()));

    auto *form = new QFormLayout();
    form->addRow(i18nc("@label", "Copying:"), m_enabled);
    form->addRow(QString(),
                 hint(i18nc("@info",
                            "Files are otherwise copied one after another, which leaves the disk waiting between them. "
                            "Each file is still copied by the same machinery as before, so nothing about the result changes."),
                      this));
    form->addRow(i18nc("@label:spinbox", "At the same time:"), m_filesInFlight);
    form->addRow(QString(),
                 hint(i18nc("@info",
                            "More is not always better. On an ordinary disk the gain stops at about four, because that is as many "
                            "helpers as the system runs; on an encrypted volume, where the work is done by the processor, eight is "
                            "faster still. On a spinning disk, keep this at one."),
                      this));
    form->addRow(i18nc("@label:spinbox", "Only from:"), m_minimumFiles);
    form->addRow(QString(), hint(i18nc("@info", "Below this many files the ordinary way is used, since the preparation would cost more than it saves."), this));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addStretch();

    loadSettings();

    connect(m_enabled, &QCheckBox::toggled, this, [this]() {
        updateEnabledState();
        Q_EMIT changed();
    });
    connect(m_filesInFlight, &QSpinBox::valueChanged, this, &SettingsPageBase::changed);
    connect(m_minimumFiles, &QSpinBox::valueChanged, this, &SettingsPageBase::changed);
}

void PowerCopySettingsPage::updateEnabledState()
{
    m_filesInFlight->setEnabled(m_enabled->isChecked());
    m_minimumFiles->setEnabled(m_enabled->isChecked());
}

void PowerCopySettingsPage::loadSettings()
{
    m_enabled->setChecked(PowerCopySettings::enabled());
    m_filesInFlight->setValue(PowerCopySettings::filesInFlight());
    m_minimumFiles->setValue(PowerCopySettings::minimumFiles());
    updateEnabledState();
}

void PowerCopySettingsPage::applySettings()
{
    PowerCopySettings::setEnabled(m_enabled->isChecked());
    PowerCopySettings::setFilesInFlight(m_filesInFlight->value());
    PowerCopySettings::setMinimumFiles(m_minimumFiles->value());
    PowerCopySettings::self()->save();
}

void PowerCopySettingsPage::restoreDefaults()
{
    PowerCopySettings::self()->setDefaults();
    loadSettings();
    Q_EMIT changed();
}
