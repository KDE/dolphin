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

#include "dolphin_directoryviewpropertysettings.h"
#include "dolphin_generalsettings.h"

#include <KComponentData>
#include <KLocale>
#include <KStandardDirs>
#include <KUrl>

#include <QCryptographicHash>
#include <QDate>
#include <QFile>
#include <QFileInfo>

namespace {
    const int AdditionalInfoViewPropertiesVersion = 1;
    const int NameRolePropertiesVersion = 2;
    const int CurrentViewPropertiesVersion = 3;

    // String representation to mark the additional properties of
    // the details view as customized by the user. See
    // ViewProperties::visibleRoles() for more information.
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
        m_filePath = destinationDir("search/") + directoryHashForUrl(url);
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
            setVisibleRoles(QList<QByteArray>() << "path");
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

void ViewProperties::setSortRole(const QByteArray& role)
{
    if (m_node->sortRole() != role) {
        m_node->setSortRole(role);
        update();
    }
}

QByteArray ViewProperties::sortRole() const
{
    return m_node->sortRole().toLatin1();
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

void ViewProperties::setVisibleRoles(const QList<QByteArray>& roles)
{
    // See ViewProperties::visibleRoles() for the storage format
    // of the additional information.

    // Remove the old values stored for the current view-mode
    const QStringList oldVisibleRoles = m_node->visibleRoles();
    const QString prefix = viewModePrefix();
    QStringList newVisibleRoles = oldVisibleRoles;
    for (int i = newVisibleRoles.count() - 1; i >= 0; --i) {
        if (newVisibleRoles[i].startsWith(prefix)) {
            newVisibleRoles.removeAt(i);
        }
    }

    // Add the updated values for the current view-mode
    foreach (const QByteArray& role, roles) {
        newVisibleRoles.append(prefix + role);
    }

    if (oldVisibleRoles != newVisibleRoles) {
        const bool markCustomizedDetails = (m_node->viewMode() == DolphinView::DetailsView)
                                           && !newVisibleRoles.contains(CustomizedDetailsString);
        if (markCustomizedDetails) {
            // The additional information of the details-view has been modified. Set a marker,
            // so that it is allowed to also show no additional information without doing the
            // fallback to show the size and date per default.
            newVisibleRoles.append(CustomizedDetailsString);
        }

        m_node->setVisibleRoles(newVisibleRoles);
        update();
    }
}

QList<QByteArray> ViewProperties::visibleRoles() const
{
    // The shown additional information is stored for each view-mode separately as
    // string with the view-mode as prefix. Example:
    //
    // AdditionalInfo=Details_size,Details_date,Details_owner,Icons_size
    //
    // To get the representation as QList<QByteArray>, the current
    // view-mode must be checked and the values of this mode added to the list.
    //
    // For the details-view a special case must be respected: Per default the size
    // and date should be shown without creating a .directory file. Only if
    // the user explictly has modified the properties of the details view (marked
    // by "CustomizedDetails"), also a details-view with no additional information
    // is accepted.

    QList<QByteArray> roles;
    roles.append("text");

    // Iterate through all stored keys and append all roles that match to
    // the curren view mode.
    const QString prefix = viewModePrefix();
    const int prefixLength = prefix.length();

    QStringList visibleRoles = m_node->visibleRoles();
    const int version = m_node->version();
    if (visibleRoles.isEmpty() && version <= AdditionalInfoViewPropertiesVersion) {
        // Convert the obsolete additionalInfo-property from older versions into the
        // visibleRoles-property
        visibleRoles = const_cast<ViewProperties*>(this)->convertAdditionalInfo();
    } else if (version <= NameRolePropertiesVersion) {
        visibleRoles = const_cast<ViewProperties*>(this)->convertNameRole();
    }

    foreach (const QString& visibleRole, visibleRoles) {
        if (visibleRole.startsWith(prefix)) {
            const QByteArray role = visibleRole.right(visibleRole.length() - prefixLength).toLatin1();
            if (role != "text") {
                roles.append(role);
            }
        }
    }

    // For the details view the size and date should be shown per default
    // until the additional information has been explicitly changed by the user
    const bool useDefaultValues = roles.count() == 1 // "text"
                                  && (m_node->viewMode() == DolphinView::DetailsView)
                                  && !visibleRoles.contains(CustomizedDetailsString);
    if (useDefaultValues) {
        roles.append("size");
        roles.append("date");
    }

    return roles;
}

void ViewProperties::setHeaderColumnWidths(const QList<int>& widths)
{
    if (m_node->headerColumnWidths() != widths) {
        m_node->setHeaderColumnWidths(widths);
        update();
    }
}

QList<int> ViewProperties::headerColumnWidths() const
{
    return m_node->headerColumnWidths();
}

void ViewProperties::setDirProperties(const ViewProperties& props)
{
    setViewMode(props.viewMode());
    setPreviewsShown(props.previewsShown());
    setHiddenFilesShown(props.hiddenFilesShown());
    setGroupedSorting(props.groupedSorting());
    setSortRole(props.sortRole());
    setSortOrder(props.sortOrder());
    setSortFoldersFirst(props.sortFoldersFirst());
    setVisibleRoles(props.visibleRoles());
    setHeaderColumnWidths(props.headerColumnWidths());
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
    m_node->setVersion(CurrentViewPropertiesVersion);
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

QStringList ViewProperties::convertAdditionalInfo()
{
    QStringList visibleRoles;

    const QStringList additionalInfo = m_node->additionalInfo();
    if (!additionalInfo.isEmpty()) {
        // Convert the obsolete values like Icons_Size, Details_Date, ...
        // to Icons_size, Details_date, ... where the suffix just represents
        // the internal role. One special-case must be handled: "LinkDestination"
        // has been used for "destination".
        visibleRoles.reserve(additionalInfo.count());
        foreach (const QString& info, additionalInfo) {
            QString visibleRole = info;
            int index = visibleRole.indexOf('_');
            if (index >= 0 && index + 1 < visibleRole.length()) {
                ++index;
                if (visibleRole[index] == QLatin1Char('L')) {
                    visibleRole.replace("LinkDestination", "destination");
                } else {
                    visibleRole[index] = visibleRole[index].toLower();
                }
            }
            visibleRoles.append(visibleRole);
        }
    }

    m_node->setAdditionalInfo(QStringList());
    m_node->setVisibleRoles(visibleRoles);
    update();

    return visibleRoles;
}

QStringList ViewProperties::convertNameRole()
{
    QStringList visibleRoles = m_node->visibleRoles();
    for (int i = 0; i < visibleRoles.count(); ++i) {
        if (visibleRoles[i].endsWith("_name")) {
            const int leftLength = visibleRoles[i].length() - 5;
            visibleRoles[i] = visibleRoles[i].left(leftLength) + "_text";
        }
    }

    m_node->setVisibleRoles(visibleRoles);
    update();

    return visibleRoles;
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

QString ViewProperties::directoryHashForUrl(const KUrl& url)
{
    const QByteArray hashValue = QCryptographicHash::hash(url.prettyUrl().toLatin1(),
                                                     QCryptographicHash::Sha1);
    QString hashString = hashValue.toBase64();
    hashString.replace('/', '-');
    return hashString;
}
