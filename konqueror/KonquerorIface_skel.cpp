#include <KonquerorIface.h>

bool KonquerorIface::process(const QCString &fun, const QByteArray &data, QCString& replyType, QByteArray &replyData)
{
	if ( fun == "openBrowserWindow(QString)" )
	{
		QDataStream str( data, IO_ReadOnly );
		QString url;
		str >> url;
		replyType = "void";
		openBrowserWindow(url );
		return TRUE;
	}
	return FALSE;
}

