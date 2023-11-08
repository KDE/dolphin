/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2021 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "viewmodesettings.h"

#include "dolphin_compactmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_iconsmodesettings.h"

#include "dolphin_generalsettings.h"

ViewModeSettings::ViewModeSettings()
{
    auto removeEntries = [](KConfigGroup &group) {
        group.deleteEntry("FontFamily");
        group.deleteEntry("FontWeight");
        group.deleteEntry("ItalicFont");
    };

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    // Migrate old config entries
    if (GeneralSettings::version() < 202) {
        for (const QString &groupName : {QStringLiteral("CompactMode"), QStringLiteral("DetailsMode"), QStringLiteral("IconsMode")}) {
            KConfigGroup group = config->group(groupName);
            const QString family = group.readEntry("FontFamily", QString{});
            if (family.isEmpty()) {
                removeEntries(group);
                continue;
            }

            QFont font;
            font.setFamily(family);
            const int weight = group.readEntry<int>("FontWeight", QFont::Normal);
            font.setWeight(static_cast<QFont::Weight>(weight));
            font.setItalic(group.readEntry("ItalicFont", false));
            removeEntries(group);

            // Write the new config entry
            group.writeEntry("ViewFont", font);
        }

        config->sync();
    }
}

ViewModeSettings::ViewModeSettings(DolphinView::Mode mode)
    : ViewModeSettings()
{
    switch (mode) {
    case DolphinView::IconsView:
        m_viewModeSettingsVariant = IconsModeSettings::self();
        return;
    case DolphinView::CompactView:
        m_viewModeSettingsVariant = CompactModeSettings::self();
        return;
    case DolphinView::DetailsView:
        m_viewModeSettingsVariant = DetailsModeSettings::self();
        return;
    default:
        Q_UNREACHABLE();
    }
}

ViewModeSettings::ViewModeSettings(ViewSettingsTab::Mode mode)
    : ViewModeSettings()
{
    switch (mode) {
    case ViewSettingsTab::IconsMode:
        m_viewModeSettingsVariant = IconsModeSettings::self();
        return;
    case ViewSettingsTab::CompactMode:
        m_viewModeSettingsVariant = CompactModeSettings::self();
        return;
    case ViewSettingsTab::DetailsMode:
        m_viewModeSettingsVariant = DetailsModeSettings::self();
        return;
    default:
        Q_UNREACHABLE();
    }
}

ViewModeSettings::ViewModeSettings(KStandardItemListView::ItemLayout itemLayout)
    : ViewModeSettings()
{
    switch (itemLayout) {
    case KStandardItemListView::IconsLayout:
        m_viewModeSettingsVariant = IconsModeSettings::self();
        return;
    case KStandardItemListView::CompactLayout:
        m_viewModeSettingsVariant = CompactModeSettings::self();
        return;
    case KStandardItemListView::DetailsLayout:
        m_viewModeSettingsVariant = DetailsModeSettings::self();
        return;
    default:
        Q_UNREACHABLE();
    }
}

void ViewModeSettings::setIconSize(int iconSize)
{
    std::visit(
        [iconSize](auto &&v) {
            v->setIconSize(iconSize);
        },
        m_viewModeSettingsVariant);
}

int ViewModeSettings::iconSize() const
{
    return std::visit(
        [](auto &&v) {
            return v->iconSize();
        },
        m_viewModeSettingsVariant);
}

void ViewModeSettings::setPreviewSize(int previewSize)
{
    std::visit(
        [previewSize](auto &&v) {
            v->setPreviewSize(previewSize);
        },
        m_viewModeSettingsVariant);
}

int ViewModeSettings::previewSize() const
{
    return std::visit(
        [](auto &&v) {
            return v->previewSize();
        },
        m_viewModeSettingsVariant);
}

void ViewModeSettings::setUseSystemFont(bool useSystemFont)
{
    std::visit(
        [useSystemFont](auto &&v) {
            v->setUseSystemFont(useSystemFont);
        },
        m_viewModeSettingsVariant);
}

bool ViewModeSettings::useSystemFont() const
{
    return std::visit(
        [](auto &&v) {
            return v->useSystemFont();
        },
        m_viewModeSettingsVariant);
}

void ViewModeSettings::setViewFont(const QFont &font)
{
    std::visit(
        [&font](auto &&v) {
            v->setViewFont(font);
        },
        m_viewModeSettingsVariant);
}

QFont ViewModeSettings::viewFont() const
{
    return std::visit(
        [](auto &&v) {
            return v->viewFont();
        },
        m_viewModeSettingsVariant);
}

void ViewModeSettings::useDefaults(bool useDefaults)
{
    std::visit(
        [useDefaults](auto &&v) {
            v->useDefaults(useDefaults);
        },
        m_viewModeSettingsVariant);
}

void ViewModeSettings::readConfig()
{
    std::visit(
        [](auto &&v) {
            v->load();
        },
        m_viewModeSettingsVariant);
}

void ViewModeSettings::save()
{
    std::visit(
        [](auto &&v) {
            return v->save();
        },
        m_viewModeSettingsVariant);
}
