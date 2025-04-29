/*
    SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TAGSSELECTOR_H
#define TAGSSELECTOR_H

#include "../updatablestateinterface.h"

#include <QToolButton>

namespace Search
{

class TagsSelector : public QToolButton, public UpdatableStateInterface
{
    Q_OBJECT

public:
    explicit TagsSelector(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent = nullptr);

    /** Causes configurationChanged() to be emitted with a DolphinQuery object that does not contain any restriction settable by this class. */
    void removeRestriction();

Q_SIGNALS:
    /** Is emitted whenever settings have changed and a new search might be necessary. */
    void configurationChanged(const Search::DolphinQuery &dolphinQuery);

private:
    /**
     * Updates the menu items for the various tags based on @p dolphinQuery and the available tags.
     * This method should only be called when the menu is QMenu::aboutToShow() or the menu is currently visible already while this selector's state changes.
     * If the menu is open when this method is called, the menu will automatically be reopened to reflect the updated contents.
     */
    void updateMenu(const std::shared_ptr<const DolphinQuery> &dolphinQuery);

    void updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery) override;
};

}

#endif // TAGSSELECTOR_H
