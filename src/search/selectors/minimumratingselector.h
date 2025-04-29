/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef MINIMUMRATINGSELECTOR_H
#define MINIMUMRATINGSELECTOR_H

#include "../updatablestateinterface.h"

#include <QComboBox>

namespace Search
{

/**
 * @brief Select the minimum rating search results should have.
 * Values <= 0 mean no restriction. 1 is half a star, 2 one full star, etc. 10 is typically the maximum in KDE software.
 * Since this box only allows selecting full star ratings, the possible values are 0, 2, 4, 6, 8, 10.
 */
class MinimumRatingSelector : public QComboBox, public UpdatableStateInterface
{
    Q_OBJECT

public:
    explicit MinimumRatingSelector(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent = nullptr);

    /** Causes configurationChanged() to be emitted with a DolphinQuery object that does not contain any restriction settable by this class. */
    void removeRestriction();

Q_SIGNALS:
    /** Is emitted whenever settings have changed and a new search might be necessary. */
    void configurationChanged(const Search::DolphinQuery &dolphinQuery);

private:
    void updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery) override;
};

}

#endif // MINIMUMRATINGSELECTOR_H
