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

#ifndef VIEWPROPERTIES_H
#define VIEWPROPERTIES_H

#include <dolphinview.h>
#include <kurl.h>
#include <qdatetime.h>

#include "directoryviewpropertysettings.h"

class QFile;

/**
 * @short Maintains the view properties like 'view mode' or 'show hidden files' for a directory.
 *
 * The view properties are automatically stored inside
 * the directory as hidden file called '.dolphinview'. To read out the view properties
 * just construct an instance by passing the Url of the directory:
 * \code
 * ViewProperties props(KUrl("/home/peter/Documents"));
 * const DolphinView::Mode mode = props.viewMode();
 * const bool showHiddenFiles = props.isShowHiddenFilesEnabled();
 * \endcode
 * When modifying a view property, the '.dolphinview' file is automatically updated
 * inside the destructor.
 *
 * @author Peter Penz
 */
// TODO: provide detailed design description, as mapping the user model to
// the physical modal is not trivial.
class ViewProperties
{
public:
    ViewProperties(const KUrl& url);
    virtual ~ViewProperties();

    void setViewMode(DolphinView::Mode mode);
    DolphinView::Mode viewMode() const;

    void setShowHiddenFilesEnabled(bool show);
    bool isShowHiddenFilesEnabled() const;

    void setSorting(DolphinView::Sorting sorting);
    DolphinView::Sorting sorting() const;

    void setSortOrder(Qt::SortOrder sortOrder);
    Qt::SortOrder sortOrder() const;

    void setAutoSaveEnabled(bool autoSave);
    bool isAutoSaveEnabled() const;

    void updateTimeStamp();
    void save();


private:
    bool m_changedProps;
    bool m_autoSave;
    QString m_filepath;
    ViewPropertySettings* m_node;

    ViewProperties(const ViewProperties& props);
    ViewProperties& operator= (const ViewProperties& props);
};

#endif
