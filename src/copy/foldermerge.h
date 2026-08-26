/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FOLDERMERGE_H
#define FOLDERMERGE_H

#include "dolphin_export.h"

class KJob;

namespace KIO
{
class DropJob;
class PasteJob;
}

/**
 * @brief Folders that meet a folder of the same name are merged, not queried.
 *
 * KIO stops at a folder that already exists at the destination and asks what
 * to do with it, offering "Write Into" among rename, skip and cancel. Windows
 * Explorer does not ask: two folders of the same name become one holding both
 * sides, and only the files inside that actually clash are worth a question.
 *
 * These functions answer that folder question with "write into" on the user's
 * behalf, for one job. A file meeting a file of the same name is left alone
 * and still brings up KIO's usual overwrite, rename and skip dialog.
 */
namespace FolderMerge
{
/*! Merges same-named folders for @p job, a copy or move job. */
DOLPHIN_EXPORT void enableFor(KJob *job);

/*! Same, for the copy job @p job starts once the drop is sorted out. */
DOLPHIN_EXPORT void enableForCopiesOf(KIO::DropJob *job);

/*! Same, for the copy job @p job starts for pasted files. */
DOLPHIN_EXPORT void enableForCopiesOf(KIO::PasteJob *job);
}

#endif
