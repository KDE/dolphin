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
#include <kpopupmenu.h>
#include <klocale.h>
#include <kiconloader.h>


// A wrapper for KHTMLPart to make it behave the way we want it to.
class KHTMLSideBar : public KHTMLPart
{
	Q_OBJECT
	public:
		KHTMLSideBar(bool universal) : KHTMLPart() {
			setStatusMessagesEnabled(false);
			setMetaRefreshEnabled(true);
			setJavaEnabled(false);
			setPluginsEnabled(false);

			setFormNotification(KHTMLPart::Only);
			connect(this,
				SIGNAL(formSubmitNotification(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)),
				this,
				SLOT(formProxy(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&))
				);


			_linkMenu = new KPopupMenu(widget(),
					"link context menu");
			if (!universal) {
				_linkMenu->insertItem(i18n("&Open Link"),
						this, SLOT(loadPage()));
				_linkMenu->insertItem(i18n("Open in New &Window"),
						this, SLOT(loadNewWindow()));
			} else {
				_linkMenu->insertItem(i18n("Open in New &Window"),
						this, SLOT(loadPage()));
			}
			_menu = new KPopupMenu(widget(), "context menu");
			_menu->insertItem(SmallIcon("reload"), i18n("&Reload"),
					this, SIGNAL(reload()));

			connect(this,
				SIGNAL(popupMenu(const QString&,const QPoint&)),
				this,
				SLOT(showMenu(const QString&, const QPoint&)));

		}
		virtual ~KHTMLSideBar() {}

	signals:
		void submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&);
		void openURLRequest(const QString& url, KParts::URLArgs args);
		void openURLNewWindow(const QString& url, KParts::URLArgs args);
		void reload();

	protected:
		virtual void urlSelected( const QString &url, int button,
				int state, const QString &_target,
				KParts::URLArgs args = KParts::URLArgs()) {
			if (button == LeftButton &&
				_target.lower() == "_content") {
				emit openURLRequest(completeURL(url).url(),
							args);
				return;
			}
			if (button == MidButton) {
				emit openURLNewWindow(completeURL(url).url(),
							args);
				return;
			}
				// A refresh
			if (button == 0 && _target.lower() == "_self") {
				openURL(completeURL(url));
				return;
			}
			KHTMLPart::urlSelected(url,button,state,_target,args);
		}

	protected slots:
		void loadPage() {
			emit openURLRequest(completeURL(_lastUrl).url(),
						KParts::URLArgs());
		}

		void loadNewWindow() {
			emit openURLNewWindow(completeURL(_lastUrl).url(),
						KParts::URLArgs());
		}

		void showMenu(const QString& url, const QPoint& pos) {
			if (url.isEmpty()) {
				_menu->popup(pos);
			} else {
				_lastUrl = url;
				_linkMenu->popup(pos);
			}
		}

		void formProxy(const char *action,
				const QString& url,
				const QByteArray& formData,
				const QString& target,
				const QString& contentType,
				const QString& boundary) {
			QString t = target.lower();
			QString u;

			if (QCString(action).lower() != "post") {
				// GET
				KURL kurl = completeURL(url);
				kurl.setQuery(formData.data());
				u = kurl.url();
			} else {
				u = completeURL(url).url();
			}

			// Some sites seem to use empty targets to send to the
			// main frame.
			if (t.isEmpty() || t == "_content") {
				emit submitFormRequest(action, u, formData,
						target, contentType, boundary);
			} else if (t == "_self") {
				setFormNotification(KHTMLPart::NoNotification);
				submitFormProxy(action, u, formData, target,
						contentType, boundary);
				setFormNotification(KHTMLPart::Only);
			}
		}
	private:
		KPopupMenu *_menu, *_linkMenu;
		QString _lastUrl;
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
		void submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&);
		void openURLRequest(const KURL &url, const KParts::URLArgs &args);
		void createNewWindow(const KURL &url, const KParts::URLArgs &args);
	protected:
		virtual void handleURL(const KURL &url);

	private slots:
		void urlClicked(const QString& url, KParts::URLArgs args);
		void urlNewWindow(const QString& url, KParts::URLArgs args);
		void pageLoaded();
		void loadFavicon();
		void setTitle(const QString&);
		void reload();

	private:
		KHTMLSideBar *_htmlPart;
		KURL _url;
		QString _desktopName;
};

#endif

