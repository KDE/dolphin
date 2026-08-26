/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CUSTOMACTIONSSETTINGSPAGE_H
#define CUSTOMACTIONSSETTINGSPAGE_H

#include "customactions.h"
#include "settings/settingspagebase.h"

class QListWidget;
class QPushButton;

/**
 * @brief Manages the user's own context menu entries.
 *
 * Directory entries and file entries are edited as two separate lists, because
 * that is how they are offered in the context menu.
 */
class CustomActionsSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit CustomActionsSettingsPage(QWidget *parent);

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    /** One list plus its buttons; there is one of these per target. */
    struct Section {
        CustomActions::Target target;
        QListWidget *list;
        QPushButton *editButton;
        QPushButton *removeButton;
        QPushButton *upButton;
        QPushButton *downButton;
        QList<CustomActions::Entry> entries;
    };

    QWidget *createSection(Section &section, const QString &title, const QString &explanation);
    void refresh(Section &section);
    void updateButtons(Section &section);

    void addEntry(Section &section);
    void editEntry(Section &section);
    void removeEntry(Section &section);
    void moveEntry(Section &section, int offset);

    Section m_directories;
    Section m_files;
};

#endif
