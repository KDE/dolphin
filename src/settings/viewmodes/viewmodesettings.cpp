/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2021 Felix Ernst <fe.a.ernst@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "viewmodesettings.h"

#include "dolphin_compactmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_iconsmodesettings.h"

ViewModeSettings::ViewModeSettings(DolphinView::Mode mode)
{
    switch (mode) {
    case DolphinView::IconsView:    m_viewModeSettingsVariant = IconsModeSettings::self();   return;
    case DolphinView::CompactView:  m_viewModeSettingsVariant = CompactModeSettings::self(); return;
    case DolphinView::DetailsView:  m_viewModeSettingsVariant = DetailsModeSettings::self(); return;
    default:
        Q_UNREACHABLE();
    }
}

ViewModeSettings::ViewModeSettings(ViewSettingsTab::Mode mode)
{
    switch (mode) {
    case ViewSettingsTab::IconsMode:    m_viewModeSettingsVariant = IconsModeSettings::self();   return;
    case ViewSettingsTab::CompactMode:  m_viewModeSettingsVariant = CompactModeSettings::self(); return;
    case ViewSettingsTab::DetailsMode:  m_viewModeSettingsVariant = DetailsModeSettings::self(); return;
    default:
        Q_UNREACHABLE();
    }
}

ViewModeSettings::ViewModeSettings(KStandardItemListView::ItemLayout itemLayout)
{
    switch (itemLayout) {
    case KStandardItemListView::IconsLayout:    m_viewModeSettingsVariant = IconsModeSettings::self();   return;
    case KStandardItemListView::CompactLayout:  m_viewModeSettingsVariant = CompactModeSettings::self(); return;
    case KStandardItemListView::DetailsLayout:  m_viewModeSettingsVariant = DetailsModeSettings::self(); return;
    default:
        Q_UNREACHABLE();
    }
}

void ViewModeSettings::setIconSize(int iconSize)
{
    std::visit([iconSize](auto &&v) {
        v->setIconSize(iconSize);
    }, m_viewModeSettingsVariant);
}

int ViewModeSettings::iconSize() const
{
    return std::visit([](auto &&v) {
        return v->iconSize();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::setPreviewSize(int previewSize)
{
    std::visit([previewSize](auto &&v) {
        v->setPreviewSize(previewSize);
    }, m_viewModeSettingsVariant);
}

int ViewModeSettings::previewSize() const
{
    return std::visit([](auto &&v) {
        return v->previewSize();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::setUseSystemFont(bool useSystemFont)
{
    std::visit([useSystemFont](auto &&v) {
        v->setUseSystemFont(useSystemFont);
    }, m_viewModeSettingsVariant);
}

bool ViewModeSettings::useSystemFont() const
{
    return std::visit([](auto &&v) {
        return v->useSystemFont();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::setFontFamily(const QString &fontFamily)
{
    std::visit([&fontFamily](auto &&v) {
        v->setFontFamily(fontFamily);
    }, m_viewModeSettingsVariant);
}

QString ViewModeSettings::fontFamily() const
{
    return std::visit([](auto &&v) {
        return v->fontFamily();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::setFontSize(qreal fontSize)
{
    std::visit([fontSize](auto &&v) {
        v->setFontSize(fontSize);
    }, m_viewModeSettingsVariant);
}

qreal ViewModeSettings::fontSize() const
{
    return std::visit([](auto &&v) {
        return v->fontSize();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::setItalicFont(bool italic)
{
    std::visit([italic](auto &&v) {
        v->setItalicFont(italic);
    }, m_viewModeSettingsVariant);
}

bool ViewModeSettings::italicFont() const
{
    return std::visit([](auto &&v) {
        return v->italicFont();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::setFontWeight(int fontWeight)
{
    std::visit([fontWeight](auto &&v) {
        v->setFontWeight(fontWeight);
    }, m_viewModeSettingsVariant);
}

int ViewModeSettings::fontWeight() const
{
    return std::visit([](auto &&v) {
        return v->fontWeight();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::useDefaults(bool useDefaults)
{
    std::visit([useDefaults](auto &&v) {
        v->useDefaults(useDefaults);
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::readConfig()
{
    std::visit([](auto &&v) {
        v->load();
    }, m_viewModeSettingsVariant);
}

void ViewModeSettings::save()
{
    std::visit([](auto &&v) {
        return v->save();
    }, m_viewModeSettingsVariant);
}
