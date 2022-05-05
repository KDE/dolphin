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

void ViewModeSettings::setViewFont(const QFont &font)
{
    std::visit([&font](auto &&v) {
        v->setViewFont(font);
    }, m_viewModeSettingsVariant);
}

QFont ViewModeSettings::viewFont() const
{
    return std::visit([](auto &&v) {
        return v->viewFont();
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
