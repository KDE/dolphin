/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef VIEWEXTENSIONSFACTORY_H
#define VIEWEXTENSIONSFACTORY_H

#include <QObject>

class DolphinController;
class KFilePreviewGenerator;
class ToolTipManager;
class QAbstractItemView;

/**
 * @brief Responsible for creating extensions like tooltips and previews
 *        that are available in all view implementations.
 *
 * Each view implementation (iconsview, detailsview, columnview) must
 * instantiate an instance of this class to assure having
 * a common behavior that is independent from the custom functionality of
 * a view implementation.
 */
class ViewExtensionsFactory : public QObject
{
    Q_OBJECT

    public:
        explicit ViewExtensionsFactory(QAbstractItemView* view,
                                       DolphinController* controller);
        virtual ~ViewExtensionsFactory();

    private slots:
        /**
         * Tells the preview generator to update all icons.
         */
        void updateIcons();

        /**
         * Tells the preview generator to cancel all pending previews.
         */
        void cancelPreviews();

        void slotShowPreviewChanged();

    private:
        DolphinController* m_controller;
        ToolTipManager* m_toolTipManager;        
        KFilePreviewGenerator* m_previewGenerator;
};

#endif

