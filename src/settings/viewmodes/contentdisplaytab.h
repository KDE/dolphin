/*
 * SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef GENERALTAB_H
#define GENERALTAB_H

#include "settings/settingspagebase.h"

class QRadioButton;
class QSpinBox;

class ContentDisplayTab : public SettingsPageBase
{
    Q_OBJECT

public:
    ContentDisplayTab(QWidget *parent);

public:
    // SettingsPageBase interface
    void applySettings() override;
    void restoreDefaults() override;

private:
    void loadSettings();

    QRadioButton *m_numberOfItems;
    QRadioButton *m_sizeOfContents;
    QSpinBox *m_recursiveDirectorySizeLimit;
    QRadioButton *m_useRelatetiveDates;
    QRadioButton *m_useShortDates;
    QRadioButton *m_useSymbolicPermissions;
    QRadioButton *m_useNumericPermissions;
    QRadioButton *m_useCombinedPermissions;
};

#endif // GENERALTAB_H
