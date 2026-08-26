/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "customactionssettingspage.h"

#include "customactiondialog.h"

#include <KLocalizedString>

#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

CustomActionsSettingsPage::CustomActionsSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
{
    m_directories.target = CustomActions::Target::Directories;
    m_files.target = CustomActions::Target::Files;

    m_directories.entries = CustomActions::load(CustomActions::Target::Directories);
    m_files.entries = CustomActions::load(CustomActions::Target::Files);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(createSection(m_directories,
                                    i18nc("@title:group", "When directories are selected"),
                                    i18nc("@info", "These entries are offered when the selection is made up of folders.")));
    layout->addWidget(createSection(m_files,
                                    i18nc("@title:group", "When files are selected"),
                                    i18nc("@info", "These entries are offered when the selection is made up of files.")));
    layout->addStretch();

    refresh(m_directories);
    refresh(m_files);
}

QWidget *CustomActionsSettingsPage::createSection(Section &section, const QString &title, const QString &explanation)
{
    auto *box = new QGroupBox(title, this);

    auto *description = new QLabel(explanation, box);
    description->setWordWrap(true);
    description->setEnabled(false);

    section.list = new QListWidget(box);
    section.list->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *addButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18nc("@action:button", "Add…"), box);
    section.editButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-edit")), i18nc("@action:button", "Edit…"), box);
    section.removeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18nc("@action:button", "Remove"), box);
    section.upButton = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), i18nc("@action:button", "Move Up"), box);
    section.downButton = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down")), i18nc("@action:button", "Move Down"), box);

    auto *buttons = new QVBoxLayout();
    buttons->addWidget(addButton);
    buttons->addWidget(section.editButton);
    buttons->addWidget(section.removeButton);
    buttons->addSpacing(12);
    buttons->addWidget(section.upButton);
    buttons->addWidget(section.downButton);
    buttons->addStretch();

    auto *row = new QHBoxLayout();
    row->addWidget(section.list);
    row->addLayout(buttons);

    auto *layout = new QVBoxLayout(box);
    layout->addWidget(description);
    layout->addLayout(row);

    Section *self = &section;
    connect(addButton, &QPushButton::clicked, this, [this, self]() {
        addEntry(*self);
    });
    connect(section.editButton, &QPushButton::clicked, this, [this, self]() {
        editEntry(*self);
    });
    connect(section.removeButton, &QPushButton::clicked, this, [this, self]() {
        removeEntry(*self);
    });
    connect(section.upButton, &QPushButton::clicked, this, [this, self]() {
        moveEntry(*self, -1);
    });
    connect(section.downButton, &QPushButton::clicked, this, [this, self]() {
        moveEntry(*self, 1);
    });
    connect(section.list, &QListWidget::itemSelectionChanged, this, [this, self]() {
        updateButtons(*self);
    });
    connect(section.list, &QListWidget::itemDoubleClicked, this, [this, self]() {
        editEntry(*self);
    });

    return box;
}

void CustomActionsSettingsPage::refresh(Section &section)
{
    const int selected = section.list->currentRow();

    section.list->clear();
    for (const CustomActions::Entry &entry : std::as_const(section.entries)) {
        auto *item = new QListWidgetItem(QIcon::fromTheme(entry.icon), entry.name, section.list);
        item->setToolTip(entry.command);
    }

    if (selected >= 0 && selected < section.list->count()) {
        section.list->setCurrentRow(selected);
    }
    updateButtons(section);
}

void CustomActionsSettingsPage::updateButtons(Section &section)
{
    const int row = section.list->currentRow();
    const bool hasSelection = row >= 0;

    section.editButton->setEnabled(hasSelection);
    section.removeButton->setEnabled(hasSelection);
    section.upButton->setEnabled(hasSelection && row > 0);
    section.downButton->setEnabled(hasSelection && row < section.entries.count() - 1);
}

void CustomActionsSettingsPage::addEntry(Section &section)
{
    CustomActionDialog dialog(this, section.target);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    section.entries.append(dialog.entry());
    refresh(section);
    section.list->setCurrentRow(section.entries.count() - 1);
    Q_EMIT changed();
}

void CustomActionsSettingsPage::editEntry(Section &section)
{
    const int row = section.list->currentRow();
    if (row < 0 || row >= section.entries.count()) {
        return;
    }

    CustomActionDialog dialog(this, section.target, section.entries.at(row));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    section.entries[row] = dialog.entry();
    refresh(section);
    Q_EMIT changed();
}

void CustomActionsSettingsPage::removeEntry(Section &section)
{
    const int row = section.list->currentRow();
    if (row < 0 || row >= section.entries.count()) {
        return;
    }

    section.entries.removeAt(row);
    refresh(section);
    Q_EMIT changed();
}

void CustomActionsSettingsPage::moveEntry(Section &section, int offset)
{
    const int row = section.list->currentRow();
    const int target = row + offset;
    if (row < 0 || row >= section.entries.count() || target < 0 || target >= section.entries.count()) {
        return;
    }

    section.entries.move(row, target);
    refresh(section);
    section.list->setCurrentRow(target);
    Q_EMIT changed();
}

void CustomActionsSettingsPage::applySettings()
{
    CustomActions::save(CustomActions::Target::Directories, m_directories.entries);
    CustomActions::save(CustomActions::Target::Files, m_files.entries);
}

void CustomActionsSettingsPage::restoreDefaults()
{
    // There are no entries out of the box.
    m_directories.entries.clear();
    m_files.entries.clear();
    refresh(m_directories);
    refresh(m_files);
    Q_EMIT changed();
}
