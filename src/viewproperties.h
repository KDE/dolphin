/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (<peter.penz@gmx.at>)                *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef VIEWPROPERTIES_H
#define VIEWPROPERTIES_H

#include <dolphinview.h>
#include <kfileitemdelegate.h>
#include <kurl.h>
#include <QtCore/QDate>
#include <libdolphin_export.h>
#include "dolphin_directoryviewpropertysettings.h"

class QFile;

/**
 * @brief Maintains the view properties like 'view mode' or
 *        'show hidden files' for a directory.
 *
 * The view properties are automatically stored as part of the file
 * .directory inside the corresponding path. To read out the view properties
 * just construct an instance by passing the path of the directory:
 *
 * \code
 * ViewProperties props(KUrl("/home/peter/Documents"));
 * const DolphinView::Mode mode = props.viewMode();
 * const bool showHiddenFiles = props.isShowHiddenFilesEnabled();
 * \endcode
 *
 * When modifying a view property, the '.directory' file is automatically updated
 * inside the destructor.
 *
 * If no .directory file is available or the global view mode is turned on
 * (see GeneralSettings::globalViewMode()), the values from the global .directory file
 * are used for initialization.
 */
class LIBDOLPHINPRIVATE_EXPORT ViewProperties
{
public:
    explicit ViewProperties(const KUrl& url);
    virtual ~ViewProperties();

    void setViewMode(DolphinView::Mode mode);
    DolphinView::Mode viewMode() const;

    void setShowPreview(bool show);
    bool showPreview() const;

    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    void setCategorizedSorting(bool categorized);
    bool categorizedSorting() const;

    void setSorting(DolphinView::Sorting sorting);
    DolphinView::Sorting sorting() const;

    void setSortOrder(Qt::SortOrder sortOrder);
    Qt::SortOrder sortOrder() const;

    void setAdditionalInfo(KFileItemDelegate::AdditionalInformation info);
    KFileItemDelegate::AdditionalInformation additionalInfo() const;

    /**
     * Sets the directory properties view mode, show preview,
     * show hidden files, sorting and sort order like
     * set in \a props.
     */
    void setDirProperties(const ViewProperties& props);

    /**
     * If \a autoSave is true, the properties are automatically
     * saved when the destructor is called. Per default autosaving
     * is enabled.
     */
    void setAutoSaveEnabled(bool autoSave);
    bool isAutoSaveEnabled() const;

    void updateTimeStamp();

    /**
     * Saves the view properties for the directory specified
     * in the constructor. The method is automatically
     * invoked in the destructor, if
     * ViewProperties::isAutoSaveEnabled() returns true and
     * at least one property has been changed.
     */
    void save();

    /**
     * Returns the URL of the directory, where the mirrored view properties
     * are stored into. Mirrored view properties are used if:
     * - there is no write access for storing the view properties into
     *   the original directory
     * - for non local directories
     */
    static KUrl mirroredDirectory();

private:
    /**
     * Returns the destination directory path where the view
     * properties are stored. \a subDir specifies the used sub
     * directory.
     */
    QString destinationDir(const QString& subDir) const;

    Q_DISABLE_COPY(ViewProperties)

private:
    bool m_changedProps;
    bool m_autoSave;
    QString m_filepath;
    ViewPropertySettings* m_node;
};

#endif
