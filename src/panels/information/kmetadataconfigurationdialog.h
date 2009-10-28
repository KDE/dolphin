/*****************************************************************************
 * Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef KMETA_DATA_CONFIGURATION_DIALOG_H
#define KMETA_DATA_CONFIGURATION_DIALOG_H

#include <kdialog.h>
#include <kfile_export.h>

class KMetaDataWidget;

/**
 * @brief Dialog which allows to configure which meta data should be shown.
 * @see KMetaDataWidget
 */
class KFILE_EXPORT KMetaDataConfigurationDialog : public KDialog
{
    Q_OBJECT

public:
    /**
     * Allows to configure the visibility of all available meta
     * data.
     */
    KMetaDataConfigurationDialog(QWidget* parent = 0,
                                 Qt::WFlags flags = 0);

    /**
     * Allows to configure the visibility of the meta data
     * shown by the meta data widget. The number of offered
     * meta data is optimized for the set of file items
     * that are applied to the meta data widget.
     */
    KMetaDataConfigurationDialog(KMetaDataWidget* metaDataWidget,
                                 QWidget* parent = 0,
                                 Qt::WFlags flags = 0);

    virtual ~KMetaDataConfigurationDialog();

protected slots:
    virtual void slotButtonClicked(int button);

private:
    class Private;
    Private* d;
};

#endif
