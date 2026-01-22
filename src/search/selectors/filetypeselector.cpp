/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "filetypeselector.h"

#include "../dolphinquery.h"

#include <KFileMetaData/TypeInfo>
#include <KLocalizedString>

using namespace Search;

FileTypeSelector::FileTypeSelector(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent)
    : QComboBox{parent}
    , UpdatableStateInterface{dolphinQuery}
{
    for (KFileMetaData::Type::Type type = KFileMetaData::Type::FirstType; type <= KFileMetaData::Type::LastType; type = KFileMetaData::Type::Type(type + 1)) {
        switch (type) {
        case KFileMetaData::Type::Empty:
            addItem(/** No icon for the empty state */ i18nc("@item:inlistbox", "Any Type"), type);
            continue;
        case KFileMetaData::Type::Archive:
            addItem(QIcon::fromTheme(QStringLiteral("package-x-generic")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Audio:
            addItem(QIcon::fromTheme(QStringLiteral("audio-x-generic")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Video:
            addItem(QIcon::fromTheme(QStringLiteral("video-x-generic")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Image:
            addItem(QIcon::fromTheme(QStringLiteral("image-x-generic")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Document:
            addItem(QIcon::fromTheme(QStringLiteral("text-x-generic")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Spreadsheet:
            addItem(QIcon::fromTheme(QStringLiteral("table")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Presentation:
            addItem(QIcon::fromTheme(QStringLiteral("view-presentation")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Text:
            addItem(QIcon::fromTheme(QStringLiteral("text-x-generic")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        case KFileMetaData::Type::Folder:
            addItem(QIcon::fromTheme(QStringLiteral("inode-directory")), KFileMetaData::TypeInfo{type}.displayName(), type);
            continue;
        default:
            addItem(QIcon(), KFileMetaData::TypeInfo{type}.displayName(), type);
        }
    }

    connect(this, &QComboBox::activated, this, [this](int activatedIndex) {
        auto activatedType = itemData(activatedIndex).value<KFileMetaData::Type::Type>();
        if (activatedType == m_searchConfiguration->fileType()) {
            return; // Already selected.
        }
        DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
        searchConfigurationCopy.setFileType(activatedType);
        Q_EMIT configurationChanged(searchConfigurationCopy);
    });

    updateStateToMatch(std::move(dolphinQuery));
}

void Search::FileTypeSelector::removeRestriction()
{
    Q_ASSERT(m_searchConfiguration->fileType() != KFileMetaData::Type::Empty);
    DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
    searchConfigurationCopy.setFileType(KFileMetaData::Type::Empty);
    Q_EMIT configurationChanged(searchConfigurationCopy);
}

void FileTypeSelector::updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery)
{
    setCurrentIndex(findData(dolphinQuery->fileType()));
}
