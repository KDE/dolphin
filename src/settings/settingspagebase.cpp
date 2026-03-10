/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settingspagebase.h"

SettingsPageBase::SettingsPageBase(QWidget *parent)
    : QWidget(parent)
{
}

SettingsPageBase::~SettingsPageBase()
{
}

void SettingsPageBase::setValid(bool valid)
{
    if (valid != m_valid) {
        m_valid = valid;
        Q_EMIT validChanged(valid);
    }
}

bool SettingsPageBase::isValid() const
{
    return m_valid;
}

#include "moc_settingspagebase.cpp"
