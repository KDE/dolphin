/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef DOLPHINSETTINGSDIALOG_H
#define DOLPHINSETTINGSDIALOG_H

#include <kpagedialog.h>

class DolphinMainWindow;
class SettingsPageBase;

/**
 * @brief Settings dialog for Dolphin.
 *
 * Contains the pages for startup settings, general settings and view settings.
 *
 * @author Peter Penz <peter.penz@gmx.at>
 */
class DolphinSettingsDialog : public KPageDialog
{
    Q_OBJECT

public:
    explicit DolphinSettingsDialog(DolphinMainWindow* mainWindow);
    virtual ~DolphinSettingsDialog();

protected slots:
    /** @see KDialog::slotButtonClicked() */
    virtual void slotButtonClicked(int button);

private slots:
    /** Enables the Apply button. */
    void enableApply();

private:
    void applySettings();
    void restoreDefaults();

private:
    QList<SettingsPageBase*> m_pages;
};

#endif
