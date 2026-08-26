/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "foldermerge.h"

#include <KIO/CopyJob>
#include <KIO/DropJob>
#include <KIO/JobUiDelegate>
#include <KIO/PasteJob>
#include <KIO/WidgetsAskUserActionHandler>
#include <KJobWidgets>

#include <QFileInfo>
#include <QTimer>

namespace
{
/*!
 * KIO's own handler, with the folder question answered before it is asked.
 */
class MergingAskUserActionHandler : public KIO::WidgetsAskUserActionHandler
{
public:
    using KIO::WidgetsAskUserActionHandler::WidgetsAskUserActionHandler;

    void askUserRename(KJob *job,
                       const QString &title,
                       const QUrl &source,
                       const QUrl &destination,
                       KIO::RenameDialog_Options options,
                       KIO::filesize_t sizeSource,
                       KIO::filesize_t sizeDestination,
                       const QDateTime &ctimeSource,
                       const QDateTime &ctimeDestination,
                       const QDateTime &mtimeSource,
                       const QDateTime &mtimeDestination) override
    {
        if (bothAreFolders(source, destination, options)) {
            // Result_Overwrite on a folder is KIO's "Write Into": the folder
            // stays where it is and the copy carries on inside it, which is
            // what merging is. Answering later, not from inside this call,
            // keeps the job's state machine on its own footing.
            QTimer::singleShot(0, this, [this, job, destination]() {
                Q_EMIT askUserRenameResult(KIO::Result_Overwrite, destination, job);
            });
            return;
        }

        KIO::WidgetsAskUserActionHandler::askUserRename(job,
                                                        title,
                                                        source,
                                                        destination,
                                                        options,
                                                        sizeSource,
                                                        sizeDestination,
                                                        ctimeSource,
                                                        ctimeDestination,
                                                        mtimeSource,
                                                        mtimeDestination);
    }

private:
    static bool bothAreFolders(const QUrl &source, const QUrl &destination, KIO::RenameDialog_Options options)
    {
        // A copy names both sides in the options. A move that could not simply
        // rename the folder names only the destination, so the source is
        // looked up instead - which only works locally, and a remote move
        // keeps the dialog rather than guessing.
        const bool sourceIsFolder = options.testFlag(KIO::RenameDialog_SourceIsDirectory) || (source.isLocalFile() && QFileInfo(source.toLocalFile()).isDir());
        const bool destinationIsFolder =
            options.testFlag(KIO::RenameDialog_DestIsDirectory) || (destination.isLocalFile() && QFileInfo(destination.toLocalFile()).isDir());

        return sourceIsFolder && destinationIsFolder;
    }
};

/*!
 * The stock delegate, with only the question handler exchanged: the progress
 * reporting, the clipboard updates and every other dialog stay KIO's.
 */
class MergingUiDelegate : public KIO::JobUiDelegate
{
public:
    explicit MergingUiDelegate(QWidget *window)
        : KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingDisabled, window, {new MergingAskUserActionHandler})
    {
    }
};
}

void FolderMerge::enableFor(KJob *job)
{
    if (!job) {
        return;
    }

    // The window the job was given, so the dialogs that are still asked for
    // keep the window they would have had.
    QWidget *window = KJobWidgets::window(job);
    job->setUiDelegate(new MergingUiDelegate(window));
    if (window) {
        KJobWidgets::setWindow(job, window);
    }
}

void FolderMerge::enableForCopiesOf(KIO::DropJob *job)
{
    if (job) {
        QObject::connect(job, &KIO::DropJob::copyJobStarted, job, [](KIO::CopyJob *copyJob) {
            FolderMerge::enableFor(copyJob);
        });
    }
}

void FolderMerge::enableForCopiesOf(KIO::PasteJob *job)
{
    if (job) {
        QObject::connect(job, &KIO::PasteJob::copyJobStarted, job, [](KIO::CopyJob *copyJob) {
            FolderMerge::enableFor(copyJob);
        });
    }
}
