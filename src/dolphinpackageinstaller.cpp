/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "dolphinpackageinstaller.h"

#include <KLocalizedString>

#if HAVE_PACKAGEKIT
#include <PackageKit/Daemon>
#else
#include <QDesktopServices>
#endif

#include <QTimer>
#include <QtAssert>

DolphinPackageInstaller::DolphinPackageInstaller(const QString &packageName,
                                                 const QUrl &fallBackInstallationPageUrl,
                                                 std::function<bool()> isPackageInstalledCheck,
                                                 QObject *parent)
    : KJob(parent)
    , m_packageName{packageName}
    , m_fallBackInstallationPageUrl{fallBackInstallationPageUrl}
    , m_isPackageInstalledCheck{isPackageInstalledCheck}
{
}

void DolphinPackageInstaller::start()
{
    if (m_isPackageInstalledCheck()) {
        emitResult();
        return;
    }

#if HAVE_PACKAGEKIT
    PackageKit::Daemon::setHints(PackageKit::Daemon::hints() + QStringList{QStringLiteral("interactive=true")});
    const PackageKit::Transaction *resolveTransaction = PackageKit::Daemon::resolve(m_packageName);

    connect(resolveTransaction, &PackageKit::Transaction::errorCode, this, &DolphinPackageInstaller::slotInstallationFailed);
    connect(resolveTransaction, &PackageKit::Transaction::finished, this, [this]() { // Will be disconnected if we find a package.
        slotInstallationFailed(PackageKit::Transaction::ErrorPackageNotFound,
                               i18nc("@info:shell about system packages", "Could not find package %1.", m_packageName));
    });
    connect(resolveTransaction,
            &PackageKit::Transaction::package,
            this,
            [this, resolveTransaction](PackageKit::Transaction::Info /* info */, const QString &packageId) {
                disconnect(resolveTransaction, nullptr, this, nullptr); // We only care about the first package.
                install(packageId);
            });
#else
    QDesktopServices::openUrl(m_fallBackInstallationPageUrl);
    auto waitForSuccess = new QTimer(this);
    connect(waitForSuccess, &QTimer::timeout, this, [this]() {
        if (m_isPackageInstalledCheck()) {
            emitResult();
        }
    });
    waitForSuccess->start(3000);
#endif
}

#if HAVE_PACKAGEKIT
void DolphinPackageInstaller::install(const QString &packageId)
{
    const PackageKit::Transaction *installTransaction = PackageKit::Daemon::installPackage(packageId);
    connectTransactionToJobProgress(*installTransaction);
    connect(installTransaction,
            &PackageKit::Transaction::errorCode,
            this,
            [installTransaction, this](PackageKit::Transaction::Error error, const QString &details) {
                disconnect(installTransaction, nullptr, this, nullptr); // We only want to emit a result once.
                slotInstallationFailed(error, details);
            });
    connect(installTransaction,
            &PackageKit::Transaction::finished,
            this,
            [installTransaction, this](const PackageKit::Transaction::Exit status, uint /* runtime */) {
                disconnect(installTransaction, nullptr, this, nullptr); // We only want to emit a result once.
                if (status == PackageKit::Transaction::ExitSuccess) {
                    emitResult();
                } else {
                    slotInstallationFailed(PackageKit::Transaction::ErrorUnknown,
                                           i18nc("@info %1 is error code",
                                                 "Installation exited without reporting success. (%1)",
                                                 QMetaEnum::fromType<PackageKit::Transaction::Exit>().valueToKey(status)));
                }
            });
}

void DolphinPackageInstaller::connectTransactionToJobProgress(const PackageKit::Transaction &transaction)
{
    connect(&transaction, &PackageKit::Transaction::speedChanged, this, [this, &transaction]() {
        emitSpeed(transaction.speed());
    });
    connect(&transaction, &PackageKit::Transaction::percentageChanged, this, [this, &transaction]() {
        setPercent(transaction.percentage());
    });
}

void DolphinPackageInstaller::slotInstallationFailed(PackageKit::Transaction::Error error, const QString &details)
{
    setErrorString(xi18nc("@info:shell %1 is package name, %2 is error message, %3 is error e.g. 'ErrorNoNetwork'",
                          "Installing <application>%1</application> failed: %2 (%3)<nl/>Please try installing <application>%1</application> manually instead.",
                          m_packageName,
                          details,
                          QMetaEnum::fromType<PackageKit::Transaction::Error>().valueToKey(error)));
    setError(error);
    emitResult();
}
#endif
