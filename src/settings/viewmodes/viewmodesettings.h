/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWMODESETTINGS_H
#define VIEWMODESETTINGS_H

#include <QString>

/**
 * @short Helper class for accessing similar properties of IconsModeSettings,
 *        CompactModeSettings and DetailsModeSettings.
 */
class ViewModeSettings
{
public:
    enum ViewMode
    {
        IconsMode,
        CompactMode,
        DetailsMode
    };

    explicit ViewModeSettings(ViewMode mode);
    virtual ~ViewModeSettings();

    void setIconSize(int size) const;
    int iconSize() const;

    void setPreviewSize(int size) const;
    int previewSize() const;

    void setUseSystemFont(bool flag);
    bool useSystemFont() const;

    void setFontFamily(const QString& fontFamily);
    QString fontFamily() const;

    void setFontSize(qreal fontSize);
    qreal fontSize() const;

    void setItalicFont(bool italic);
    bool italicFont() const;

    void setFontWeight(int fontWeight);
    int fontWeight() const;

    void readConfig();
    void save();

private:
    ViewMode m_mode;
};

#endif
