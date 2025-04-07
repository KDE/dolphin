/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "dateselector.h"

#include "../dolphinquery.h"

#include <KDatePicker>
#include <KDatePickerPopup>
#include <KFormat>
#include <KLocalizedString>

using namespace Search;

Search::DateSelector::DateSelector(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent)
    : QToolButton{parent}
    , UpdatableStateInterface{dolphinQuery}
    , m_datePickerPopup{
          new KDatePickerPopup{KDatePickerPopup::NoDate | KDatePickerPopup::DatePicker | KDatePickerPopup::Words, dolphinQuery->modifiedSinceDate(), this}}
{
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setPopupMode(QToolButton::InstantPopup);

    m_datePickerPopup->setDateRange(QDate{}, QDate::currentDate());
    connect(m_datePickerPopup, &KDatePickerPopup::dateChanged, this, [this](const QDate &activatedDate) {
        if (activatedDate == m_searchConfiguration->modifiedSinceDate()) {
            return; // Already selected.
        }
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setModifiedSinceDate(activatedDate);
        Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
    });
    setMenu(m_datePickerPopup);

    updateStateToMatch(std::move(dolphinQuery));
}

void DateSelector::removeRestriction()
{
    Q_ASSERT(m_searchConfiguration->modifiedSinceDate().isValid());
    DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
    searchConfigurationCopy.setModifiedSinceDate(QDate{});
    Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
}

void DateSelector::updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery)
{
    m_datePickerPopup->setDate(dolphinQuery->modifiedSinceDate());
    if (!dolphinQuery->modifiedSinceDate().isValid()) {
        setIcon(QIcon{}); // No icon for the empty state
        setText(i18nc("@item:inlistbox", "Any Date"));
        return;
    }
    setIcon(QIcon::fromTheme(QStringLiteral("view-calendar")));
    QLocale local;
    KFormat formatter(local);
    setText(formatter.formatRelativeDate(dolphinQuery->modifiedSinceDate(), QLocale::ShortFormat));
}
