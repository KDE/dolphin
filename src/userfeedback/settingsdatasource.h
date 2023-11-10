/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SETTINGSDATASOURCE_H
#define SETTINGSDATASOURCE_H

#include <KUserFeedback/AbstractDataSource>

class DolphinMainWindow;

class SettingsDataSource : public KUserFeedback::AbstractDataSource
{
public:
    SettingsDataSource();

    QString name() const override;
    QString description() const override;
    QVariant data() override;

private:
    DolphinMainWindow *m_mainWindow = nullptr;
};

#endif // SETTINGSDATASOURCE_H
