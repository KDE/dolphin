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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef DOLPHINSETTINGS_H
#define DOLPHINSETTINGS_H

class KBookmark;
class KBookmarkManager;
class GeneralSettings;
class IconsModeSettings;
class PreviewsModeSettings;
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
class DolphinSettings {
public:
    static DolphinSettings& instance();

    GeneralSettings* generalSettings() const { return m_generalSettings; }
    IconsModeSettings* iconsModeSettings() const { return m_iconsModeSettings; }
    PreviewsModeSettings* previewsModeSettings() const { return m_previewsModeSettings; }
    DetailsModeSettings* detailsModeSettings() const { return m_detailsModeSettings; }

    KBookmarkManager* bookmarkManager() const;

    // TODO: should this really belong here or get moved to a derived KBookmarkManager?
    // Dolphin uses some lists where an index is given and the corresponding bookmark
    // should get retrieved...
    KBookmark bookmark(int index) const;

    /** @see DolphinSettingsBase::save */
    virtual void save();

    /**
     * TODO: just temporary until the port to KDE4 has been done
     *
     * Calculates the width and the height of the grid dependant from \a hint and
     * the current settings. The hint gives information about the wanted text
     * width, where a lower value indicates a smaller text width. Currently
     * in Dolphin the values 0, 1 and 2 are used. See also
     * DolhinIconsViewSettings::textWidthHint.
     *
     * The calculation of the grid width and grid height is a little bit tricky,
     * as the user model does not fit to the implementation model of QIconView. The user model
     * allows to specify icon-, preview- and text width sizes, whereas the implementation
     * model expects only a grid width and height. The nasty thing is that the specified
     * width and height varies dependant from the arrangement (e. g. the height is totally
     * ignored for the top-to-bottom arrangement inside QIconView).
     */
    void calculateGridSize(int hint);

    /**
     * TODO: just temporary until the port to KDE4 has been done
     *
     * Returns the text width hint dependant from the given settings.
     * A lower value indicates a smaller text width. Currently
     * in Dolphin the values 0, 1 and 2 are used. The text width hint can
     * be used later for DolphinIconsViewSettings::calculateGridSize().
     */
    int textWidthHint() const;

protected:
    DolphinSettings();
    virtual ~DolphinSettings();

private:
    GeneralSettings* m_generalSettings;
    IconsModeSettings* m_iconsModeSettings;
    PreviewsModeSettings* m_previewsModeSettings;
    DetailsModeSettings* m_detailsModeSettings;
};

#endif
