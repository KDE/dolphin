/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2006 by Aaron J. Seigo <aseigo@kde.org>                 *
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

#include "viewproperties.h"

#include "additionalinfoaccessor.h"
#include "dolphin_directoryviewpropertysettings.h"
#include "dolphin_generalsettings.h"

#include <KComponentData>
#include <KLocale>
#include <KStandardDirs>
#include <KUrl>

#include <QDate>
#include <QFile>
#include <QFileInfo>

namespace {
    // String representation to mark the additional properties of
    // the details view as customized by the user. See
    // ViewProperties::additionalInfoList() for more information.
    const char* CustomizedDetailsString = "CustomizedDetails";
}

ViewProperties::ViewProperties(const KUrl& url) :
    m_changedProps(false),
    m_autoSave(true),
    m_node(0)
{
    GeneralSettings* settings = GeneralSettings::self();
    const bool useGlobalViewProps = settings->globalViewProps();
    bool useDetailsViewWithPath = false;

    // We try and save it to the file .directory in the directory being viewed.
    // If the directory is not writable by the user or the directory is not local,
    // we store the properties information in a local file.
    if (useGlobalViewProps) {
        m_filePath = destinationDir("global");
    } else if (url.protocol().contains("search")) {
        m_filePath = destinationDir("search");
        useDetailsViewWithPath = true;
    } else if (url.protocol() == QLatin1String("trash")) {
        m_filePath = destinationDir("trash");
        useDetailsViewWithPath = true;
    } else if (url.isLocalFile()) {
        m_filePath = url.toLocalFile();
        const QFileInfo info(m_filePath);
        if (!info.isWritable() || !isPartOfHome(m_filePath)) {
#ifdef Q_OS_WIN
			// m_filePath probably begins with C:/ - the colon is not a valid character for paths though
			m_filePath =  QDir::separator() + m_filePath.remove(QLatin1Char(':'));
#endif
            m_filePath = destinationDir("local") + m_filePath;
        }
    } else {
        m_filePath = destinationDir("remote") + m_filePath;
    }

    const QString file = m_filePath + QDir::separator() + QLatin1String(".directory");
    m_node = new ViewPropertySettings(KSharedConfig::openConfig(file));

    // If the .directory file does not exist or the timestamp is too old,
    // use default values instead.
    const bool useDefaultProps = (!useGlobalViewProps || useDetailsViewWithPath) &&
                                 (!QFileInfo(file).exists() ||
                                  (m_node->timestamp() < settings->viewPropsTimestamp()));
    if (useDefaultProps) {
        if (useDetailsViewWithPath) {
            setViewMode(DolphinView::DetailsView);
            setAdditionalInfoList(QList<DolphinView::AdditionalInfo>() << DolphinView::PathInfo);
        } else {
            // The global view-properties act as default for directories without
            // any view-property configuration
            settings->setGlobalViewProps(true);

            ViewProperties defaultProps(url);
            setDirProperties(defaultProps);

            settings->setGlobalViewProps(false);
            m_changedProps = false;
        }
    }
}

ViewProperties::~ViewProperties()
{
    if (m_changedProps && m_autoSave) {
        save();
    }

    delete m_node;
    m_node = 0;
}

void ViewProperties::setViewMode(DolphinView::Mode mode)
{
    if (m_node->viewMode() != mode) {
        m_node->setViewMode(mode);
        update();
    }
}

DolphinView::Mode ViewProperties::viewMode() const
{
    const int mode = qBound(0, m_node->viewMode(), 2);
    return static_cast<DolphinView::Mode>(mode);
}

void ViewProperties::setPreviewsShown(bool show)
{
    if (m_node->previewsShown() != show) {
        m_node->setPreviewsShown(show);
        update();
    }
}

bool ViewProperties::previewsShown() const
{
    return m_node->previewsShown();
}

void ViewProperties::setHiddenFilesShown(bool show)
{
    if (m_node->hiddenFilesShown() != show) {
        m_node->setHiddenFilesShown(show);
        update();
    }
}

void ViewProperties::setGroupedSorting(bool grouped)
{
    if (m_node->groupedSorting() != grouped) {
        m_node->setGroupedSorting(grouped);
        update();
    }
}

bool ViewProperties::groupedSorting() const
{
    return m_node->groupedSorting();
}

bool ViewProperties::hiddenFilesShown() const
{
    return m_node->hiddenFilesShown();
}

void ViewProperties::setSorting(DolphinView::Sorting sorting)
{
    if (m_node->sorting() != sorting) {
        m_node->setSorting(sorting);
        update();
    }
}

DolphinView::Sorting ViewProperties::sorting() const
{
    return static_cast<DolphinView::Sorting>(m_node->sorting());
}

void ViewProperties::setSortOrder(Qt::SortOrder sortOrder)
{
    if (m_node->sortOrder() != sortOrder) {
        m_node->setSortOrder(sortOrder);
        update();
    }
}

Qt::SortOrder ViewProperties::sortOrder() const
{
    return static_cast<Qt::SortOrder>(m_node->sortOrder());
}

void ViewProperties::setSortFoldersFirst(bool foldersFirst)
{
    if (m_node->sortFoldersFirst() != foldersFirst) {
        m_node->setSortFoldersFirst(foldersFirst);
        update();
    }
}

bool ViewProperties::sortFoldersFirst() const
{
    return m_node->sortFoldersFirst();
}

void ViewProperties::setAdditionalInfoList(const QList<DolphinView::AdditionalInfo>& list)
{
    // See ViewProperties::additionalInfoList() for the storage format
    // of the additional information.

    // Remove the old values stored for the current view-mode
    const QStringList oldInfoStringList = m_node->additionalInfo();
    const QString prefix = viewModePrefix();
    QStringList newInfoStringList = oldInfoStringList;
    for (int i = newInfoStringList.count() - 1; i >= 0; --i) {
        if (newInfoStringList.at(i).startsWith(prefix)) {
            newInfoStringList.removeAt(i);
        }
    }

    // Add the updated values for the current view-mode
    AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
    foreach (DolphinView::AdditionalInfo info, list) {
        newInfoStringList.append(prefix + infoAccessor.value(info));
    }

    if (oldInfoStringList != newInfoStringList) {
        const bool markCustomizedDetails = (m_node->viewMode() == DolphinView::DetailsView)
                                           && !newInfoStringList.contains(CustomizedDetailsString);
        if (markCustomizedDetails) {
            // The additional information of the details-view has been modified. Set a marker,
            // so that it is allowed to also show no additional information
            // (see fallback in ViewProperties::additionalInfoV2, if no additional information is
            // available).
            newInfoStringList.append(CustomizedDetailsString);
        }

        m_node->setAdditionalInfo(newInfoStringList);
        update();
    }
}

QList<DolphinView::AdditionalInfo> ViewProperties::additionalInfoList() const
{
    // The shown additional information is stored for each view-mode separately as
    // string with the view-mode as prefix. Example:
    //
    // AdditionalInfo=Details_Size,Details_Date,Details_Owner,Icon_Size
    //
    // To get the representation as QList<DolphinView::AdditionalInfo>, the current
    // view-mode must be checked and the values of this mode added to the list.
    //
    // For the details-view a special case must be respected: Per default the size
    // and date should be shown without creating a .directory file. Only if
    // the user explictly has modified the properties of the details view (marked
    // by "CustomizedDetails"), also a details-view with no additional information
    // is accepted.

    QList<DolphinView::AdditionalInfo> usedInfo;

    // infoHash allows to get the mapped DolphinView::AdditionalInfo value
    // for a stored string-value in a fast way
    static QHash<QString, DolphinView::AdditionalInfo> infoHash;
    if (infoHash.isEmpty()) {
        AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
        const QList<DolphinView::AdditionalInfo> keys = infoAccessor.keys();
        foreach (DolphinView::AdditionalInfo key, keys) {
            infoHash.insert(infoAccessor.value(key), key);
        }
    }

    // Iterate through all stored keys stored as strings and map them to
    // the corresponding DolphinView::AdditionalInfo values.
    const QString prefix = viewModePrefix();
    const int prefixLength = prefix.length();
    const QStringList infoStringList = m_node->additionalInfo();
    foreach (const QString& infoString, infoStringList) {
        if (infoString.startsWith(prefix)) {
            const QString key = infoString.right(infoString.length() - prefixLength);
            if (infoHash.contains(key)) {
                usedInfo.append(infoHash.value(key));
            } else {
                kWarning() << "Did not find the key" << key << "in the information string";
            }
        }
    }

    // For the details view the size and date should be shown per default
    // until the additional information has been explicitly changed by the user
    const bool useDefaultValues = usedInfo.isEmpty()
                                  && (m_node->viewMode() == DolphinView::DetailsView)
                                  && !infoStringList.contains(CustomizedDetailsString);
    Q_UNUSED(useDefaultValues);
    if (useDefaultValues) {
        usedInfo.append(DolphinView::SizeInfo);
        usedInfo.append(DolphinView::DateInfo);
    }

    return usedInfo;
}

void ViewProperties::setDirProperties(const ViewProperties& props)
{
    setViewMode(props.viewMode());
    setPreviewsShown(props.previewsShown());
    setHiddenFilesShown(props.hiddenFilesShown());
    setGroupedSorting(props.groupedSorting());
    setSorting(props.sorting());
    setSortOrder(props.sortOrder());
    setSortFoldersFirst(props.sortFoldersFirst());
    setAdditionalInfoList(props.additionalInfoList());
}

void ViewProperties::setAutoSaveEnabled(bool autoSave)
{
    m_autoSave = autoSave;
}

bool ViewProperties::isAutoSaveEnabled() const
{
    return m_autoSave;
}

void ViewProperties::update()
{
    m_changedProps = true;
    m_node->setTimestamp(QDateTime::currentDateTime());
}

void ViewProperties::save()
{
    KStandardDirs::makeDir(m_filePath);
    m_node->writeConfig();
    m_changedProps = false;
}

KUrl ViewProperties::mirroredDirectory()
{
    QString basePath = KGlobal::mainComponent().componentName();
    basePath.append("/view_properties/");
    return KUrl(KStandardDirs::locateLocal("data", basePath));
}

QString ViewProperties::destinationDir(const QString& subDir) const
{
    QString basePath = KGlobal::mainComponent().componentName();
    basePath.append("/view_properties/").append(subDir);
    return KStandardDirs::locateLocal("data", basePath);
}

QString ViewProperties::viewModePrefix() const
{
    QString prefix;

    switch (m_node->viewMode()) {
    case DolphinView::IconsView:   prefix = "Icons_"; break;
    case DolphinView::CompactView: prefix = "Compact_"; break;
    case DolphinView::DetailsView: prefix = "Details_"; break;
    default: kWarning() << "Unknown view-mode of the view properties";
    }

    return prefix;
}

bool ViewProperties::isPartOfHome(const QString& filePath)
{
    // For performance reasons cache the path in a static QString
    // (see QDir::homePath() for more details)
    static QString homePath;
    if (homePath.isEmpty()) {
        homePath = QDir::homePath();
        Q_ASSERT(!homePath.isEmpty());
    }

    return filePath.startsWith(homePath);
}
