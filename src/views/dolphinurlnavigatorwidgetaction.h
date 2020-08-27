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

#ifndef DOLPHINURLNAVIGATORWIDGETACTION_H
#define DOLPHINURLNAVIGATORWIDGETACTION_H

#include "dolphinurlnavigator.h"

#include <QWidgetAction>

class KXmlGuiWindow;
class QStackedWidget;

/**
 * @brief QWidgetAction that allows to use a KUrlNavigator in a toolbar.
 * 
 * When the UrlNavigator of this object is not in use,
 * setUrlNavigatorVisible(false) is used to hide it. It will then be
 * replaced in the toolbar by an empty expanding spacer. This makes sure
 * that the other widgets in the toolbar will not change location when
 * switching the UrlNavigators visibility.
 */
class DolphinUrlNavigatorWidgetAction : public QWidgetAction
{
    Q_OBJECT

public:
    DolphinUrlNavigatorWidgetAction(QWidget *parent = nullptr);

    DolphinUrlNavigator *urlNavigator() const;

    /**
     * Sets the QStackedWidget which is the defaultWidget() to either
     * show a KUrlNavigator or an expanding spacer.
     */
    void setUrlNavigatorVisible(bool visible);

    /**
     * Adds this action to the mainWindow's toolbar and saves the change
     * in the users ui configuration file.
     * @return true if successful. Otherwise false.
     * @note This method does multiple things which are discouraged in
     *       the API documentation.
     */
    bool addToToolbarAndSave(KXmlGuiWindow *mainWindow);

private:
    QStackedWidget *m_stackedWidget;
};

#endif // DOLPHINURLNAVIGATORWIDGETACTION_H
