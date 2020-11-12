/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KCMDOLPHINCONTEXTMENU_H
#define KCMDOLPHINCONTEXTMENU_H

#include <KCModule>

class ContextMenuSettingsPage;

/**
 * @brief Allow to configure the Dolphin context menu.
 */
class DolphinContextMenuConfigModule : public KCModule
{
    Q_OBJECT

public:
    DolphinContextMenuConfigModule(QWidget* parent, const QVariantList& args);
    ~DolphinContextMenuConfigModule() override;

    void save() override;
    void defaults() override;

private:
    ContextMenuSettingsPage *m_contextMenu;
};

#endif
