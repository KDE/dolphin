#include "konqmimedata.h"
#include <qmimedata.h>
#include <kdebug.h>

void KonqMimeData::populateMimeData( QMimeData* mimeData,
                                     const KURL::List& kdeURLs,
                                     const KURL::List& mostLocalURLs,
                                     bool cut )
{
    mostLocalURLs.populateMimeData( mimeData );


    // Mostly copied from KURL::List::populateMimeData
    QList<QByteArray> urlStringList;
    KURL::List::ConstIterator uit = kdeURLs.begin();
    const KURL::List::ConstIterator uEnd = kdeURLs.end();
    for ( ; uit != uEnd ; ++uit )
    {
        // Get each URL encoded in toUtf8 - and since we get it in escaped
        // form on top of that, .toLatin1() is fine.
        urlStringList.append( (*uit).toMimeDataString().toLatin1() );
    }

    QByteArray uriListData;
    for ( QList<QByteArray>::const_iterator it = urlStringList.begin(), end = urlStringList.end()
                                                 ; it != end ; ++it ) {
        uriListData += (*it);
        uriListData += "\r\n";
    }
    mimeData->setData( "text/uri-list", uriListData );

    QByteArray cutSelectionData = cut ? "1" : "0";
    mimeData->setData( "application/x-kde-cutselection", cutSelectionData );
}

bool KonqMimeData::decodeIsCutSelection( const QMimeData *mimeData )
{
  QByteArray a = mimeData->data( "application/x-kde-cutselection" );
  if ( a.isEmpty() )
    return false;
  else
  {
    kdDebug(1203) << "KonqDrag::decodeIsCutSelection : a=" << a << endl;
    return (a.at(0) == '1'); // true if 1
  }
}
