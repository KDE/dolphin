// kcookiesmanagement.cpp - View/delete received cookies
//
// Original framework by (Waldo, David, Adawit)?
// This dialog created by Marco Pinelli <pinmc@orion.it>

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdatetime.h>
#include <qvaluelist.h>

#include <kglobal.h>
#include <klocale.h>
#include <klistview.h>
#include <kdialog.h>
#include <dcopclient.h>

#include "kcookiesmanagement.h"

struct CookieProp
{
    QString domain;
    QString name;
    QString path;
    QString setBy;
    QString value;
    QString protoVer;
    QString expireDate;
    QString secure;
    bool allLoaded;
};

CookieListViewItem::CookieListViewItem(QListView *parent, QString dom)
                   :QListViewItem(parent)
{
    init( 0, dom );
}

CookieListViewItem::CookieListViewItem(QListViewItem *parent, CookieProp *cookie)
                   :QListViewItem(parent)
{
    init( cookie );
}

CookieListViewItem::~CookieListViewItem()
{
    delete mCookie;
}

void CookieListViewItem::init( CookieProp* cookie, QString domain,
                               bool cookieLoaded )
{
    mCookie = cookie;
    mDomain = domain;
    mCookiesLoaded = cookieLoaded;
}

CookieProp* CookieListViewItem::leaveCookie()
{
    CookieProp *ret = mCookie;
    mCookie = 0;
    return ret;
}

QString CookieListViewItem::text(int f) const
{
    if (mCookie)
        return f == 0 ? QString::null : mCookie->name;
    else
        return f == 0 ? mDomain : QString::null;
}

KCookiesManagement::KCookiesManagement(QWidget *parent, const char *name)
                   :KCModule(parent, name)
{
	// Toplevel layout
	QVBoxLayout *layout = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
	// Cookie list and buttons layout
	QHBoxLayout *lst_layout = new QHBoxLayout( layout );

	// The cookie list
	cLV = new KListView( this );
	cLV->addColumn(i18n("Domain"));
	cLV->addColumn(i18n("Cookie name"));
	cLV->setRootIsDecorated(true);
	cLV->setAllColumnsShowFocus(true);
	cLV->setSorting(0);
	lst_layout->addWidget(cLV);

	// Buttons layout
	QVBoxLayout *btn_layout = new QVBoxLayout( lst_layout );

	// The command buttons
	delBT = new QPushButton(i18n("Delete"), this);
	delBT->setEnabled(false);
	btn_layout->addWidget(delBT);

	delAllBT = new QPushButton(i18n("Delete all"), this);
	delAllBT->setEnabled(false);
	btn_layout->addWidget(delAllBT);

	// I like the buttons to be at top
	btn_layout->addStretch();

	// Cookie details layout
	QGroupBox *dtl_group = new QGroupBox(1, Qt::Horizontal, i18n("Cookie details"), this);
	layout->addWidget(dtl_group);

	QWidget *details = new QWidget(dtl_group); // Layout would be screwed alot without this
	QGridLayout *dtl_layout = new QGridLayout(details, 8, 2, KDialog::marginHint(), KDialog::spacingHint());

	dtl_layout->addWidget( new QLabel(i18n("Target Domain"),    details), 0, 0 );
	dtl_layout->addWidget( new QLabel(i18n("Target Path"),      details), 0, 1 );
	dtl_layout->addWidget( new QLabel(i18n("Value"),            details), 2, 0 );
	dtl_layout->addWidget( new QLabel(i18n("Set by Host"),      details), 4, 0 );
	dtl_layout->addWidget( new QLabel(i18n("Expires On"),       details), 4, 1 );
	dtl_layout->addWidget( new QLabel(i18n("Protocol Version"), details), 6, 0 );
	dtl_layout->addWidget( new QLabel(i18n("Is Secure"),        details), 6, 1 );


	dtl_layout->addWidget(domainLE   = new QLineEdit(details), 1, 0);
	dtl_layout->addWidget(pathLE     = new QLineEdit(details), 1, 1);
	dtl_layout->addMultiCellWidget(valueLE = new QLineEdit(details), 3, 3, 0, 1);
	dtl_layout->addWidget(setByLE    = new QLineEdit(details), 5, 0);
	dtl_layout->addWidget(expiresLE  = new QLineEdit(details), 5, 1);
	dtl_layout->addWidget(protoVerLE = new QLineEdit(details), 7, 0);
	dtl_layout->addWidget(isSecureLE = new QLineEdit(details), 7, 1);

	domainLE->setReadOnly(true);
	pathLE->setReadOnly(true);
	valueLE->setReadOnly(true);
	setByLE->setReadOnly(true);
	expiresLE->setReadOnly(true);
	protoVerLE->setReadOnly(true);
	isSecureLE->setReadOnly(true);

	connect(cLV, SIGNAL(expanded(QListViewItem*)), SLOT(getCookies(QListViewItem*)) );
	connect(cLV, SIGNAL(selectionChanged(QListViewItem*)), SLOT(showCookieDetails(QListViewItem*)) );
	connect(delBT, SIGNAL(clicked()), SLOT(deleteCookie()));
	connect(delAllBT, SIGNAL(clicked()), SLOT(deleteAllCookies()));

	deletedCookies.setAutoDelete(true);
	deleteAll = false;

	dcop = new DCOPClient();
	if( dcop->attach() )
	{
		getDomains();
	}
	else
	{
	// TODO
	}
}

KCookiesManagement::~KCookiesManagement()
{
	delete dcop;
}

void KCookiesManagement::load()
{
	deletedCookies.clear();
	deletedDomains.clear();
	deleteAll = false;
	cLV->clear();
	clearCookieDetails();
	getDomains();
}

void KCookiesManagement::save()
{
	bool success = true;

	if(deleteAll)
	{
		QByteArray call;
		QByteArray reply;
		QCString replyType;

		if( dcop->call("kcookiejar", "kcookiejar", "deleteAllCookies()", call, replyType, reply) )
		{
			// deleted[Cookies|Domains] have been cleared yet
			deleteAll = false;
		}
		else
		{
		// TODO
		}

		return;
	}

	QStringList::Iterator dom = deletedDomains.begin();
	while(dom != deletedDomains.end())
	{
		QByteArray call;
		QByteArray reply;
		QCString replyType;
		QDataStream callStream(call, IO_WriteOnly);

		callStream << *dom;

		if( dcop->call("kcookiejar", "kcookiejar", "deleteCookiesFromDomain(QString)", call, replyType, reply) )
		{
			dom = deletedDomains.remove(dom);
		}
		else
		{
			success = false;
			break;
		}
	}

	if(!success)
	{
	// TODO
	}

	success = true; // Maybe we can go on...
	QDictIterator<QList<CookieProp> > cookiesDom(deletedCookies);
	while(cookiesDom.current())
	{
		QString currDom = cookiesDom.currentKey();
		QList<CookieProp> *list = cookiesDom.current();
		QListIterator<CookieProp> cookie(*list);
		while(*cookie)
		{
			QByteArray call;
			QByteArray reply;
			QCString replyType;
			QDataStream callStream(call, IO_WriteOnly);

			callStream << currDom << (*cookie)->domain << (*cookie)->path << (*cookie)->name;

			if( dcop->call("kcookiejar", "kcookiejar", "deleteCookie(QString,QString,QString,QString)",
			               call, replyType, reply) )
			{
				list->removeRef(*cookie);
			}
			else
			{
				success = false;
				break;
			}
		}

		if(success)
		{
			deletedCookies.remove(currDom);
		}
		else break;
        }

	if(!success)
	{
	// TODO
	}
}

void KCookiesManagement::defaults()
{
}

QString KCookiesManagement::quickHelp() const
{
	return i18n("<h1>KCookiesManagement::quickHelp()</h1>" );
}

void KCookiesManagement::changed()
{
	emit KCModule::changed(true);
}

void KCookiesManagement::getDomains()
{
	QByteArray call;
	QByteArray reply;
	QCString replyType;
	QStringList domains;

	if( dcop->call("kcookiejar", "kcookiejar", "findDomains()",
		call, replyType, reply) && replyType == "QStringList")
	{
		QDataStream replyStream(reply, IO_ReadOnly);

		replyStream >> domains;
		CookieListViewItem *dom;

		for(QStringList::Iterator dIt = domains.begin();
		dIt != domains.end(); dIt++)
		{
			dom = new CookieListViewItem(cLV, *dIt);
			dom->setExpandable(true);
		}
	}
	else
	{
	// TODO
	}

	delAllBT->setEnabled(cLV->childCount());
}

void KCookiesManagement::getCookies(QListViewItem *cookieDom)
{
	if(static_cast<CookieListViewItem*>(cookieDom)->cookiesLoaded())
	{
		// BUG? This does not work when cookieDom is the last item in the view
		cLV->ensureItemVisible(cookieDom->firstChild());
		return;
	}

	QByteArray call;
	QByteArray reply;
	QCString replyType;
	QValueList<int> fields;
	QStringList fieldVal;

	QDataStream callStream(call, IO_WriteOnly);

	fields << 0 << 1 << 2;
	callStream << fields << (static_cast<CookieListViewItem*>(cookieDom))->domain()
	           << QString::null << QString::null << QString::null;

	if( dcop->call("kcookiejar", "kcookiejar", "findCookies(QValueList<int>,QString,QString,QString,QString)",
	               call, replyType, reply) && replyType == "QStringList" )
	{
		QDataStream replyStream(reply, IO_ReadOnly);

		replyStream >> fieldVal;

		QStringList::Iterator fIt = fieldVal.begin();
		while(fIt != fieldVal.end())
		{
			CookieProp *details = new CookieProp;

			details->domain    = *fIt++;
			details->path      = *fIt++;
			details->name      = *fIt++;
			details->allLoaded = false;

			new CookieListViewItem(cookieDom, details);
		}

		static_cast<CookieListViewItem*>(cookieDom)->setCookiesLoaded();
		// This always works (see above)
		cLV->ensureItemVisible(cookieDom->firstChild());
	}
	else
	{
	//TODO
	}
}


bool KCookiesManagement::getCookieDetails(QString domain, CookieProp *cookie)
{
	if( cookie->allLoaded )
		return true;

	QByteArray call;
	QByteArray reply;
	QCString replyType;
	QValueList<int> fields;
	QStringList fieldVal;
	QDateTime expDate;

	QDataStream callStream(call, IO_WriteOnly);

	fields << 3 << 4 << 5 << 6 << 7;
	callStream << fields << domain << cookie->domain << cookie->path << cookie->name;

	if( dcop->call("kcookiejar", "kcookiejar", "findCookies(QValueList<int>,QString,QString,QString,QString)",
	               call, replyType, reply) && replyType == "QStringList" )
	{
		QDataStream replyStream(reply, IO_ReadOnly);

		replyStream >> fieldVal;

		QStringList::Iterator c = fieldVal.begin();
		cookie->setBy = *c++;
		cookie->value = *c++;

		unsigned tmp = (*c++).toUInt();
		if( tmp == 0 ) {
			cookie->expireDate = i18n("End of session");
		}
		else
		{
			expDate.setTime_t(tmp);
			cookie->expireDate = KGlobal::locale()->formatDateTime(expDate);
		}

		cookie->protoVer   = *c++;
		tmp = (*c).toUInt();
		cookie->secure     = i18n(tmp ? "Yes" : "No");

		cookie->allLoaded = true;

		return true;
	}
	else return false;
}

void KCookiesManagement::showCookieDetails(QListViewItem* itm)
{
	CookieProp *cookie = static_cast<CookieListViewItem*>(itm)->cookie();

	if( cookie )
	{
		if( getCookieDetails(static_cast<CookieListViewItem*>(itm->parent())->domain(), cookie) )
		{
			domainLE->setText(cookie->domain);
			pathLE->setText(cookie->path);
			valueLE->setText(cookie->value);
			setByLE->setText(cookie->setBy);
			expiresLE->setText(cookie->expireDate);
			protoVerLE->setText(cookie->protoVer);
			isSecureLE->setText(cookie->secure);

			// I'd rather see the beginning of long values
			domainLE->setCursorPosition(0);
			pathLE->setCursorPosition(0);
			valueLE->setCursorPosition(0);
			setByLE->setCursorPosition(0);
			// You never know...
			expiresLE->setCursorPosition(0);
			protoVerLE->setCursorPosition(0);
			isSecureLE->setCursorPosition(0);
		}
		else // DCOP call gone wrong
		{
		// TODO
		}
	}
	else
	{
		clearCookieDetails();
		domainLE->setText(static_cast<CookieListViewItem*>(itm)->domain());
		domainLE->setCursorPosition(0);
	}

	delBT->setEnabled(true);
}

void KCookiesManagement::clearCookieDetails()
{
	domainLE->clear();
	pathLE->clear();
	valueLE->clear();
	setByLE->clear();
	expiresLE->clear();
	protoVerLE->clear();
	isSecureLE->clear();
}

void KCookiesManagement::deleteCookie()
{
	CookieListViewItem *itm = static_cast<CookieListViewItem*>(cLV->currentItem());
	CookieProp *cookie = itm->cookie();

	if(cookie)
	{
		CookieListViewItem *parent = static_cast<CookieListViewItem*>(itm->parent());
		QList<CookieProp> *list = deletedCookies.find(parent->domain());

		if(!list)
		{
			list = new QList<CookieProp>;
			list->setAutoDelete(true);
			deletedCookies.insert(parent->domain(), list);
		}

		list->append(itm->leaveCookie());

		delete itm;
		if(parent->childCount() == 0)
			delete parent;
	}
	else
	{
		deletedDomains.append(itm->domain());
		delete itm;
		if (cLV->currentItem())
		    cLV->setSelected(cLV->currentItem(), true);
	}

	clearCookieDetails();
	delBT->setEnabled(cLV->selectedItem());
	delAllBT->setEnabled(cLV->childCount());
	changed();
}

void KCookiesManagement::deleteAllCookies()
{
	clearCookieDetails();
	cLV->clear();
	deletedDomains.clear();
	deletedCookies.clear();
	deleteAll = true;
	delBT->setEnabled(false);
	delAllBT->setEnabled(false);
	changed();
}

#include "kcookiesmanagement.moc"
