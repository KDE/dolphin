/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * The code is based on kdelibs/kio/directorysizejob:
 * SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef APPLYVIEWPROPSJOB_H
#define APPLYVIEWPROPSJOB_H

#include <KIO/Job>
#include <KIO/UDSEntry>

#include <QUrl>

class ViewProperties;

/**
 * @brief Applies view properties recursively to directories.
 *
 * Usage:
 * \code
 * KJob* job = new ApplyViewPropsJob(dir, viewProps);
 * connect(job, SIGNAL(result(KJob*)),
 *          this, SLOT(slotResult(KJob*)));
 * \endcode
 *
 * To be able to show a progress of the operation, the following steps
 * are recommended:
 * - Use a DirectorySizeJob to count the number of directories.
 * - Use a timer to show the current count of directories by invoking
 *   DirectorySizeJob::totalSubdirs() until the result signal is emitted.
 * - Use the ApplyViewPropsJob.
 * - Use a timer to show the progress by invoking ApplyViwePropsJob::progress().
 *   In combination with the total directory count it is possible to show a
 *   progress bar now.
 */
class ApplyViewPropsJob : public KIO::Job
{
    Q_OBJECT

public:
    /**
     * @param dir       Directory where the view properties should be applied to
     *                  (including sub directories).
     * @param viewProps View properties for the directory \a dir including its
     *                  sub directories.
     */
    ApplyViewPropsJob(const QUrl &dir, const ViewProperties &viewProps);
    ~ApplyViewPropsJob() override;
    int progress() const;

private Q_SLOTS:
    void slotResult(KJob *job) override;
    void slotEntries(KIO::Job *, const KIO::UDSEntryList &);

private:
    ViewProperties *m_viewProps;
    int m_progress;
    QUrl m_dir;
};

inline int ApplyViewPropsJob::progress() const
{
    return m_progress;
}

#endif
