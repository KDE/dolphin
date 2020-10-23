/*
 * SPDX-FileCopyrightText: 2006-2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Aaron J. Seigo <aseigo@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "viewproperties.h"

#include "dolphin_directoryviewpropertysettings.h"
#include "dolphin_generalsettings.h"
#include "dolphindebug.h"

#include <QCryptographicHash>

#include <KFileItem>

namespace {
    const int AdditionalInfoViewPropertiesVersion = 1;
    const int NameRolePropertiesVersion = 2;
    const int DateRolePropertiesVersion = 4;
    const int CurrentViewPropertiesVersion = 4;

    // String representation to mark the additional properties of
    // the details view as customized by the user. See
    // ViewProperties::visibleRoles() for more information.
    const char CustomizedDetailsString[] = "CustomizedDetails";

    // Filename that is used for storing the properties
    const char ViewPropertiesFileName[] = ".directory";
}

ViewProperties::ViewProperties(const QUrl& url) :
    m_changedProps(false),
    m_autoSave(true),
    m_node(nullptr)
{
    GeneralSettings* settings = GeneralSettings::self();
    const bool useGlobalViewProps = settings->globalViewProps() || url.isEmpty();
    bool useDetailsViewWithPath = false;
    bool useRecentDocumentsView = false;
    bool useDownloadsView = false;

    // We try and save it to the file .directory in the directory being viewed.
    // If the directory is not writable by the user or the directory is not local,
    // we store the properties information in a local file.
    if (useGlobalViewProps) {
        m_filePath = destinationDir(QStringLiteral("global"));
    } else if (url.scheme().contains(QLatin1String("search"))) {
        m_filePath = destinationDir(QStringLiteral("search/")) + directoryHashForUrl(url);
        useDetailsViewWithPath = true;
    } else if (url.scheme() == QLatin1String("trash")) {
        m_filePath = destinationDir(QStringLiteral("trash"));
        useDetailsViewWithPath = true;
    } else if (url.scheme() == QLatin1String("recentdocuments")) {
        m_filePath = destinationDir(QStringLiteral("recentdocuments"));
        useRecentDocumentsView = true;
    } else if (url.isLocalFile()) {
        m_filePath = url.toLocalFile();

        bool useDestinationDir = !isPartOfHome(m_filePath);
        if (!useDestinationDir) {
            const KFileItem fileItem(url);
            useDestinationDir = fileItem.isSlow();
        }

        if (!useDestinationDir) {
            const QFileInfo dirInfo(m_filePath);
            const QFileInfo fileInfo(m_filePath + QDir::separator() + ViewPropertiesFileName);
            useDestinationDir = !dirInfo.isWritable() || (dirInfo.size() > 0 && fileInfo.exists() && !(fileInfo.isReadable() && fileInfo.isWritable()));
        }

        if (useDestinationDir) {
    #ifdef Q_OS_WIN
            // m_filePath probably begins with C:/ - the colon is not a valid character for paths though
            m_filePath =  QDir::separator() + m_filePath.remove(QLatin1Char(':'));
    #endif
            m_filePath = destinationDir(QStringLiteral("local")) + m_filePath;
        }

        if (m_filePath == QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)) {
            useDownloadsView = true;
        }
    } else {
        m_filePath = destinationDir(QStringLiteral("remote")) + m_filePath;
    }

    const QString file = m_filePath + QDir::separator() + ViewPropertiesFileName;
    m_node = new ViewPropertySettings(KSharedConfig::openConfig(file));

    // If the .directory file does not exist or the timestamp is too old,
    // use default values instead.
    const bool useDefaultProps = (!useGlobalViewProps || useDetailsViewWithPath) &&
                                 (!QFile::exists(file) ||
                                  (m_node->timestamp() < settings->viewPropsTimestamp()));
    if (useDefaultProps) {
        if (useDetailsViewWithPath) {
            setViewMode(DolphinView::DetailsView);
            setVisibleRoles({"path"});
        } else if (useRecentDocumentsView || useDownloadsView) {
            setSortRole(QByteArrayLiteral("modificationtime"));
            setSortOrder(Qt::DescendingOrder);

            if (useRecentDocumentsView) {
                setViewMode(DolphinView::DetailsView);
                setVisibleRoles({QByteArrayLiteral("path")});
            } else if (useDownloadsView) {
                setSortFoldersFirst(false);
                setGroupedSorting(true);
            }
        } else {
            // The global view-properties act as default for directories without
            // any view-property configuration. Constructing a ViewProperties 
            // instance for an empty QUrl ensures that the global view-properties
            // are loaded.
            QUrl emptyUrl;
            ViewProperties defaultProps(emptyUrl);
            setDirProperties(defaultProps);

            m_changedProps = false;
        }
    }

    if (m_node->version() < CurrentViewPropertiesVersion) {
        // The view-properties have an outdated version. Convert the properties
        // to the changes of the current version.
        if (m_node->version() < AdditionalInfoViewPropertiesVersion) {
            convertAdditionalInfo();
            Q_ASSERT(m_node->version() == AdditionalInfoViewPropertiesVersion);
        }

        if (m_node->version() < NameRolePropertiesVersion) {
            convertNameRoleToTextRole();
            Q_ASSERT(m_node->version() == NameRolePropertiesVersion);
        }

        if (m_node->version() < DateRolePropertiesVersion) {
            convertDateRoleToModificationTimeRole();
            Q_ASSERT(m_node->version() == DateRolePropertiesVersion);
        }

        m_node->setVersion(CurrentViewPropertiesVersion);
    }
}

ViewProperties::~ViewProperties()
{
    if (m_changedProps && m_autoSave) {
        save();
    }

    delete m_node;
    m_node = nullptr;
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
    if (roles == visibleRoles()) {
        return;
    }

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
    newVisibleRoles.reserve(roles.count());
    for (const QByteArray& role : roles) {
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
    // the user explicitly has modified the properties of the details view (marked
    // by "CustomizedDetails"), also a details-view with no additional information
    // is accepted.

    QList<QByteArray> roles{"text"};

    // Iterate through all stored keys and append all roles that match to
    // the current view mode.
    const QString prefix = viewModePrefix();
    const int prefixLength = prefix.length();

    const QStringList visibleRoles = m_node->visibleRoles();
    for (const QString& visibleRole : visibleRoles) {
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
        roles.append("modificationtime");
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
    m_node->setVersion(props.m_node->version());
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
    qCDebug(DolphinDebug) << "Saving view-properties to" << m_filePath;
    QDir dir;
    dir.mkpath(m_filePath);
    m_node->setVersion(CurrentViewPropertiesVersion);
    m_node->save();
    m_changedProps = false;
}

bool ViewProperties::exist() const
{
    const QString file = m_filePath + QDir::separator() + ViewPropertiesFileName;
    return QFile::exists(file);
}

QString ViewProperties::destinationDir(const QString& subDir) const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    path.append("/view_properties/").append(subDir);
    return path;
}

QString ViewProperties::viewModePrefix() const
{
    QString prefix;

    switch (m_node->viewMode()) {
    case DolphinView::IconsView:   prefix = QStringLiteral("Icons_"); break;
    case DolphinView::CompactView: prefix = QStringLiteral("Compact_"); break;
    case DolphinView::DetailsView: prefix = QStringLiteral("Details_"); break;
    default: qCWarning(DolphinDebug) << "Unknown view-mode of the view properties";
    }

    return prefix;
}

void ViewProperties::convertAdditionalInfo()
{
    QStringList visibleRoles;

    const QStringList additionalInfo = m_node->additionalInfo();
    if (!additionalInfo.isEmpty()) {
        // Convert the obsolete values like Icons_Size, Details_Date, ...
        // to Icons_size, Details_date, ... where the suffix just represents
        // the internal role. One special-case must be handled: "LinkDestination"
        // has been used for "destination".
        visibleRoles.reserve(additionalInfo.count());
        for (const QString& info : additionalInfo) {
            QString visibleRole = info;
            int index = visibleRole.indexOf('_');
            if (index >= 0 && index + 1 < visibleRole.length()) {
                ++index;
                if (visibleRole[index] == QLatin1Char('L')) {
                    visibleRole.replace(QLatin1String("LinkDestination"), QLatin1String("destination"));
                } else {
                    visibleRole[index] = visibleRole[index].toLower();
                }
            }
            visibleRoles.append(visibleRole);
        }
    }

    m_node->setAdditionalInfo(QStringList());
    m_node->setVisibleRoles(visibleRoles);
    m_node->setVersion(AdditionalInfoViewPropertiesVersion);
    update();
}

void ViewProperties::convertNameRoleToTextRole()
{
    QStringList visibleRoles = m_node->visibleRoles();
    for (int i = 0; i < visibleRoles.count(); ++i) {
        if (visibleRoles[i].endsWith(QLatin1String("_name"))) {
            const int leftLength = visibleRoles[i].length() - 5;
            visibleRoles[i] = visibleRoles[i].left(leftLength) + "_text";
        }
    }

    QString sortRole = m_node->sortRole();
    if (sortRole == QLatin1String("name")) {
        sortRole = QStringLiteral("text");
    }

    m_node->setVisibleRoles(visibleRoles);
    m_node->setSortRole(sortRole);
    m_node->setVersion(NameRolePropertiesVersion);
    update();
}

void ViewProperties::convertDateRoleToModificationTimeRole()
{
    QStringList visibleRoles = m_node->visibleRoles();
    for (int i = 0; i < visibleRoles.count(); ++i) {
        if (visibleRoles[i].endsWith(QLatin1String("_date"))) {
            const int leftLength = visibleRoles[i].length() - 5;
            visibleRoles[i] = visibleRoles[i].left(leftLength) + "_modificationtime";
        }
    }

    QString sortRole = m_node->sortRole();
    if (sortRole == QLatin1String("date")) {
        sortRole = QStringLiteral("modificationtime");
    }

    m_node->setVisibleRoles(visibleRoles);
    m_node->setSortRole(sortRole);
    m_node->setVersion(DateRolePropertiesVersion);
    update();
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

QString ViewProperties::directoryHashForUrl(const QUrl& url)
{
    const QByteArray hashValue = QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Sha1);
    QString hashString = hashValue.toBase64();
    hashString.replace('/', '-');
    return hashString;
}
