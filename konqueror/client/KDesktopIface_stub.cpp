#include <KDesktopIface_stub.h>
#include <dcopclient.h>

#include <kapp.h>

KDesktopIface_stub::KDesktopIface_stub( const QCString& _app, const QCString& _id )
	: DCOPStub( _app, _id )

{
}

void KDesktopIface_stub::rearrangeIcons(int bAsk) 
{
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
		QDataStream str( snd, IO_WriteOnly );
		str << bAsk;
	}
	kapp->dcopClient()->call( app(), obj(), "rearrangeIcons(int)", snd, _type_, rcv );
	ASSERT( _type_ == "void" );
}

void KDesktopIface_stub::selectIconsInRect(int x, int y, int dx, int dy) 
{
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
		QDataStream str( snd, IO_WriteOnly );
		str << x;
		str << y;
		str << dx;
		str << dy;
	}
	kapp->dcopClient()->call( app(), obj(), "selectIconsInRect(intintintint)", snd, _type_, rcv );
	ASSERT( _type_ == "void" );
}

void KDesktopIface_stub::selectAll() 
{
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
	}
	kapp->dcopClient()->call( app(), obj(), "selectAll()", snd, _type_, rcv );
	ASSERT( _type_ == "void" );
}

void KDesktopIface_stub::unselectAll() 
{
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
	}
	kapp->dcopClient()->call( app(), obj(), "unselectAll()", snd, _type_, rcv );
	ASSERT( _type_ == "void" );
}

QStringList KDesktopIface_stub::selectedURLs() 
{
	QStringList _ret_;
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
	}
	kapp->dcopClient()->call( app(), obj(), "selectedURLs()", snd, _type_, rcv );
	ASSERT( _type_ == "QStringList" );
	QDataStream out( rcv, IO_ReadOnly );
	out >> _ret_;
	return _ret_;
}

void KDesktopIface_stub::configure() 
{
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	{
	}
	kapp->dcopClient()->call( app(), obj(), "configure()", snd, _type_, rcv );
	ASSERT( _type_ == "void" );
}

