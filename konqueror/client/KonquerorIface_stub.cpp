#include <KonquerorIface_stub.h>
#include <dcopclient.h>

#include <kapp.h>

KonquerorIface_stub::KonquerorIface_stub( const QCString& _app, const QCString& _id )
	: DCOPStub( _app, _id )

{
}

void KonquerorIface_stub::configure() 
{
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
	}
	kapp->dcopClient()->call( app(), obj(), "configure()", snd, _type_, rcv );
	ASSERT( _type_ == "void" );
}

void KonquerorIface_stub::openBrowserWindow(const QString& url) 
{
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
		QDataStream str( snd, IO_WriteOnly );
		str << url;
	}
	kapp->dcopClient()->call( app(), obj(), "openBrowserWindow(QString)", snd, _type_, rcv );
	ASSERT( _type_ == "void" );
}

