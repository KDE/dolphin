/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef dolphinpackageinstaller_H
#define dolphinpackageinstaller_H

#include "config-dolphin.h"

#if HAVE_PACKAGEKIT
#include <PackageKit/Transaction>
#endif
#include <KJob>

#include <QUrl>

/**
 * @brief A KJob providing simple means to install a package.
 */
class DolphinPackageInstaller : public KJob
{
public:
    /**
     * @brief Installs a system package.
     *
     * @param packageName                   A name that can be resolved to a package.
     * @param fallBackInstallationPageUrl   This url will be opened if Dolphin was installed without the PackageKit library. A good choice for this parameter
     *                                      is an appstream url that will be opened in a software store like Discover
     *                                      e.g. "appstream://org.kde.filelight.desktop". The user is then expected to install the package themselves and
     *                                      KJob::result() will be emitted when it is detected that the installation finished successfully.
     * @param isPackageInstalledCheck       A function that can be regularly checked to determine if the installation was already successful.
     */
    explicit DolphinPackageInstaller(const QString &packageName,
                                     const QUrl &fallBackInstallationPageUrl,
                                     std::function<bool()> isPackageInstalledCheck,
                                     QObject *parent = nullptr);

    /**
     * @see KJob::start().
     * Make sure to connect to the KJob::result() signal and show the KJob::errorString() to users there before calling this.
     */
    void start() override;

    /** @see KJob::errorString(). */
    inline QString errorString() const override
    {
        return m_errorString;
    };

private:
    /** @see KJob::errorString(). */
    inline void setErrorString(const QString &errorString)
    {
        m_errorString = errorString;
    };

#if HAVE_PACKAGEKIT
    /**
     * Asynchronously installs a package uniquely identified by its @param packageId using PackageKit.
     * For progress reporting this method will use DolphinPackageInstaller::connectTransactionToJobProgress().
     * This method will call KJob::emitResult() once it failed or succeeded.
     */
    void install(const QString &packageId);

    /**
     * Makes sure progress signals of @p transaction are forwarded to KJob's progress signals.
     */
    void connectTransactionToJobProgress(const PackageKit::Transaction &transaction);

private Q_SLOTS:
    /** Creates a nice user-facing error message from its parameters and then finishes this job with an @p error. */
    void slotInstallationFailed(PackageKit::Transaction::Error error, const QString &details);
#endif

private:
    /** The name of the package that is supposed to be installed. */
    const QString m_packageName;

    /** @see DolphinPackageInstaller::DolphinPackageInstaller(). */
    const QUrl m_fallBackInstallationPageUrl;

    /** @see DolphinPackageInstaller::DolphinPackageInstaller(). */
    const std::function<bool()> m_isPackageInstalledCheck;

    /** @see KJob::errorString(). */
    QString m_errorString;
};

#endif // dolphinpackageinstaller_H
