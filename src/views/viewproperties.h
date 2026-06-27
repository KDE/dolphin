/*
 * SPDX-FileCopyrightText: 2006-2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Aaron J. Seigo <aseigo@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWPROPERTIES_H
#define VIEWPROPERTIES_H

#include "dolphin_export.h"
#include "views/dolphinview.h"

#include <QUrl>

class ViewPropertySettings;
/**
 * @brief Maintains the view properties like 'view mode' or
 *        'show hidden files' for a directory.
 *
 * The view properties are automatically stored as part of the file
 * .directory inside the corresponding path. To read out the view properties
 * just construct an instance by passing the path of the directory:
 *
 * \code
 * ViewProperties props(QUrl::fromLocalFile("/home/peter/Documents"));
 * const DolphinView::Mode mode = props.viewMode();
 * const bool hiddenFilesShown = props.hiddenFilesShown();
 * \endcode
 *
 * When modifying a view property, the '.directory' file is automatically updated
 * inside the destructor.
 *
 * If no .directory file is available or the global view mode is turned on
 * (see GeneralSettings::globalViewMode()), the values from the global .directory file
 * are used for initialization.
 */
class DOLPHIN_EXPORT ViewProperties
{
public:
    explicit ViewProperties(const QUrl &url);
    virtual ~ViewProperties();

    void setViewMode(DolphinView::Mode mode);
    DolphinView::Mode viewMode() const;

    void setZoomLevel(int zoomLevel);
    /// -1 is the default zoom
    int zoomLevel() const;

    void setPreviewsShown(bool show);
    bool previewsShown() const;

    void setHiddenFilesShown(bool show);
    bool hiddenFilesShown() const;

    void setGroupedSorting(bool grouped);
    bool groupedSorting() const;

    void setSortRole(const QByteArray &role);
    QByteArray sortRole() const;

    void setGroupRole(const QByteArray &role);
    QByteArray groupRole() const;

    void setSortOrder(Qt::SortOrder sortOrder);
    Qt::SortOrder sortOrder() const;

    void setSortFoldersFirst(bool foldersFirst);
    bool sortFoldersFirst() const;

    void setSortHiddenLast(bool hiddenLast);
    bool sortHiddenLast() const;

    void setDynamicViewPassed(bool dynamicViewPassed);
    bool dynamicViewPassed() const;

    /**
     * Sets the additional information for the current set view-mode.
     * Note that the additional-info property is the only property where
     * the value is dependent from another property (in this case the view-mode).
     */
    void setVisibleRoles(const QList<QByteArray> &info);

    /**
     * Returns the additional information for the current set view-mode.
     * Note that the additional-info property is the only property where
     * the value is dependent from another property (in this case the view-mode).
     */
    QList<QByteArray> visibleRoles() const;

    void setHeaderColumnWidths(const QList<int> &widths);
    QList<int> headerColumnWidths() const;

    /**
     * Sets the directory properties view mode, show preview,
     * show hidden files, sorting and sort order like
     * set in \a props.
     */
    void setDirProperties(const ViewProperties &props);

    /**
     * If \a autoSave is true, the properties are automatically
     * saved when the destructor is called. Per default autosaving
     * is enabled.
     */
    void setAutoSaveEnabled(bool autoSave);
    bool isAutoSaveEnabled() const;

    void update();

    /**
     * Saves the view properties for the directory specified
     * in the constructor. The method is automatically
     * invoked in the destructor, if
     * ViewProperties::isAutoSaveEnabled() returns true and
     * at least one property has been changed.
     */
    void save();

    /**
     * Returns the destination directory path where the view
     * properties are stored. \a subDir specifies the used sub
     * directory.
     */
    QString destinationDir(const QString &subDir) const;

    /**
     * @return A hash-value for a URL that can be used as a directory name.
     *         Used to store view-properties for non-local or long URLs without
     *         embedding the path (which can be long or privacy-sensitive).
     */
    static QString directoryHashForUrl(const QUrl &url);

    /**
     * Restore the view settings to the default settings
     * Depending on the view type.
     *
     * Only useful when in per-folder settings
     */
    void restoreToDefaults();
    bool isDefaults() const;

private:
    /**
     * Returns the view-mode prefix when storing additional properties for
     * a view-mode.
     */
    QString viewModePrefix() const;

    /**
     * Provides backward compatibility with .directory files created with
     * Dolphin < 2.0: Converts the old additionalInfo-property into
     * the visibleRoles-property and clears the additionalInfo-property.
     */
    void convertAdditionalInfo();

    /**
     * Provides backward compatibility with .directory files created with
     * Dolphin < 2.1: Converts the old name-role "name" to the generic
     * role "text".
     */
    void convertNameRoleToTextRole();

    /**
     * Provides backward compatibility with .directory files created with
     * Dolphin < 16.11.70: Converts the old name-role "date" to "modificationtime"
     */

    void convertDateRoleToModificationTimeRole();
    /**
     * Returns true, if \a filePath is part of the home-path (see QDir::homePath()).
     */
    static bool isPartOfHome(const QString &filePath);

    /** @returns a ViewPropertySettings object with properties loaded for the directory at @param filePath. Ownership is returned to the caller. */
    ViewPropertySettings *loadProperties(const QString &folderPath) const;
    /** @returns a ViewPropertySettings object with the globally configured default values. Ownership is returned to the caller. */
    ViewPropertySettings *defaultProperties() const;

    Q_DISABLE_COPY(ViewProperties)

private:
    bool m_changedProps;
    bool m_autoSave;
    QString m_filePath;
    ViewPropertySettings *m_node;
};

#endif
