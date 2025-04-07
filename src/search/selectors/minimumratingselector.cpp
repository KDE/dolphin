/*
    SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "minimumratingselector.h"

#include "../dolphinquery.h"

#include <KLocalizedString>

using namespace Search;

MinimumRatingSelector::MinimumRatingSelector(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent)
    : QComboBox{parent}
    , UpdatableStateInterface{dolphinQuery}
{
    addItem(/** No icon for the empty state */ i18nc("@item:inlistbox", "Any Rating"), 0);
    addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "1 or more"), 2);
    addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "2 or more"), 4);
    addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "3 or more"), 6);
    addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox", "4 or more"), 8);
    addItem(QIcon::fromTheme(QStringLiteral("starred-symbolic")), i18nc("@item:inlistbox 5 star rating, has a star icon in front", "5"), 10);

    connect(this, &QComboBox::activated, this, [this](int activatedIndex) {
        auto activatedMinimumRating = itemData(activatedIndex).value<int>();
        if (activatedMinimumRating == m_searchConfiguration->minimumRating()) {
            return; // Already selected.
        }
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setMinimumRating(activatedMinimumRating);
        Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
    });

    updateStateToMatch(std::move(dolphinQuery));
}

void MinimumRatingSelector::removeRestriction()
{
    Q_ASSERT(m_searchConfiguration->minimumRating() > 0);
    DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
    searchConfigurationCopy.setMinimumRating(0);
    Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
}

void MinimumRatingSelector::updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery)
{
    setCurrentIndex(findData(dolphinQuery->minimumRating()));
}
