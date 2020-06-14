/*
 * Copyright 2020  Felix Ernst <fe.a.ernst@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dolphinurlnavigatorwidgetaction.h"

#include "dolphin_generalsettings.h"
#include "dolphinviewcontainer.h"

#include <KLocalizedString>

DolphinUrlNavigatorWidgetAction::DolphinUrlNavigatorWidgetAction(QWidget *parent) :
    QWidgetAction(parent)
{
    setText(i18nc("@action:inmenu", "Url navigator"));

    m_stackedWidget = new QStackedWidget(parent);

    auto expandingSpacer = new QWidget(m_stackedWidget);
    expandingSpacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_stackedWidget->addWidget(expandingSpacer); // index 0 of QStackedWidget

    auto urlNavigator = new DolphinUrlNavigator(m_stackedWidget);
    m_stackedWidget->addWidget(urlNavigator); // index 1 of QStackedWidget

    setDefaultWidget(m_stackedWidget);
    setUrlNavigatorVisible(GeneralSettings::locationInToolbar());
}

DolphinUrlNavigator* DolphinUrlNavigatorWidgetAction::urlNavigator() const
{
    return static_cast<DolphinUrlNavigator *>(m_stackedWidget->widget(1));
}

void DolphinUrlNavigatorWidgetAction::setUrlNavigatorVisible(bool visible)
{
    if (!visible) {
        m_stackedWidget->setCurrentIndex(0); // expandingSpacer
    } else {
        m_stackedWidget->setCurrentIndex(1); // urlNavigator
    }
}
