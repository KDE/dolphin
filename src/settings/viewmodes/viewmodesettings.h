/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWMODESETTINGS_H
#define VIEWMODESETTINGS_H

#include "kitemviews/kstandarditemlistview.h"
#include "viewsettingstab.h"
#include "views/dolphinview.h"

#include <variant>

class CompactModeSettings;
class DetailsModeSettings;
class IconsModeSettings;

/**
 * @short Helper class for accessing similar properties of IconsModeSettings,
 *        CompactModeSettings and DetailsModeSettings.
 */
class ViewModeSettings
{
public:
    explicit ViewModeSettings(DolphinView::Mode mode);
    explicit ViewModeSettings(ViewSettingsTab::Mode mode);
    explicit ViewModeSettings(KStandardItemListView::ItemLayout itemLayout);

    void setIconSize(int iconSize);
    int iconSize() const;

    void setPreviewSize(int previewSize);
    int previewSize() const;

    void setUseSystemFont(bool useSystemFont);
    bool useSystemFont() const;

    void setViewFont(const QFont &font);
    QFont viewFont() const;

    void useDefaults(bool useDefaults);

    void readConfig();
    void save();

private:
    explicit ViewModeSettings();

    std::variant<IconsModeSettings *, CompactModeSettings *, DetailsModeSettings *> m_viewModeSettingsVariant;
};

#endif
