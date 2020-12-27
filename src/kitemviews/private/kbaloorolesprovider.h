/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2013 Vishesh Handa <me@vhanda.in>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KBALOO_ROLESPROVIDER_H
#define KBALOO_ROLESPROVIDER_H

#include "dolphin_export.h"

#include <QHash>
#include <QSet>
#include <QVariant>

namespace Baloo {
    class File;
}

/**
 * @brief Allows accessing metadata of a file by providing KFileItemModel roles.
 *
 * Is a helper class for KFileItemModelRolesUpdater to retrieve roles that
 * are only accessible with Baloo.
 */
class DOLPHIN_EXPORT KBalooRolesProvider
{
public:
    static KBalooRolesProvider& instance();
    virtual ~KBalooRolesProvider();

    /**
     * @return Roles that can be provided by KBalooRolesProvider.
     */
    QSet<QByteArray> roles() const;

    /**
     * @return Values for the roles \a roles that can be determined from the file
     *         with the URL \a url.
     */
    QHash<QByteArray, QVariant> roleValues(const Baloo::File& file,
                                           const QSet<QByteArray>& roles) const;

protected:
    KBalooRolesProvider();

private:
    QSet<QByteArray> m_roles;
    QHash<QString, QByteArray> m_roleForProperty;

    friend struct KBalooRolesProviderSingleton;
};

#endif

