/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef PREVIEWSSETTINGSPAGE_H
#define PREVIEWSSETTINGSPAGE_H

#include <kurl.h>
#include <settings/settingspagebase.h>

class DolphinMainWindow;
class QCheckBox;
class QListWidget;
class QRadioButton;
class QSlider;
class KIntSpinBox;

/**
 * @brief Allows the configuration of file previews.
 */
class PreviewsSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    PreviewsSettingsPage(QWidget* parent);
    virtual ~PreviewsSettingsPage();

    /**
     * Applies the general settings for the view modes
     * The settings are persisted automatically when
     * closing Dolphin.
     */
    virtual void applySettings();

    /** Restores the settings to default values. */
    virtual void restoreDefaults();

protected:
    virtual bool event(QEvent* event);

private:
    void loadSettings();

private:
    bool m_initialized;
    QListWidget* m_previewPluginsList;
    QStringList m_enabledPreviewPlugins;
    QSlider* m_maxPreviewSize;
    KIntSpinBox* m_spinBox;
    QCheckBox* m_useFileThumbnails;
};

#endif
