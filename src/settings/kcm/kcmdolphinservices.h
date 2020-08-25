/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KCMDOLPHINSERVICES_H
#define KCMDOLPHINSERVICES_H

#include <KCModule>

class ServicesSettingsPage;

/**
 * @brief Allow to configure the Dolphin services.
 */
class DolphinServicesConfigModule : public KCModule
{
    Q_OBJECT

public:
    DolphinServicesConfigModule(QWidget* parent, const QVariantList& args);
    ~DolphinServicesConfigModule() override;

    void save() override;
    void defaults() override;

private:
    ServicesSettingsPage *m_services;
};

#endif
