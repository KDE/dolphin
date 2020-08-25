/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KCMDOLPHINVIEWMODES_H
#define KCMDOLPHINVIEWMODES_H

#include <KCModule>

class ViewSettingsTab;

/**
 * @brief Allow to configure the Dolphin views.
 */
class DolphinViewModesConfigModule : public KCModule
{
    Q_OBJECT

public:
    DolphinViewModesConfigModule(QWidget *parent, const QVariantList &args);
    ~DolphinViewModesConfigModule() override;

    void save() override;
    void defaults() override;

private:
    void reparseConfiguration();

private Q_SLOTS:
   void viewModeChanged();

private:
    QList<ViewSettingsTab*> m_tabs;
};

#endif
