/* This file is part of the KDE project
   Copyright (C) 2003 George Staikos <staikos@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef web_module_h
#define web_module_h

#include <qobject.h>
#include <konqsidebarplugin.h>
#include <khtml_part.h>


class KHTMLSideBar : public KHTMLPart
{
	Q_OBJECT
	public:
		KHTMLSideBar() : KHTMLPart() {
			setStatusMessagesEnabled(false);
			setMetaRefreshEnabled(true);
			setJavaEnabled(false);
			setPluginsEnabled(false);
		}
		virtual ~KHTMLSideBar() {}

	signals:
		void openURLRequest(const QString& url, KParts::URLArgs args);

	protected:
		virtual void urlSelected( const QString &url, int button,
				int state, const QString &_target,
				KParts::URLArgs args = KParts::URLArgs()) {
			if (button == LeftButton &&
				_target.lower() == "_content") {
				emit openURLRequest( url, args );
				return;
			}
			KHTMLPart::urlSelected(url,button,state,_target,args);
		}
};


class KonqSideBarWebModule : public KonqSidebarPlugin
{
	Q_OBJECT
	public:
		KonqSideBarWebModule(KInstance *instance, QObject *parent,
			       	QWidget *widgetParent, QString &desktopName,
			       	const char *name);
		virtual ~KonqSideBarWebModule();

		virtual QWidget *getWidget();
		virtual void *provides(const QString &);

	signals:
		void openURLRequest(const KURL &url, const KParts::URLArgs &args);
	protected:
		virtual void handleURL(const KURL &url);

	private slots:
		void urlClicked(const QString& url, KParts::URLArgs args);
		void pageLoaded();
		void loadFavicon();
		void setTitle(const QString&);

	private:
		KHTMLSideBar *_htmlPart;
		QString _url;
};

#endif

