/*
    SPDX-FileCopyrightText: 2025 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KSharedConfig>

int main()
{
    const auto showStatusBar = QStringLiteral("ShowStatusBar");
    auto config = KSharedConfig::openConfig(QStringLiteral("dolphinrc"));

    KConfigGroup general = config->group(QStringLiteral("General"));
    if (!general.hasKey(showStatusBar)) {
        return EXIT_SUCCESS;
    }

    const auto value = general.readEntry(showStatusBar);
    if (value == QStringLiteral("true")) {
        general.writeEntry(showStatusBar, QStringLiteral("Small"));
    } else if (value == QStringLiteral("false")) {
        general.writeEntry(showStatusBar, QStringLiteral("Disabled"));
    }

    return general.sync() ? EXIT_SUCCESS : EXIT_FAILURE;
}
