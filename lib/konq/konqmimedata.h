#ifndef KONQMIMEDATA_H
#define KONQMIMEDATA_H

#include <libkonq_export.h>
#include <kurl.h>
class QMimeData;

// Clipboard/dnd data for: (kde) urls, most-local urls, isCut
class LIBKONQ_EXPORT KonqMimeData
{
public:
    /**
     * Populate a QMimeData with urls, and whether they were cut or copied.
     *
     * @param kdeURLs list of urls (which can include kde-specific protocols)
     * @param mostLocalURLs "most local urls" (which try to resolve those to file:/ where possible),
     * @param cut if true, the user selected "cut" (saved as application/x-kde-cutselection in the mimedata).
     */
    static void populateMimeData( QMimeData* mimeData,
                                  const KURL::List& kdeURLs,
                                  const KURL::List& mostLocalURLs,
                                  bool cut = false );

    // TODO other methods for icon positions

    /**
     * @return true if the urls in @p mimeData were cut
     */
    static bool decodeIsCutSelection( const QMimeData *mimeData );
};

#endif /* KONQMIMEDATA_H */

