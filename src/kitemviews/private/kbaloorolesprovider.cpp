/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Vishesh Handa <me@vhanda.in>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kbaloorolesprovider.h"

#include <Baloo/File>
#include <KFileMetaData/PropertyInfo>
#include <KFileMetaData/UserMetaData>

#include <QCollator>
#include <QSize>

namespace {
    QString tagsFromValues(const QStringList& values)
    {
        if (values.size() == 1) {
            return values.at(0);
        }

        QStringList alphabeticalOrderTags = values;
        QCollator coll;
        coll.setNumericMode(true);
        std::sort(alphabeticalOrderTags.begin(), alphabeticalOrderTags.end(), [&](const QString& s1, const QString& s2){ return coll.compare(s1, s2) < 0; });
        return alphabeticalOrderTags.join(QLatin1String(", "));
    }

    using Property = KFileMetaData::Property::Property;
    // Mapping from the KFM::Property to the KFileItemModel roles.
    const QHash<Property, QByteArray> propertyRoleMap() {
        static const auto map = QHash<Property, QByteArray> {
            { Property::Rating,            QByteArrayLiteral("rating") },
            { Property::Comment,           QByteArrayLiteral("comment") },
            { Property::Title,             QByteArrayLiteral("title") },
            { Property::Author,            QByteArrayLiteral("author") },
            { Property::Publisher,         QByteArrayLiteral("publisher") },
            { Property::PageCount,         QByteArrayLiteral("pageCount") },
            { Property::WordCount,         QByteArrayLiteral("wordCount") },
            { Property::LineCount,         QByteArrayLiteral("lineCount") },
            { Property::Width,             QByteArrayLiteral("width") },
            { Property::Height,            QByteArrayLiteral("height") },
            { Property::ImageDateTime,     QByteArrayLiteral("imageDateTime") },
            { Property::ImageOrientation,  QByteArrayLiteral("orientation") },
            { Property::Artist,            QByteArrayLiteral("artist") },
            { Property::Genre,             QByteArrayLiteral("genre")  },
            { Property::Album,             QByteArrayLiteral("album") },
            { Property::Duration,          QByteArrayLiteral("duration") },
            { Property::BitRate,           QByteArrayLiteral("bitrate") },
            { Property::AspectRatio,       QByteArrayLiteral("aspectRatio") },
            { Property::FrameRate,         QByteArrayLiteral("frameRate") },
            { Property::ReleaseYear,       QByteArrayLiteral("releaseYear") },
            { Property::TrackNumber,       QByteArrayLiteral("track") }
        };
        return map;
    }
}

struct KBalooRolesProviderSingleton
{
    KBalooRolesProvider instance;
};
Q_GLOBAL_STATIC(KBalooRolesProviderSingleton, s_balooRolesProvider)


KBalooRolesProvider& KBalooRolesProvider::instance()
{
    return s_balooRolesProvider->instance;
}

KBalooRolesProvider::~KBalooRolesProvider()
{
}

QSet<QByteArray> KBalooRolesProvider::roles() const
{
    return m_roles;
}

QHash<QByteArray, QVariant> KBalooRolesProvider::roleValues(const Baloo::File& file,
                                                            const QSet<QByteArray>& roles) const
{
    QHash<QByteArray, QVariant> values;

    using entry = std::pair<const KFileMetaData::Property::Property&, const QVariant&>;

    const auto& propMap = file.properties();
    auto rangeBegin = propMap.constKeyValueBegin();

    while (rangeBegin != propMap.constKeyValueEnd()) {
        auto key = (*rangeBegin).first;

        auto rangeEnd = std::find_if(rangeBegin, propMap.constKeyValueEnd(),
            [key](const entry& e) { return e.first != key; });

        const QByteArray role = propertyRoleMap().value(key);
        if (role.isEmpty() || !roles.contains(role)) {
            rangeBegin = rangeEnd;
            continue;
        }

        const KFileMetaData::PropertyInfo propertyInfo(key);
        auto distance = std::distance(rangeBegin, rangeEnd);
        if (distance > 1) {
            QVariantList list;
            list.reserve(static_cast<int>(distance));
            std::for_each(rangeBegin, rangeEnd, [&list](const entry& s) { list.append(s.second); });
            values.insert(role, propertyInfo.formatAsDisplayString(list));
        } else {
            if (propertyInfo.valueType() == QVariant::DateTime) {
                // Let dolphin format later Dates
                values.insert(role, (*rangeBegin).second);
            } else {
                values.insert(role, propertyInfo.formatAsDisplayString((*rangeBegin).second));
            }
        }
        rangeBegin = rangeEnd;
    }

    if (roles.contains("dimensions")) {
        bool widthOk = false;
        bool heightOk = false;

        const int width = propMap.value(KFileMetaData::Property::Width).toInt(&widthOk);
        const int height = propMap.value(KFileMetaData::Property::Height).toInt(&heightOk);

        if (widthOk && heightOk && width >= 0 && height >= 0) {
            values.insert("dimensions", QSize(width, height));
        }
    }

    KFileMetaData::UserMetaData::Attributes attributes;
    if (roles.contains("tags")) {
        attributes |= KFileMetaData::UserMetaData::Tags;
    }
    if (roles.contains("rating")) {
        attributes |= KFileMetaData::UserMetaData::Rating;
    }
    if (roles.contains("comment")) {
        attributes |= KFileMetaData::UserMetaData::Comment;
    }
    if (roles.contains("originUrl")) {
        attributes |= KFileMetaData::UserMetaData::OriginUrl;
    }

    if (attributes == KFileMetaData::UserMetaData::None) {
        return values;
    }

    KFileMetaData::UserMetaData md(file.path());
    attributes = md.queryAttributes(attributes);

    if (attributes & KFileMetaData::UserMetaData::Tags) {
        values.insert("tags", tagsFromValues(md.tags()));
    }
    if (attributes & KFileMetaData::UserMetaData::Rating) {
        values.insert("rating", QString::number(md.rating()));
    }
    if (attributes & KFileMetaData::UserMetaData::Comment) {
        values.insert("comment", md.userComment());
    }
    if (attributes & KFileMetaData::UserMetaData::OriginUrl) {
        values.insert("originUrl", md.originUrl());
    }

    return values;
}

KBalooRolesProvider::KBalooRolesProvider()
{
    // Display roles filled from Baloo property cache
    for (const auto& role : propertyRoleMap()) {
        m_roles.insert(role);
    }
    m_roles.insert("dimensions");

    // Display roles provided by UserMetaData
    m_roles.insert(QByteArrayLiteral("tags"));
    m_roles.insert(QByteArrayLiteral("rating"));
    m_roles.insert(QByteArrayLiteral("comment"));
    m_roles.insert(QByteArrayLiteral("originUrl"));
}

