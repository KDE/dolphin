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

#include <QBuffer>
#include <QCryptographicHash>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <KFileItem>
#include <KFileMetaData/UserMetaData>

namespace
{
const int AdditionalInfoViewPropertiesVersion = 1;
const int NameRolePropertiesVersion = 2;
const int DateRolePropertiesVersion = 4;
const int CurrentViewPropertiesVersion = 4;

const QString MetaDataKey = QStringLiteral("kde.fm.viewproperties#1");

// String representation to mark the additional properties of
// the details view as customized by the user. See
// ViewProperties::visibleRoles() for more information.
const char CustomizedDetailsString[] = "CustomizedDetails";

// Filename that is used for storing the properties
const char ViewPropertiesFileName[] = ".directory";
}

ViewPropertySettings *ViewProperties::loadProperties(const QString &folderPath) const
{
    const QString settingsFile = folderPath + QDir::separator() + ViewPropertiesFileName;

    std::shared_ptr<QFile> file = std::make_shared<QFile>(settingsFile);
    if (file->exists()) {
        if (!file->open(QIODevice::ReadOnly)) {
            qCWarning(DolphinDebug) << "Could not open file" << file->fileName();
            return nullptr;
        }
    }

    KFileMetaData::UserMetaData metadata(folderPath);
    if (!metadata.isSupported()) {
        auto fileConfig = std::make_unique<KConfig>(file, KConfig::OpenFlag::SimpleConfig);
        return new ViewPropertySettings(std::move(fileConfig));
    }

    auto buffer = std::make_shared<QBuffer>();

    if (file->exists()) {
        // load settings into new ViewPropertySettings
        KConfig config(file, KConfig::OpenFlag::SimpleConfig);
        // keep settings that are outside of dolphin scope
        if (config.hasGroup("Dolphin") || config.hasGroup("Settings")) {
            const auto groupList = config.groupList();
            for (const auto &group : groupList) {
                if (group != QStringLiteral("Dolphin") && group != QStringLiteral("Settings")) {
                    config.deleteGroup(group);
                }
            }

            // copy the filtered config to a new ViewPropertySettings
            if (!buffer->open(QIODevice::ReadWrite)) {
                qCWarning(DolphinDebug) << "Could not open buffer";
            }

            auto bufferConfig = std::make_unique<KConfig>(buffer, KConfig::OpenFlag::SimpleConfig);
            bufferConfig->copyFrom(config);
            return new ViewPropertySettings(std::move(bufferConfig));
        }
    }

    // load from metadata
    const QString viewPropertiesString = metadata.attribute(MetaDataKey);
    if (viewPropertiesString.isEmpty()) {
        return nullptr;
    }

    buffer->setData(viewPropertiesString.toUtf8());
    // must have the iodevice opened for KConfig
    if (!buffer->open(QIODevice::ReadWrite)) {
        qCWarning(DolphinDebug) << "Could not open buffer";
    }

    auto bufferConfig = std::make_unique<KConfig>(buffer, KConfig::OpenFlag::SimpleConfig);

    return new ViewPropertySettings(std::move(bufferConfig));
}

ViewPropertySettings *ViewProperties::defaultProperties() const
{
    auto propsOpt = loadProperties(destinationDir(QStringLiteral("global")));
    ViewPropertySettings *props;
    if (propsOpt) {
        props = propsOpt;
    } else {
        qCDebug(DolphinDebug) << "Could not load default global viewproperties";

        auto buffer = std::make_shared<QBuffer>();
        if (!buffer->open(QIODevice::ReadWrite)) {
            qCWarning(DolphinDebug) << "Could not create buffer";
            return nullptr;
        }
        auto bufferConfig = std::make_unique<KConfig>(buffer, KConfig::OpenFlag::SimpleConfig);
        props = new ViewPropertySettings(std::move(bufferConfig));
    }

    return props;
}

void ViewProperties::restoreToDefaults()
{
    delete m_node;
    m_node = defaultProperties();
    update();
}

bool ViewProperties::isDefaults() const
{
    const auto defaultProps = defaultProperties();
    auto cleanup = qScopeGuard([&defaultProps]() {
        delete defaultProps;
    });
    // clang-format off
    return m_node->viewMode() == defaultProps->viewMode()
        && m_node->zoomLevel() == defaultProps->zoomLevel()
        && m_node->previewsShown() == defaultProps->previewsShown()
        && m_node->hiddenFilesShown() == defaultProps->hiddenFilesShown()
        && m_node->groupedSorting() == defaultProps->groupedSorting()
        && m_node->groupRole() == defaultProps->groupRole()
        && m_node->sortRole() == defaultProps->sortRole()
        && m_node->sortOrder() == defaultProps->sortOrder()
        && m_node->sortFoldersFirst() == defaultProps->sortFoldersFirst()
        && m_node->sortHiddenLast() == defaultProps->sortHiddenLast()
        && m_node->visibleRoles() == defaultProps->visibleRoles()
        && m_node->headerColumnWidths() == defaultProps->headerColumnWidths();
    // clang-format on
}

ViewProperties::ViewProperties(const QUrl &url)
    : m_changedProps(false)
    , m_autoSave(true)
    , m_node(nullptr)
{
    GeneralSettings *settings = GeneralSettings::self();
    const bool useGlobalViewProps = settings->globalViewProps() || url.isEmpty();
    bool useSearchView = false;
    bool useTrashView = false;
    bool useRecentDocumentsView = false;
    bool useDownloadsView = false;

    // We try and save it to the file .directory in the directory being viewed.
    // If the directory is not writable by the user or the directory is not local,
    // we store the properties information in a local file.
    if (url.scheme().contains(QLatin1String("search"))) {
        m_filePath = destinationDir(QStringLiteral("search/")) + directoryHashForUrl(url);
        useSearchView = true;
    } else if (url.scheme() == QLatin1String("trash")) {
        m_filePath = destinationDir(QStringLiteral("trash"));
        useTrashView = true;
    } else if (url.scheme() == QLatin1String("recentlyused")) {
        m_filePath = destinationDir(QStringLiteral("recentlyused"));
        useRecentDocumentsView = true;
    } else if (url.scheme() == QLatin1String("timeline")) {
        m_filePath = destinationDir(QStringLiteral("timeline"));
        useRecentDocumentsView = true;
    } else if (useGlobalViewProps) {
        m_filePath = destinationDir(QStringLiteral("global"));
    } else if (url.isLocalFile()) {
        m_filePath = url.toLocalFile();
        const QString localPath = m_filePath;

        // Resolve symlinks (bug 477662).
        const QFileInfo dirInfo(m_filePath);
        if (const QString canonicalPath = dirInfo.canonicalFilePath(); !canonicalPath.isEmpty()) {
            m_filePath = canonicalPath;
        }
        const QUrl canonicalUrl = QUrl::fromLocalFile(m_filePath);

        bool useDestinationDir = !isPartOfHome(m_filePath);
        if (!useDestinationDir) {
            const KFileItem fileItem(canonicalUrl);
            useDestinationDir = fileItem.isSlow();
        }

        if (!useDestinationDir) {
            const QFileInfo fileInfo(m_filePath + QDir::separator() + ViewPropertiesFileName);
            bool dirWritable = dirInfo.isWritable();
#ifdef Q_OS_WIN
            if (dirWritable) {
                const DWORD attrs = GetFileAttributesW(reinterpret_cast<const wchar_t *>(m_filePath.utf16()));
                dirWritable = (attrs == INVALID_FILE_ATTRIBUTES) || !(attrs & FILE_ATTRIBUTE_READONLY);
            }
#endif
            useDestinationDir = !dirWritable || (dirInfo.size() > 0 && fileInfo.exists() && !(fileInfo.isReadable() && fileInfo.isWritable()));
        }

        if (useDestinationDir) {
            m_filePath = destinationDir(QStringLiteral("local/")) + directoryHashForUrl(canonicalUrl);

            // Migration: properties for such folders used to be stored under the
            // full local path, which could exceed path-length limits and exposed
            // the path in the storage location. Move an existing entry to the
            // hashed location once.
            // TODO: remove this block after 2028-06, once Debian 14 (Forky)
            // has shipped and users have had time to migrate.
            if (!QFileInfo::exists(m_filePath)) {
#ifdef Q_OS_WIN
                // The old path had the drive colon stripped to keep it valid.
                const QString oldPath = destinationDir(QStringLiteral("local")) + QDir::separator() + QString(localPath).remove(QLatin1Char(':'));
#else
                const QString oldPath = destinationDir(QStringLiteral("local")) + localPath;
#endif
                if (QFileInfo::exists(oldPath)) {
                    QDir().rename(oldPath, m_filePath);
                }
            }
        }

        if (localPath == QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)) {
            useDownloadsView = true;
        }
    } else {
        m_filePath = destinationDir(QStringLiteral("remote/")) + directoryHashForUrl(url);
    }

    auto propsOpt = loadProperties(m_filePath);

    bool useDefaultSettings =
        // If the props timestamp is too old,
        // use default values instead.
        (propsOpt && (!useGlobalViewProps || useSearchView || useTrashView || useRecentDocumentsView || useDownloadsView)
         && propsOpt->timestamp() < settings->viewPropsTimestamp())
        // When global view props is on and this is a special folder (search, trash,
        // recents/timeline, downloads), only apply defaults on the first visit.
        // On subsequent visits the user's saved properties should be preserved.
        || (useGlobalViewProps && !(useSearchView || useTrashView || useRecentDocumentsView || useDownloadsView));

    if (propsOpt) {
        m_node = propsOpt;
    } else {
        // no settings found for m_filepath, load defaults
        m_node = defaultProperties();
        useDefaultSettings = true;
    }

    // default values for special directories
    if (useDefaultSettings) {
        if (useSearchView) {
            const QString path = url.path();

            if (path == QLatin1String("/images")) {
                setViewMode(DolphinView::IconsView);
                setPreviewsShown(true);
                setVisibleRoles({"text", "dimensions", "imageDateTime"});
            } else if (path == QLatin1String("/audio")) {
                setViewMode(DolphinView::DetailsView);
                setVisibleRoles({"text", "artist", "album", "duration"});
            } else if (path == QLatin1String("/videos")) {
                setViewMode(DolphinView::IconsView);
                setPreviewsShown(true);
                setVisibleRoles({"text"});
            } else {
                setViewMode(DolphinView::DetailsView);
                setVisibleRoles({"text", "path", "modificationtime"});
            }
        } else if (useTrashView) {
            setViewMode(DolphinView::DetailsView);
            setVisibleRoles({"text", "path", "deletiontime"});
        } else if (useRecentDocumentsView || useDownloadsView) {
            setSortOrder(Qt::DescendingOrder);
            setSortFoldersFirst(false);
            setGroupedSorting(true);

            if (useRecentDocumentsView) {
                setSortRole(QByteArrayLiteral("accesstime"));
                setViewMode(DolphinView::DetailsView);
                setVisibleRoles({"text", "path", "accesstime"});
            } else {
                setSortRole(QByteArrayLiteral("modificationtime"));
            }
        } else {
            m_changedProps = false;
        }
        setZoomLevel(-1);
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

    auto name = m_node->config()->name();
    if (!name.isEmpty() && !name.endsWith(ViewPropertiesFileName)) {
        // remove backing file
        QFile::remove(m_node->config()->name());
    }

    delete m_node;
    m_node = nullptr;
}

void ViewProperties::setZoomLevel(int zoomLevel)
{
    if (m_node->zoomLevel() != zoomLevel) {
        m_node->setZoomLevel(zoomLevel);
        update();
    }
}

int ViewProperties::zoomLevel() const
{
    return m_node->zoomLevel();
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
    const int mode = qBound(0, m_node->viewMode(), 3);
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

void ViewProperties::setSortRole(const QByteArray &role)
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

void ViewProperties::setGroupRole(const QByteArray &role)
{
    if (m_node->groupRole() != role) {
        m_node->setGroupRole(role);
        update();
    }
}

QByteArray ViewProperties::groupRole() const
{
    return m_node->groupRole().toLatin1();
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

void ViewProperties::setSortHiddenLast(bool hiddenLast)
{
    if (m_node->sortHiddenLast() != hiddenLast) {
        m_node->setSortHiddenLast(hiddenLast);
        update();
    }
}

bool ViewProperties::sortHiddenLast() const
{
    return m_node->sortHiddenLast();
}

void ViewProperties::setDynamicViewPassed(bool dynamicViewPassed)
{
    if (m_node->dynamicViewPassed() != dynamicViewPassed) {
        m_node->setDynamicViewPassed(dynamicViewPassed);
        update();
    }
}

bool ViewProperties::dynamicViewPassed() const
{
    return m_node->dynamicViewPassed();
}

void ViewProperties::setVisibleRoles(const QList<QByteArray> &roles)
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
    for (const QByteArray &role : roles) {
        newVisibleRoles.append(prefix + role);
    }

    if (oldVisibleRoles != newVisibleRoles) {
        const bool markCustomizedDetails = (m_node->viewMode() == DolphinView::DetailsView) && !newVisibleRoles.contains(CustomizedDetailsString);
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
    for (const QString &visibleRole : visibleRoles) {
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
        && (m_node->viewMode() == DolphinView::DetailsView) && !visibleRoles.contains(CustomizedDetailsString);
    if (useDefaultValues) {
        roles.append("size");
        roles.append("modificationtime");
    }

    return roles;
}

void ViewProperties::setHeaderColumnWidths(const QList<int> &widths)
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

void ViewProperties::setDirProperties(const ViewProperties &props)
{
    setViewMode(props.viewMode());
    setZoomLevel(props.zoomLevel());
    setPreviewsShown(props.previewsShown());
    setHiddenFilesShown(props.hiddenFilesShown());
    setGroupedSorting(props.groupedSorting());
    setSortRole(props.sortRole());
    setGroupRole(props.groupRole());
    setSortOrder(props.sortOrder());
    setSortFoldersFirst(props.sortFoldersFirst());
    setSortHiddenLast(props.sortHiddenLast());
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

    auto cleanDotDirectoryFile = [this]() {
        const QString settingsFile = m_filePath + QDir::separator() + ViewPropertiesFileName;
        if (QFile::exists(settingsFile)) {
            qCDebug(DolphinDebug) << "cleaning .directory" << settingsFile;
            KConfig cfg(settingsFile, KConfig::OpenFlag::SimpleConfig);
            const auto groupList = cfg.groupList();
            for (const auto &group : groupList) {
                if (group == QStringLiteral("Dolphin") || group == QStringLiteral("Settings")) {
                    cfg.deleteGroup(group);
                }
            }
            if (cfg.groupList().isEmpty()) {
                QFile::remove(settingsFile);
            } else if (cfg.isDirty()) {
                cfg.sync();
            }
        }
    };

    // ensures the destination dir exists, in case we don't write metadata directly on the folder
    QDir destinationDir(m_filePath);
    if (!destinationDir.exists() && !destinationDir.mkpath(m_filePath)) {
        qCWarning(DolphinDebug) << "Could not create fake directory to store metadata";
    }

    KFileMetaData::UserMetaData metaData(m_filePath);
    if (!metaData.isSupported()) {
        // save to dotDirectory file as fallback
        QDir dir;
        dir.mkpath(m_filePath);
        m_node->setVersion(CurrentViewPropertiesVersion);
        m_node->save();

        m_changedProps = false;
        return;
    }
    const auto items = m_node->items();
    // default settings must not be cleared
    bool allDefault = ViewProperties::destinationDir(QStringLiteral("global")) != m_filePath;
    const auto defaultConfig = defaultProperties();
    for (const auto item : items) {
        if (item->name() == "Timestamp") {
            continue;
        }
        if (item->name() == "Version") {
            if (m_node->version() != CurrentViewPropertiesVersion) {
                allDefault = false;
                break;
            } else {
                continue;
            }
        }
        auto defaultItem = defaultConfig->findItem(item->name());
        if (!defaultItem || (defaultItem->property() != item->property())) {
            allDefault = false;
            break;
        }
    }
    delete defaultConfig;

    if (allDefault) {
        if (metaData.hasAttribute(MetaDataKey)) {
            qCDebug(DolphinDebug) << "clearing extended attributes for " << m_filePath;
            const auto result = metaData.setAttribute(MetaDataKey, QString());
            if (result != KFileMetaData::UserMetaData::NoError) {
                qCWarning(DolphinDebug) << "could not clear extended attributes for " << m_filePath << "error:" << result;
            }
        }
        cleanDotDirectoryFile();
        return;
    }

    auto buffer = std::make_shared<QBuffer>();
    if (!buffer->open(QIODevice::ReadWrite)) {
        qCWarning(DolphinDebug) << "Could not create buffer";
    }
    auto config = std::make_unique<KConfig>(buffer, KConfig::OpenFlag::SimpleConfig);
    // until KConfig / KCoreSkeletonConfig can expose its internal QIODevice we have to copy the config
    config->copyFrom(*m_node->config());

    // if we are using a different filePath as m_filePath
    // it means we are using fallback code path, the dir is a special case
    if (!m_node->config()->name().startsWith(m_filePath)) {
        m_node->setConfig(std::move(config));

        // save config to disk
        if (!m_node->save()) {
            qCWarning(DolphinDebug) << "could not save viewproperties" << m_node->config()->name();
            return;
        }
    }

    // load config from buffer
    buffer->seek(0);
    const QString viewPropertiesString = buffer->readAll();

    // save to xattr
    const auto result = metaData.setAttribute(MetaDataKey, viewPropertiesString);
    if (result != KFileMetaData::UserMetaData::NoError) {
        if (result == KFileMetaData::UserMetaData::NoSpace) {
            // free the space used by viewproperties from the file metadata
            metaData.setAttribute(MetaDataKey, QString());
            qCWarning(DolphinDebug) << "could not save viewproperties to extended attributes for dir " << m_filePath << ", no space available in attributes";
            // xattr space exhausted — fall back to .directory file.
            // Copy only the Dolphin-managed groups into the existing file using
            // KConfigGroup::copyTo so that unrelated groups (e.g. [Desktop Entry]
            // with a custom folder icon) are left untouched by sync().
            const QString settingsFile = m_filePath + QDir::separator() + ViewPropertiesFileName;
            KConfig fileConfig(settingsFile, KConfig::SimpleConfig);
            for (const QString &group : m_node->config()->groupList()) {
                KConfigGroup srcGrp(m_node->config(), group);
                KConfigGroup dstGrp(&fileConfig, group);
                srcGrp.copyTo(&dstGrp);
            }
            if (!fileConfig.sync()) {
                qCWarning(DolphinDebug) << "could not save viewproperties to .directory for" << m_filePath;
            }
        } else {
            qCWarning(DolphinDebug) << "could not save viewproperties to extended attributes for dir " << m_filePath << "error:" << result;
        }
        m_changedProps = false;
        return;
    }
    cleanDotDirectoryFile();

    m_changedProps = false;
}

QString ViewProperties::destinationDir(const QString &subDir) const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    path.append("/view_properties/").append(subDir);
    return path;
}

QString ViewProperties::viewModePrefix() const
{
    QString prefix;

    switch (m_node->viewMode()) {
    case DolphinView::IconsView:
        prefix = QStringLiteral("Icons_");
        break;
    case DolphinView::CompactView:
        prefix = QStringLiteral("Compact_");
        break;
    case DolphinView::DetailsView:
        prefix = QStringLiteral("Details_");
        break;
    case DolphinView::ColumnsView:
        prefix = QStringLiteral("Columns_");
        break;
    default:
        qCWarning(DolphinDebug) << "Unknown view-mode of the view properties";
    }

    return prefix;
}

void ViewProperties::convertAdditionalInfo()
{
    QStringList visibleRoles = m_node->visibleRoles();

    const QStringList additionalInfo = m_node->additionalInfo();
    if (!additionalInfo.isEmpty()) {
        // Convert the obsolete values like Icons_Size, Details_Date, ...
        // to Icons_size, Details_date, ... where the suffix just represents
        // the internal role. One special-case must be handled: "LinkDestination"
        // has been used for "destination".
        visibleRoles.reserve(visibleRoles.count() + additionalInfo.count());
        for (const QString &info : additionalInfo) {
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
            if (!visibleRoles.contains(visibleRole)) {
                visibleRoles.append(visibleRole);
            }
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

bool ViewProperties::isPartOfHome(const QString &filePath)
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

QString ViewProperties::directoryHashForUrl(const QUrl &url)
{
    const QByteArray hashValue = QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Sha1);
    QString hashString = hashValue.toBase64();
    hashString.replace('/', '-');
    return hashString;
}
