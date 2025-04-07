/*
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef UPDATABLESTATEINTERFACE_H
#define UPDATABLESTATEINTERFACE_H

#include <QtAssert>

#include <memory>

namespace Search
{
class DolphinQuery;

class UpdatableStateInterface
{
public:
    inline explicit UpdatableStateInterface(std::shared_ptr<const DolphinQuery> dolphinQuery)
        : m_searchConfiguration{std::move(dolphinQuery)} {};

    virtual ~UpdatableStateInterface(){};

    /**
     * Updates this object and its child widgets so their states are correctly described by the @p dolphinQuery.
     * This method is always initially called on the Search::Bar which in turn calls this method on its child widgets. That is because the Search::Bar is the
     * ancestor widget of all classes implementing UpdatableStateInterface, and from Search::Bar::updateStateToMatch() the changed state represented by the
     * @p dolphinQuery is propagated to all other UpdatableStateInterfaces through UpdatableStateInterface::updateState() calls.
     */
    inline void updateStateToMatch(std::shared_ptr<const DolphinQuery> dolphinQuery)
    {
        Q_ASSERT_X(m_searchConfiguration, "UpdatableStateInterface::updateStateToMatch()", "An UpdatableStateInterface should always have a consistent state.");
        updateState(dolphinQuery);
        m_searchConfiguration = std::move(dolphinQuery);
        Q_ASSERT_X(m_searchConfiguration, "UpdatableStateInterface::updateStateToMatch()", "An UpdatableStateInterface should always have a consistent state.");
    };

private:
    /**
     * Implementations of this method initialize the state of this object and its child widgets to represent the state of the @p dolphinQuery.
     * This method is only ever called from UpdatableStateInterface::updateStateToMatch().
     */
    virtual void updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery) = 0;

protected:
    /**
     * The DolphinQuery that was used to initialize this object's state.
     */
    std::shared_ptr<const DolphinQuery> m_searchConfiguration;
};

}

#endif // UPDATABLESTATEINTERFACE_H
