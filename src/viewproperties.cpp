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

#include "viewproperties.h"

#include "settings/dolphinsettings.h"
#include "dolphin_directoryviewpropertysettings.h"
#include "dolphin_generalsettings.h"

#include <kcomponentdata.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <QDate>
#include <QFile>
#include <QFileInfo>

bool ViewProperties::m_nepomukSupport = false;

#define FILE_NAME "/.directory"

ViewProperties::ViewProperties(const KUrl& url) :
    m_changedProps(false),
    m_autoSave(true),
    m_node(0)
{
    KUrl cleanUrl(url);
    cleanUrl.cleanPath();
    m_filepath = cleanUrl.toLocalFile();

    if ((m_filepath.length() < 1) || (!QDir::isAbsolutePath(m_filepath))) {
        const QString file = destinationDir("global") + FILE_NAME;
        m_node = new ViewPropertySettings(KSharedConfig::openConfig(file));
        return;
    }

    // We try and save it to a file in the directory being viewed.
    // If the directory is not writable by the user or the directory is not local,
    // we store the properties information in a local file.
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    const bool useGlobalViewProps = settings->globalViewProps();
    if (useGlobalViewProps) {
        m_filepath = destinationDir("global");
    } else if (cleanUrl.isLocalFile()) {
        const QFileInfo info(m_filepath);
        if (!info.isWritable()) {
            m_filepath = destinationDir("local") + m_filepath;
        }
    } else {
        m_filepath = destinationDir("remote") + m_filepath;
    }

    const QString file = m_filepath + FILE_NAME;
    m_node = new ViewPropertySettings(KSharedConfig::openConfig(file));

    const bool useDefaultProps = !useGlobalViewProps &&
                                 (!QFileInfo(file).exists() ||
                                  (m_node->timestamp() < settings->viewPropsTimestamp()));
    if (useDefaultProps) {
        // If the .directory file does not exist or the timestamp is too old,
        // use the values from the global .directory file instead, which acts
        // as default view for new folders in this case.
        settings->setGlobalViewProps(true);

        ViewProperties defaultProps(url);
        setDirProperties(defaultProps);

        settings->setGlobalViewProps(false);
        m_changedProps = false;
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
        updateTimeStamp();
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
        updateTimeStamp();
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
        updateTimeStamp();
    }
}

void ViewProperties::setCategorizedSorting(bool categorized)
{
    if (m_node->categorizedSorting() != categorized) {
        m_node->setCategorizedSorting(categorized);
        updateTimeStamp();
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
        updateTimeStamp();
    }
}

DolphinView::Sorting ViewProperties::sorting() const
{
    // If Nepomuk is not available, return SortByName as fallback if SortByRating
    // or SortByTags is stored.
    DolphinView::Sorting sorting = static_cast<DolphinView::Sorting>(m_node->sorting());
    const bool sortByName = !m_nepomukSupport &&
                            ((sorting == DolphinView::SortByRating) || (sorting == DolphinView::SortByTags));
    if (sortByName) {
        sorting = DolphinView::SortByName;
    }
    return sorting;
}

void ViewProperties::setSortOrder(Qt::SortOrder sortOrder)
{
    if (m_node->sortOrder() != sortOrder) {
        m_node->setSortOrder(sortOrder);
        updateTimeStamp();
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
        updateTimeStamp();
    }
}

bool ViewProperties::sortFoldersFirst() const
{
    return m_node->sortFoldersFirst();
}

void ViewProperties::setAdditionalInfo(KFileItemDelegate::InformationList list)
{
    int info = NoInfo;
    foreach (KFileItemDelegate::Information currentInfo, list) {
        switch (currentInfo) {
        case KFileItemDelegate::Size:
            info = info | SizeInfo;
            break;
        case KFileItemDelegate::ModificationTime:
            info = info | DateInfo;
            break;
        case KFileItemDelegate::Permissions:
            info = info | PermissionsInfo;
            break;
        case KFileItemDelegate::Owner:
            info = info | OwnerInfo;
            break;
        case KFileItemDelegate::OwnerAndGroup:
            info = info | GroupInfo;
            break;
        case KFileItemDelegate::FriendlyMimeType:
            info = info | TypeInfo;
            break;
        default:
            break;
        }
    }

    const int encodedInfo = encodedAdditionalInfo(info);
    if (m_node->additionalInfo() != encodedInfo) {
        m_node->setAdditionalInfo(encodedInfo);
        updateTimeStamp();
    }
}

KFileItemDelegate::InformationList ViewProperties::additionalInfo() const
{
    const int info = decodedAdditionalInfo();

    KFileItemDelegate::InformationList list;
    if (info & SizeInfo) {
        list.append(KFileItemDelegate::Size);
    }
    if (info & DateInfo) {
        list.append(KFileItemDelegate::ModificationTime);
    }
    if (info & PermissionsInfo) {
        list.append(KFileItemDelegate::Permissions);
    }
    if (info & OwnerInfo) {
        list.append(KFileItemDelegate::Owner);
    }
    if (info & GroupInfo) {
        list.append(KFileItemDelegate::OwnerAndGroup);
    }
    if (info & TypeInfo) {
        list.append(KFileItemDelegate::FriendlyMimeType);
    }

    return list;
}


void ViewProperties::setDirProperties(const ViewProperties& props)
{
    setViewMode(props.viewMode());
    setShowPreview(props.showPreview());
    setShowHiddenFiles(props.showHiddenFiles());
    setCategorizedSorting(props.categorizedSorting());
    setSorting(props.sorting());
    setSortOrder(props.sortOrder());
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

void ViewProperties::updateTimeStamp()
{
    m_changedProps = true;
    m_node->setTimestamp(QDateTime::currentDateTime());
}

void ViewProperties::save()
{
    KStandardDirs::makeDir(m_filepath);
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

int ViewProperties::encodedAdditionalInfo(int info) const
{
    int encodedInfo = m_node->additionalInfo();

    switch (viewMode()) {
    case DolphinView::DetailsView:
        encodedInfo = (encodedInfo & 0xFFFF00) | info;
        break;
    case DolphinView::IconsView:
        encodedInfo = (encodedInfo & 0xFF00FF) | (info << 8);
        break;
    case DolphinView::ColumnView:
        encodedInfo = (encodedInfo & 0x00FFFF) | (info << 16);
        break;
    default: break;
    }

    return encodedInfo;
}

int ViewProperties::decodedAdditionalInfo() const
{
    int decodedInfo = m_node->additionalInfo();

    switch (viewMode()) {
    case DolphinView::DetailsView:
        decodedInfo = decodedInfo & 0xFF;
        if (decodedInfo == NoInfo) {
            // a details view without any additional info makes no sense, hence
            // provide at least a size-info and date-info as fallback
            decodedInfo = SizeInfo | DateInfo;
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

    return decodedInfo;
}
