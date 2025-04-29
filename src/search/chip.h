/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef CHIP_H
#define CHIP_H

#include "dolphinquery.h"
#include "selectors/dateselector.h"
#include "selectors/filetypeselector.h"
#include "selectors/minimumratingselector.h"
#include "selectors/tagsselector.h"
#include "updatablestateinterface.h"

#include <QComboBox>
#include <QLayout>
#include <QToolButton>
#include <QWidget>

#include <type_traits>

namespace Search
{

/**
 * @brief The non-template base class for the template class Chip.
 *
 * @see Chip below.
 *
 * Template classes cannot have Qt signals. This class works around that by being a non-template class which the template Chip class then inherits from.
 * This base class contains all non-template logic of Chip.
 */
class ChipBase : public QWidget, public UpdatableStateInterface
{
    Q_OBJECT

public:
    ChipBase(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent = nullptr);

Q_SIGNALS:
    /** Is emitted whenever settings have changed and a new search might be necessary. */
    void configurationChanged(const Search::DolphinQuery &dolphinQuery);

protected:
    void paintEvent(QPaintEvent *event) override;

protected:
    QToolButton *m_removeButton = nullptr;
};

/**
 * @brief A button-sized UI component for modifying or removing search filters.
 *
 * A template widget is taken and this Chip forms a button-like outline around it. The Chip has a close button on the side to remove itself, which communicates
 * to the user that the effect of the widget inside this Chip no longer applies. The functionality of the widget inside is not affected by the Chip.
 *
 * Most logic of this class is in the non-template ChipBase base class.
 * @see ChipBase above.
 */
template<class Selector>
class Chip : public ChipBase
{
public:
    Chip(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent = nullptr)
        : ChipBase{dolphinQuery, parent}
        , m_selector{new Selector{dolphinQuery, this}}
    {
        // Make the selector flat within the chip.
        if constexpr (std::is_base_of<QComboBox, Selector>::value) {
            m_selector->setFrame(false);
        } else if constexpr (std::is_base_of<QToolButton, Selector>::value) {
            m_selector->setAutoRaise(true);
        }
        setFocusProxy(m_selector);
        setTabOrder(m_selector, m_removeButton);

        connect(m_selector, &Selector::configurationChanged, this, &ChipBase::configurationChanged);

        // The m_removeButton does not directly remove the Chip. Instead the Selector's removeRestriction() method will emit ChipBase::configurationChanged()
        // with a DolphinQuery object that effectively removes the effects of the Selector. This in turn will then eventually remove this Chip when the new
        // state of the Search UI components is propagated through the various UpdatableStateInterface::updateStateToMatch() methods.
        connect(m_removeButton, &QAbstractButton::clicked, m_selector, &Selector::removeRestriction);

        layout()->addWidget(m_selector);
        layout()->addWidget(m_removeButton);
    };

private:
    void updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery) override
    {
        m_selector->updateStateToMatch(dolphinQuery);
    }

private:
    Selector *const m_selector;
};
}

#endif
