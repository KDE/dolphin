#include <KonquerorIface.h>

bool KonquerorIface::process(const QCString &fun, const QByteArray &data, QCString& replyType, QByteArray &replyData)
{
	if ( fun == "configure()" )
	{
		replyType = "void";
		configure( );
		return TRUE;
	}
	if ( fun == "openBrowserWindow(QString)" )
	{
		QDataStream str( data, IO_ReadOnly );
		QString url;
		str >> url;
		replyType = "void";
		openBrowserWindow(url );
		return TRUE;
	}
	if ( DCOPObject::process( fun, data, replyType, replyData ) )
		return TRUE;
	return FALSE;
}

