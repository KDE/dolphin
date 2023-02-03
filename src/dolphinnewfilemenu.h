/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef DOLPHINNEWFILEMENU_H
#define DOLPHINNEWFILEMENU_H

#include "dolphin_export.h"

#include <KNewFileMenu>

class KJob;

/**
 * @brief Represents the 'Create New...' sub menu for the File menu
 *        and the context menu.
 *
 * The only difference to KNewFileMenu is the custom error handling.
 * All errors are shown in the status bar of Dolphin
 * instead as modal error dialog with an OK button.
 */
class DOLPHIN_EXPORT DolphinNewFileMenu : public KNewFileMenu
{
    Q_OBJECT

public:
    DolphinNewFileMenu(KActionCollection *collection, QObject *parent);
    ~DolphinNewFileMenu() override;

Q_SIGNALS:
    void errorMessage(const QString &error);

protected Q_SLOTS:
    /** @see KNewFileMenu::slotResult() */
    void slotResult(KJob *job) override;
};

#endif
