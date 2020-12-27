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
#include <QDebug>
#include <QTime>

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
        const KFileMetaData::PropertyInfo propertyInfo(key);
        const QByteArray role = roleForProperty(propertyInfo.name());

        auto rangeEnd = std::find_if(rangeBegin, propMap.constKeyValueEnd(),
            [key](const entry& e) { return e.first != key; });

        if (role.isEmpty() || !roles.contains(role)) {
            rangeBegin = rangeEnd;
            continue;
        }

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

    KFileMetaData::UserMetaData md(file.path());
    if (roles.contains("tags")) {
        values.insert("tags", tagsFromValues(md.tags()));
    }
    if (roles.contains("rating")) {
        values.insert("rating", QString::number(md.rating()));
    }
    if (roles.contains("comment")) {
        values.insert("comment", md.userComment());
    }
    if (roles.contains("originUrl")) {
        values.insert("originUrl", md.originUrl());
    }

    return values;
}

KBalooRolesProvider::KBalooRolesProvider() :
    m_roles(),
    m_roleForProperty()
{
    struct PropertyInfo
    {
        const char* const property;
        const char* const role;
    };

    // Mapping from the URIs to the KFileItemModel roles. Note that this must not be
    // a 1:1 mapping: One role may contain several URI-values
    static const PropertyInfo propertyInfoList[] = {
        { "rating", "rating" },
        { "tag",        "tags" },
        { "comment",   "comment" },
        { "title",         "title" },
        { "wordCount",     "wordCount" },
        { "lineCount",     "lineCount" },
        { "width",         "width" },
        { "height",        "height" },
        { "imageDateTime",   "imageDateTime"},
        { "imageOrientation", "orientation", },
        { "artist",     "artist" },
        { "genre",	"genre"  },
        { "album",    "album" },
        { "duration",      "duration" },
        { "bitRate", "bitrate" },
        { "aspectRatio", "aspectRatio" },
        { "frameRate", "frameRate" },
        { "releaseYear",    "releaseYear" },
        { "trackNumber",   "track" },
        { "originUrl", "originUrl" }
    };

    for (unsigned int i = 0; i < sizeof(propertyInfoList) / sizeof(PropertyInfo); ++i) {
        m_roleForProperty.insert(propertyInfoList[i].property, propertyInfoList[i].role);
        m_roles.insert(propertyInfoList[i].role);
    }

}

