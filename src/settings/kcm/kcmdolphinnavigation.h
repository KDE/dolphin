/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KCMDOLPHINNAVIGATION_H
#define KCMDOLPHINNAVIGATION_H

#include <KCModule>

class NavigationSettingsPage;

/**
 * @brief Allow to configure the Dolphin navigation.
 */
class DolphinNavigationConfigModule : public KCModule
{
    Q_OBJECT

public:
    DolphinNavigationConfigModule(QWidget *parent, const QVariantList &args);
    ~DolphinNavigationConfigModule() override;

    void save() override;
    void defaults() override;

private:
    NavigationSettingsPage *m_navigation;
};

#endif
