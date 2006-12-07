/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef VIEWPROPERTIESDIALOG_H
#define VIEWPROPERTIESDIALOG_H

#include <kdialog.h>

class QCheckBox;
class QComboBox;
class QRadioButton;
class ViewProperties;
class DolphinView;

/**
 * @brief Dialog for changing the current view properties of a directory.
 *
 * It is possible to specify the view mode and whether hidden files
 * should be shown. The properties can be assigned to the current folder,
 * recursively to all sub folders or to all folders.
 *
 * @author Peter Penz
 */
class ViewPropertiesDialog : public KDialog
{
    Q_OBJECT

public:
    ViewPropertiesDialog(DolphinView* dolphinView);
    virtual ~ViewPropertiesDialog();

private slots:
    void slotOk();
    void slotApply();
    void slotViewModeChanged(int index);
    void slotSortingChanged(int index);
    void slotSortOrderChanged(int index);
    void slotShowHiddenFilesChanged();
    void markAsDirty();

private:
    bool m_isDirty;
    DolphinView* m_dolphinView;
    ViewProperties* m_viewProps;

    QComboBox* m_viewMode;
    QComboBox* m_sorting;
    QComboBox* m_sortOrder;
    QCheckBox* m_showHiddenFiles;
    QRadioButton* m_applyToCurrentFolder;
    QRadioButton* m_applyToSubFolders;
    QRadioButton* m_applyToAllFolders;

    void applyViewProperties();
};

#endif
