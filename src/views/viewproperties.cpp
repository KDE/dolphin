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
#include <QTemporaryFile>

#include <KFileItem>
#include <KFileMetaData/UserMetaData>

namespace
{
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

ViewPropertySettings *ViewProperties::loadProperties(const QString &folderPath) const
{
    const QString settingsFile = folderPath + QDir::separator() + ViewPropertiesFileName;

    KFileMetaData::UserMetaData metadata(folderPath);
    if (!metadata.isSupported()) {
        return new ViewPropertySettings(KSharedConfig::openConfig(settingsFile, KConfig::SimpleConfig));
    }

    auto createTempFile = []() -> QTemporaryFile * {
        QTemporaryFile *tempFile = new QTemporaryFile;
        tempFile->setAutoRemove(false);
        if (!tempFile->open()) {
            qCWarning(DolphinDebug) << "Could not open temp file";
            return nullptr;
        }
        return tempFile;
    };

    if (QFile::exists(settingsFile)) {
        // copy settings to tempfile to load them separately
        const QTemporaryFile *tempFile = createTempFile();
        if (!tempFile) {
            return nullptr;
        }
        QFile::remove(tempFile->fileName());
        QFile::copy(settingsFile, tempFile->fileName());

        auto config = KConfig(tempFile->fileName(), KConfig::SimpleConfig);
        // ignore settings that are outside of dolphin scope
        if (config.hasGroup("Dolphin") || config.hasGroup("Settings")) {
            const auto groupList = config.groupList();
            for (const auto &group : groupList) {
                if (group != QStringLiteral("Dolphin") && group != QStringLiteral("Settings")) {
                    config.deleteGroup(group);
                }
            }
            return new ViewPropertySettings(KSharedConfig::openConfig(tempFile->fileName(), KConfig::SimpleConfig));

        } else if (!config.groupList().isEmpty()) {
            // clear temp file content
            QFile::remove(tempFile->fileName());
        }
    }

    // load from metadata
    const QString viewPropertiesString = metadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
    if (viewPropertiesString.isEmpty()) {
        return nullptr;
    }
    // load view properties from xattr to temp file then loads into ViewPropertySettings
    // clear the temp file
    const QTemporaryFile *tempFile = createTempFile();
    if (!tempFile) {
        return nullptr;
    }
    QFile outputFile(tempFile->fileName());
    outputFile.open(QIODevice::WriteOnly);
    outputFile.write(viewPropertiesString.toUtf8());
    outputFile.close();
    return new ViewPropertySettings(KSharedConfig::openConfig(tempFile->fileName(), KConfig::SimpleConfig));
}

ViewPropertySettings *ViewProperties::defaultProperties() const
{
    auto props = loadProperties(destinationDir(QStringLiteral("global")));
    if (props == nullptr) {
        qCWarning(DolphinDebug) << "Could not load default global viewproperties";
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        if (!tempFile.open()) {
            qCWarning(DolphinDebug) << "Could not open temp file";
            props = new ViewPropertySettings;
        } else {
            props = new ViewPropertySettings(KSharedConfig::openConfig(tempFile.fileName(), KConfig::SimpleConfig));
        }
    }

    return props;
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
            m_filePath = QDir::separator() + m_filePath.remove(QLatin1Char(':'));
#endif
            m_filePath = destinationDir(QStringLiteral("local")) + m_filePath;
        }

        if (m_filePath == QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)) {
            useDownloadsView = true;
        }
    } else {
        m_filePath = destinationDir(QStringLiteral("remote")) + m_filePath;
    }

    m_node = loadProperties(m_filePath);

    bool useDefaultSettings = useGlobalViewProps ||
        // If the props timestamp is too old,
        // use default values instead.
        (m_node != nullptr && (!useGlobalViewProps || useSearchView || useTrashView || useRecentDocumentsView || useDownloadsView)
         && m_node->timestamp() < settings->viewPropsTimestamp());

    if (m_node == nullptr) {
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

    if (!m_node->config()->name().endsWith(ViewPropertiesFileName)) {
        // remove temp file
        QFile::remove(m_node->config()->name());
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
    setPreviewsShown(props.previewsShown());
    setHiddenFilesShown(props.hiddenFilesShown());
    setGroupedSorting(props.groupedSorting());
    setSortRole(props.sortRole());
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
    if (metaData.isSupported()) {
        const auto metaDataKey = QStringLiteral("kde.fm.viewproperties#1");

        const auto items = m_node->items();
        const bool allDefault = std::all_of(items.cbegin(), items.cend(), [this](const KConfigSkeletonItem *item) {
            return item->name() == "Timestamp" || (item->name() == "Version" && m_node->version() == CurrentViewPropertiesVersion) || item->isDefault();
        });
        if (allDefault) {
            if (metaData.hasAttribute(metaDataKey)) {
                qCDebug(DolphinDebug) << "clearing extended attributes for " << m_filePath;
                const auto result = metaData.setAttribute(metaDataKey, QString());
                if (result != KFileMetaData::UserMetaData::NoError) {
                    qCWarning(DolphinDebug) << "could not clear extended attributes for " << m_filePath << "error:" << result;
                }
            }
            cleanDotDirectoryFile();
            return;
        }

        // save config to disk
        if (!m_node->save()) {
            qCWarning(DolphinDebug) << "could not save viewproperties" << m_node->config()->name();
            return;
        }

        QFile configFile(m_node->config()->name());
        if (!configFile.open(QIODevice::ReadOnly)) {
            qCWarning(DolphinDebug) << "Could not open readonly config file" << m_node->config()->name();
        } else {
            // load config from disk
            const QString viewPropertiesString = configFile.readAll();

            // save to xattr
            const auto result = metaData.setAttribute(metaDataKey, viewPropertiesString);
            if (result != KFileMetaData::UserMetaData::NoError) {
                if (result == KFileMetaData::UserMetaData::NoSpace) {
                    // copy settings to dotDirectory file as fallback
                    if (!configFile.copy(m_filePath + QDir::separator() + ViewPropertiesFileName)) {
                        qCWarning(DolphinDebug) << "could not write viewproperties to .directory for dir " << m_filePath;
                    }
                    // free the space used by viewproperties from the file metadata
                    metaData.setAttribute(metaDataKey, "");
                } else {
                    qCWarning(DolphinDebug) << "could not save viewproperties to extended attributes for dir " << m_filePath << "error:" << result;
                }
                // keep .directory file
                return;
            }
            cleanDotDirectoryFile();
        }

        m_changedProps = false;
        return;
    }

    QDir dir;
    dir.mkpath(m_filePath);
    m_node->setVersion(CurrentViewPropertiesVersion);
    m_node->save();

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
