// SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <KConfigGroup>
#include <KSharedConfig>

int main()
{
    auto config = KSharedConfig::openConfig(QStringLiteral("dolphinrc"));

    // A profile that Dolphin never used has no version yet, and gets the automatic icon size.
    if (!config->group(QStringLiteral("General")).hasKey(QStringLiteral("Version"))) {
        return EXIT_SUCCESS;
    }

    KConfigGroup placesPanel = config->group(QStringLiteral("PlacesPanel"));
    const auto iconSize = QStringLiteral("IconSize");
    if (placesPanel.hasKey(iconSize)) {
        return EXIT_SUCCESS;
    }

    // Keep KIconLoader::SizeSmallMedium, the default until the icon size became automatic.
    placesPanel.writeEntry(iconSize, 22);
    return placesPanel.sync() ? EXIT_SUCCESS : EXIT_FAILURE;
}
