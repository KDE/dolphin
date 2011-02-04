/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz19@gmail.com>             *
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
#ifndef STARTUPSETTINGSPAGE_H
#define STARTUPSETTINGSPAGE_H

#include <settings/settingspagebase.h>
#include <KUrl>

class KLineEdit;
class QCheckBox;

/**
 * @brief Page for the 'Startup' settings of the Dolphin settings dialog.
 *
 * The startup settings allow to set the home URL and to configure the
 * state of the view mode, split mode and the filter bar when starting Dolphin.
 */
class StartupSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    StartupSettingsPage(const KUrl& url, QWidget* parent);
    virtual ~StartupSettingsPage();

    /** @see SettingsPageBase::applySettings() */
    virtual void applySettings();

    /** @see SettingsPageBase::restoreDefaults() */
    virtual void restoreDefaults();

private slots:
    void slotSettingsChanged();
    void selectHomeUrl();
    void useCurrentLocation();
    void useDefaultLocation();

private:
    void loadSettings();

private:
    KUrl m_url;
    KLineEdit* m_homeUrl;

    QCheckBox* m_splitView;
    QCheckBox* m_editableUrl;
    QCheckBox* m_showFullPath;
    QCheckBox* m_filterBar;
};

#endif
