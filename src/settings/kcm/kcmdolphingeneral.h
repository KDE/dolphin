/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KCMDOLPHINGENERAL_H
#define KCMDOLPHINGENERAL_H

#include <KCModule>

#include <QList>

class SettingsPageBase;

/**
 * @brief Allow to configure general Dolphin settings.
 */
class DolphinGeneralConfigModule : public KCModule
{
    Q_OBJECT

public:
    DolphinGeneralConfigModule(QObject *parent, const KPluginMetaData &data);
    ~DolphinGeneralConfigModule() override;

    void save() override;
    void defaults() override;

private:
    QList<SettingsPageBase *> m_pages;
};

#endif
