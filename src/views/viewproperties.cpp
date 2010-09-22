/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz@gmx.at>             *
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

#include <kcomponentdata.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <QDate>
#include <QFile>
#include <QFileInfo>

#include "settings/dolphinsettings.h"

namespace {
    // String representation to mark the additional properties of
    // the details view as customized by the user. See
    // ViewProperties::additionalInfoV2() for more information.
    const char* CustomizedDetailsString = "CustomizedDetails";
}

ViewProperties::ViewProperties(const KUrl& url) :
    m_changedProps(false),
    m_autoSave(true),
    m_node(0)
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    const bool useGlobalViewProps = settings->globalViewProps();

    // We try and save it to the file .directory in the directory being viewed.
    // If the directory is not writable by the user or the directory is not local,
    // we store the properties information in a local file.
    const bool isSearchUrl = url.protocol().contains("search");
    if (isSearchUrl) {
        m_filePath = destinationDir("search");
    } else if (useGlobalViewProps) {
        m_filePath = destinationDir("global");
    } else if (url.isLocalFile()) {
        m_filePath = url.toLocalFile();
        const QFileInfo info(m_filePath);
        if (!info.isWritable()) {
            m_filePath = destinationDir("local") + m_filePath;
        }
    } else {
        m_filePath = destinationDir("remote") + m_filePath;
    }

    const QString file = m_filePath + QDir::separator() + QLatin1String(".directory");
    m_node = new ViewPropertySettings(KSharedConfig::openConfig(file));

    // If the .directory file does not exist or the timestamp is too old,
    // use default values instead.
    const bool useDefaultProps = (!useGlobalViewProps || isSearchUrl) &&
                                 (!QFileInfo(file).exists() ||
                                  (m_node->timestamp() < settings->viewPropsTimestamp()));
    if (useDefaultProps) {
        if (isSearchUrl) {
            setViewMode(DolphinView::DetailsView);
            setAdditionalInfo(KFileItemDelegate::InformationList() << KFileItemDelegate::LocalPathOrUrl);
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
    return static_cast<DolphinView::Mode>(m_node->viewMode());
}

void ViewProperties::setShowPreview(bool show)
{
    if (m_node->showPreview() != show) {
        m_node->setShowPreview(show);
        update();
    }
}

bool ViewProperties::showPreview() const
{
    return m_node->showPreview();
}

void ViewProperties::setShowHiddenFiles(bool show)
{
    if (m_node->showHiddenFiles() != show) {
        m_node->setShowHiddenFiles(show);
        update();
    }
}

void ViewProperties::setCategorizedSorting(bool categorized)
{
    if (m_node->categorizedSorting() != categorized) {
        m_node->setCategorizedSorting(categorized);
        update();
    }
}

bool ViewProperties::categorizedSorting() const
{
    return m_node->categorizedSorting();
}

bool ViewProperties::showHiddenFiles() const
{
    return m_node->showHiddenFiles();
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

void ViewProperties::setAdditionalInfo(const KFileItemDelegate::InformationList& list)
{
    // See ViewProperties::additionalInfoV2() for the storage format
    // of the additional information.

    // Remove the old values stored for the current view-mode
    const QStringList oldInfoStringList = m_node->additionalInfoV2();
    const QString prefix = viewModePrefix();
    QStringList newInfoStringList = oldInfoStringList;
    for (int i = newInfoStringList.count() - 1; i >= 0; --i) {
        if (newInfoStringList.at(i).startsWith(prefix)) {
            newInfoStringList.removeAt(i);
        }
    }

    // Add the updated values for the current view-mode
    AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
    foreach (KFileItemDelegate::Information info, list) {
        newInfoStringList.append(prefix + infoAccessor.value(info));
    }

    // Only update the information if it has been changed
    bool changed = oldInfoStringList.count() != newInfoStringList.count();
    if (!changed) {
        foreach (const QString& oldInfoString, oldInfoStringList) {
            if (!newInfoStringList.contains(oldInfoString)) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        if (m_node->version() < 2) {
            m_node->setVersion(2);
        }

        const bool markCustomizedDetails = (m_node->viewMode() == DolphinView::DetailsView)
                                           && !newInfoStringList.contains(CustomizedDetailsString);
        if (markCustomizedDetails) {
            // The additional information of the details-view has been modified. Set a marker,
            // so that it is allowed to also show no additional information
            // (see fallback in ViewProperties::additionalInfoV2, if no additional information is
            // available).
            newInfoStringList.append(CustomizedDetailsString);
        }

        m_node->setAdditionalInfoV2(newInfoStringList);
        update();
    }
}

KFileItemDelegate::InformationList ViewProperties::additionalInfo() const
{
    KFileItemDelegate::InformationList usedInfo;

    switch (m_node->version()) {
    case 1: usedInfo = additionalInfoV1(); break;
    case 2: usedInfo = additionalInfoV2(); break;
    default: kWarning() << "Unknown version of the view properties";
    }

    return usedInfo;
}


void ViewProperties::setDirProperties(const ViewProperties& props)
{
    setViewMode(props.viewMode());
    setShowPreview(props.showPreview());
    setShowHiddenFiles(props.showHiddenFiles());
    setCategorizedSorting(props.categorizedSorting());
    setSorting(props.sorting());
    setSortOrder(props.sortOrder());
    setSortFoldersFirst(props.sortFoldersFirst());
    setAdditionalInfo(props.additionalInfo());
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

    // If the view-properties are stored in an older format, take
    // care to update them to the current format.
    switch (m_node->version()) {
    case 1: {
        const KFileItemDelegate::InformationList infoList = additionalInfoV1();
        m_node->setVersion(2);
        setAdditionalInfo(infoList);
        break;
    }
    case 2:
        // Current version. Nothing needs to get converted.
        break;
    default:
        kWarning() << "Unknown version of the view properties";
    }
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

KFileItemDelegate::InformationList ViewProperties::additionalInfoV1() const
{
    KFileItemDelegate::InformationList usedInfo;

    int decodedInfo = m_node->additionalInfo();

    switch (viewMode()) {
    case DolphinView::DetailsView:
        decodedInfo = decodedInfo & 0xFF;
        if (decodedInfo == 0) {
            // A details view without any additional info makes no sense, hence
            // provide at least a size-info and date-info as fallback
            AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
            decodedInfo = infoAccessor.bitValue(KFileItemDelegate::Size) |
                          infoAccessor.bitValue(KFileItemDelegate::ModificationTime);
        }
        break;
    case DolphinView::IconsView:
        decodedInfo = (decodedInfo >> 8) & 0xFF;
        break;
    case DolphinView::ColumnView:
        decodedInfo = (decodedInfo >> 16) & 0xFF;
        break;
    default: break;
    }

    AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
    const KFileItemDelegate::InformationList infoKeys = infoAccessor.keys();

    foreach (const KFileItemDelegate::Information info, infoKeys) {
        if (decodedInfo & infoAccessor.bitValue(info)) {
            usedInfo.append(info);
        }
    }

    return usedInfo;
}

KFileItemDelegate::InformationList ViewProperties::additionalInfoV2() const
{
    // The shown additional information is stored for each view-mode separately as
    // string with the view-mode as prefix. Example:
    //
    // AdditionalInfoV2=Details_Size,Details_Date,Details_Owner,Icon_Size
    //
    // To get the representation as KFileItemDelegate::InformationList, the current
    // view-mode must be checked and the values of this mode added to the list.
    //
    // For the details-view a special case must be respected: Per default the size
    // and date should be shown without creating a .directory file. Only if
    // the user explictly has modified the properties of the details view (marked
    // by "CustomizedDetails"), also a details-view with no additional information
    // is accepted.

    KFileItemDelegate::InformationList usedInfo;

    // infoHash allows to get the mapped KFileItemDelegate::Information value
    // for a stored string-value in a fast way
    static QHash<QString, KFileItemDelegate::Information> infoHash;
    if (infoHash.isEmpty()) {
        AdditionalInfoAccessor& infoAccessor = AdditionalInfoAccessor::instance();
        const KFileItemDelegate::InformationList keys = infoAccessor.keys();
        foreach (const KFileItemDelegate::Information key, keys) {
            infoHash.insert(infoAccessor.value(key), key);
        }
    }

    // Iterate through all stored keys stored as strings and map them to
    // the corresponding KFileItemDelegate::Information values.
    const QString prefix = viewModePrefix();
    const int prefixLength = prefix.length();
    const QStringList infoStringList = m_node->additionalInfoV2();
    foreach (const QString& infoString, infoStringList) {
        if (infoString.startsWith(prefix)) {
            const QString key = infoString.right(infoString.length() - prefixLength);
            Q_ASSERT(infoHash.contains(key));
            usedInfo.append(infoHash.value(key));
        }
    }

    // For the details view the size and date should be shown per default
    // until the additional information has been explicitly changed by the user
    const bool useDefaultValues = usedInfo.isEmpty()
                                  && (m_node->viewMode() == DolphinView::DetailsView)
                                  && !infoStringList.contains(CustomizedDetailsString);
    if (useDefaultValues) {
        usedInfo.append(KFileItemDelegate::Size);
        usedInfo.append(KFileItemDelegate::ModificationTime);
    }

    return usedInfo;
}

QString ViewProperties::viewModePrefix() const
{
    QString prefix;

    switch (m_node->viewMode()) {
    case DolphinView::DetailsView: prefix = "Details_"; break;
    case DolphinView::IconsView:   prefix = "Icons_"; break;
    case DolphinView::ColumnView:  prefix = "Column_"; break;
    default: kWarning() << "Unknown view-mode of the view properties";
    }

    return prefix;
}
