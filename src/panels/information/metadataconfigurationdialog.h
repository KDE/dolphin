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

#ifndef META_DATA_CONFIGURATION_DIALOG_H
#define META_DATA_CONFIGURATION_DIALOG_H

#include <kdialog.h>

class MetaDataWidget;

/**
 * @brief Dialog which allows to configure which meta data should be shown.
 */
class MetaDataConfigurationDialog : public KDialog
{
    Q_OBJECT

public:
    /**
     * Allows to configure the visibility of all available meta
     * data.
     */
    MetaDataConfigurationDialog(QWidget* parent = 0,
                                Qt::WFlags flags = 0);

    /**
     * Allows to configure the visibility of the meta data
     * shown by the meta data widget. The number of offered
     * meta data is optimized for the set of file items
     * that are applied to the meta data widget.
     */
    MetaDataConfigurationDialog(MetaDataWidget* metaDataWidget,
                                QWidget* parent = 0,
                                Qt::WFlags flags = 0);

    virtual ~MetaDataConfigurationDialog();

protected slots:
    virtual void slotButtonClicked(int button);

private:
    class Private;
    Private* d;
};

#endif
