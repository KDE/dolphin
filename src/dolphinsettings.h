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

#ifndef DOLPHINSETTINGS_H
#define DOLPHINSETTINGS_H

#include <libdolphin_export.h>

class KBookmark;
class KBookmarkManager;
class GeneralSettings;
class IconsModeSettings;
class DetailsModeSettings;

/**
 * @brief Manages and stores all settings from Dolphin.
 *
 * The following properties are stored:
 * - home Url
 * - default view mode
 * - Url navigator state (editable or not)
 * - split view
 * - bookmarks
 * - properties for icons and details view
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinSettings {
public:
    static DolphinSettings& instance();

    GeneralSettings* generalSettings() const { return m_generalSettings; }
    IconsModeSettings* iconsModeSettings() const { return m_iconsModeSettings; }
    DetailsModeSettings* detailsModeSettings() const { return m_detailsModeSettings; }

    KBookmarkManager* bookmarkManager() const;

    // TODO: should this really belong here or get moved to a derived KBookmarkManager?
    // Dolphin uses some lists where an index is given and the corresponding bookmark
    // should get retrieved...
    KBookmark bookmark(int index) const;

    /** @see DolphinSettingsBase::save */
    virtual void save();

protected:
    DolphinSettings();
    virtual ~DolphinSettings();

private:
    GeneralSettings* m_generalSettings;
    IconsModeSettings* m_iconsModeSettings;
    DetailsModeSettings* m_detailsModeSettings;
};

#endif
