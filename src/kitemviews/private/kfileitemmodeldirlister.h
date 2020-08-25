/*
 * SPDX-FileCopyrightText: 2006-2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMMODELDIRLISTER_H
#define KFILEITEMMODELDIRLISTER_H

#include "dolphin_export.h"

#include <KDirLister>

#include <QUrl>

/**
 * @brief Extends the class KDirLister by emitting a signal when an
 *        error occurred instead of showing an error dialog.
 *        KDirLister::autoErrorHandlingEnabled() is set to false.
 */
class DOLPHIN_EXPORT KFileItemModelDirLister : public KDirLister
{
    Q_OBJECT

public:
    explicit KFileItemModelDirLister(QObject* parent = nullptr);
    ~KFileItemModelDirLister() override;

signals:
    /** Is emitted whenever an error has occurred. */
    void errorMessage(const QString& msg);

    /**
     * Is emitted when the URL of the directory lister represents a file.
     * In this case no signal errorMessage() will be emitted.
     */
    void urlIsFileError(const QUrl& url);

protected:
    void handleError(KIO::Job* job) override;
};

#endif
