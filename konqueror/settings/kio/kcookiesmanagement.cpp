/**
 * kcookiesmanagement.cpp - Cookies manager
 *
 * Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
 *
 * Contributors:
 * Copyright (c) 2000-2001 Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdatetime.h>
#include <qvaluelist.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdialog.h>
#include <klistview.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kiconloader.h>

#include "kcookiesmanagement.h"

struct CookieProp
{
    QString host;
    QString name;
    QString value;
    QString domain;
    QString path;
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
        return f == 0 ? QString::null : mCookie->host;
    else
        return f == 0 ? mDomain : QString::null;
}

KCookiesManagement::KCookiesManagement(QWidget *parent, const char *name)
                   :KCModule(parent, name)
{
  // Toplevel layout
  QVBoxLayout *layout = new QVBoxLayout( this, 2*KDialog::marginHint(),
                                         KDialog::spacingHint() );
  layout->setAutoAdd( true );

  // Cookie list and buttons layout
  QHBox* hbox = new QHBox( this );
  hbox->setSpacing( KDialog::spacingHint() );

  // The cookie list
  lv_cookies = new KListView( hbox );
  lv_cookies->addColumn(i18n("Domain [Group]"));
  lv_cookies->addColumn(i18n("Host [Set By]"));
  lv_cookies->setRootIsDecorated(true);
  lv_cookies->setTreeStepSize(15);
  lv_cookies->setAllColumnsShowFocus(true);
  lv_cookies->setShowSortIndicator(true);
  lv_cookies->setSorting(0);

  // Buttons layout
  QWidget* bbox = new QWidget ( hbox );
  QBoxLayout* vlay = new QVBoxLayout( bbox );
  vlay->setSpacing( KDialog::spacingHint() );

  // The command buttons
  btn_delete = new QPushButton(i18n("De&lete"), bbox);
  btn_delete->setEnabled(false);
  vlay->addWidget(btn_delete);

  btn_deleteAll = new QPushButton(i18n("Dele&te all"), bbox);
  btn_deleteAll->setEnabled(false);
  vlay->addWidget(btn_deleteAll);

  btn_reload = new QPushButton( i18n("&Reload List"), bbox );
  vlay->addWidget(btn_reload);
  vlay->addStretch( 1 );

  // Cookie details layout
  grp_details = new QGroupBox( i18n("Cookie Details"), this);
  grp_details->setSizePolicy( QSizePolicy(QSizePolicy::Expanding,
                                          QSizePolicy::Fixed,
                                          grp_details ->sizePolicy().hasHeightForWidth()));
  grp_details->setColumnLayout(0, Qt::Vertical );
  grp_details->layout()->setSpacing( 0 );
  grp_details->layout()->setMargin( 0 );
  QGridLayout* d_lay = new QGridLayout( grp_details->layout() );
  d_lay->setAlignment( Qt::AlignTop );
  d_lay->setSpacing( 2*KDialog::spacingHint()  );
  d_lay->setMargin( 2*KDialog::marginHint() );

  QLabel* label = new QLabel(i18n("Name:"), grp_details);
  d_lay->addWidget(label,1,0);
  le_name = new QLineEdit( grp_details );
  le_name->setReadOnly(true);
  d_lay->addWidget(le_name,1,1);

  label = new QLabel(i18n("Value:"), grp_details);
  d_lay->addWidget(label,2,0);
  le_value = new QLineEdit( grp_details );
  le_value->setReadOnly(true);
  d_lay->addWidget(le_value,2,1);

  label = new QLabel(i18n("Domain:"), grp_details);
  d_lay->addWidget(label,3,0);
  le_domain = new QLineEdit( grp_details );
  le_domain->setReadOnly(true);
  d_lay->addWidget(le_domain,3,1);

  label = new QLabel(i18n("Path:"), grp_details);
  d_lay->addWidget(label,4,0);
  le_path = new QLineEdit( grp_details );
  le_path->setReadOnly(true);
  d_lay->addWidget(le_path,4,1);

  label = new QLabel(i18n("Expires:"), grp_details);
  d_lay->addWidget(label,5,0);
  le_expires = new QLineEdit( grp_details );
  le_expires->setReadOnly(true);
  d_lay->addWidget(le_expires,5,1);

  label = new QLabel(i18n("Secure:"), grp_details);
  d_lay->addWidget(label,6,0);
  le_isSecure = new QLineEdit( grp_details );
  le_isSecure->setReadOnly(true);
  d_lay->addWidget(le_isSecure,6,1);

  connect(lv_cookies, SIGNAL(expanded(QListViewItem*)), SLOT(getCookies(QListViewItem*)) );
  connect(lv_cookies, SIGNAL(selectionChanged(QListViewItem*)), SLOT(showCookieDetails(QListViewItem*)) );
  connect(btn_delete, SIGNAL(clicked()), SLOT(deleteCookie()));
  connect(btn_deleteAll, SIGNAL(clicked()), SLOT(deleteAllCookies()));
  connect(btn_reload, SIGNAL(clicked()), SLOT(getDomains()));

  deletedCookies.setAutoDelete(true);
  m_bDeleteAll = false;

  dcop = new DCOPClient();
  if( dcop->attach() )
  {
    load();
  }
  else
  {
    //TODO: Do something here!!  Like disabling the whole
    // management tab after giving a nice feedback to user.
  }
}

KCookiesManagement::~KCookiesManagement()
{
  delete dcop;
}

void KCookiesManagement::load()
{
  defaults();
  getDomains();
}

void KCookiesManagement::save()
{
  if(m_bDeleteAll)
  {
    QByteArray call;
    QByteArray reply;
    QCString replyType;

    if( dcop->call("kcookiejar", "kcookiejar", "deleteAllCookies()", call, replyType, reply) )
    {
      // deleted[Cookies|Domains] have been cleared yet
      m_bDeleteAll = false;
    }
    else
    {
    }
    return;
  }

  QStringList::Iterator dIt = deletedDomains.begin();
  while(dIt != deletedDomains.end())
  {
    QByteArray call;
    QByteArray reply;
    QCString replyType;
    QDataStream callStream(call, IO_WriteOnly);
    callStream << *dIt;

    if( dcop->call("kcookiejar", "kcookiejar",
                   "deleteCookiesFromDomain(QString)", call, replyType, reply) )
    {
      dIt = deletedDomains.remove(dIt);
    }
    else
    {
    }
  }

  bool success = true; // Maybe we can go on...
  QDictIterator<CookiePropList> cookiesDom(deletedCookies);
  while(cookiesDom.current())
  {
    CookiePropList *list = cookiesDom.current();
    QListIterator<CookieProp> cookie(*list);
    while(*cookie)
    {
      QByteArray call;
      QByteArray reply;
      QCString replyType;
      QDataStream callStream(call, IO_WriteOnly);
      callStream << (*cookie)->domain << (*cookie)->host << (*cookie)->path << (*cookie)->name;
      if( dcop->call("kcookiejar", "kcookiejar",
                     "deleteCookie(QString,QString,QString,QString)",
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
      deletedCookies.remove(cookiesDom.currentKey());
    }
    else
      break;
  }
}

void KCookiesManagement::defaults()
{
  m_bDeleteAll = false;
  clearCookieDetails();
  lv_cookies->clear();
  deletedDomains.clear();
  deletedCookies.clear();
}

void KCookiesManagement::clearCookieDetails()
{
  le_name->clear();
  le_value->clear();
  le_domain->clear();
  le_path->clear();
  le_expires->clear();
  le_isSecure->clear();
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

    if ( lv_cookies->childCount() )
    {
      defaults();
      lv_cookies->setCurrentItem( 0L );
    }

    CookieListViewItem *dom;
    QStringList::Iterator dIt = domains.begin();
    for( ; dIt != domains.end(); dIt++)
    {
      dom = new CookieListViewItem(lv_cookies, *dIt);
      dom->setExpandable(true);
    }
  }
  else
  {
    // TODO
  }
  btn_deleteAll->setEnabled(lv_cookies->childCount());
}

void KCookiesManagement::getCookies(QListViewItem *cookieDom)
{
  CookieListViewItem* ckd = static_cast<CookieListViewItem*>(cookieDom);
  if ( ckd->cookiesLoaded() )
    return;

  QByteArray call;
  QByteArray reply;
  QCString replyType;
  QDataStream callStream(call, IO_WriteOnly);

  QValueList<int> fields;
  fields << 0 << 1 << 2 << 3;  // GET: domain/name/path/host

  callStream << fields << ckd->domain() << QString::null
             << QString::null << QString::null;

  if( dcop->call("kcookiejar", "kcookiejar",
                 "findCookies(QValueList<int>,QString,QString,QString,QString)",
                 call, replyType, reply) && replyType == "QStringList" )
  {
    QDataStream replyStream(reply, IO_ReadOnly);
    QStringList fieldVal;
    replyStream >> fieldVal;

    QStringList::Iterator fIt = fieldVal.begin();
    while(fIt != fieldVal.end())
    {
      CookieProp *details = new CookieProp;
      details->domain = *fIt++;
      details->path = *fIt++;
      details->name = *fIt++;
      details->host = *fIt++;
      details->allLoaded = false;
      new CookieListViewItem(cookieDom, details);
    }
    static_cast<CookieListViewItem*>(cookieDom)->setCookiesLoaded();
  }
  else
  {
    //TODO
  }
}

bool KCookiesManagement::getCookieDetails(CookieProp *cookie)
{
  QByteArray call;
  QByteArray reply;
  QCString replyType;
  QValueList<int> fields;
  QDateTime expDate;

  QStringList fieldVal;
  fields << 4 << 5 << 7;
  QDataStream callStream(call, IO_WriteOnly);
  callStream << fields << cookie->domain << cookie->host
             << cookie->path << cookie->name;

  if( dcop->call("kcookiejar", "kcookiejar",
                 "findCookies(QValueList<int>,QString,QString,QString,QString)",
                 call, replyType, reply) && replyType == "QStringList" )
  {
    QDataStream replyStream(reply, IO_ReadOnly);
    replyStream >> fieldVal;

    QStringList::Iterator c = fieldVal.begin();
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

    tmp = (*c).toUInt();
    cookie->secure     = i18n(tmp ? "Yes" : "No");
    cookie->allLoaded = true;
    return true;
  }
  return false;
}

void KCookiesManagement::showCookieDetails(QListViewItem* item)
{
  CookieProp *cookie = static_cast<CookieListViewItem*>(item)->cookie();
  if( cookie )
  {
    if( cookie->allLoaded || getCookieDetails(cookie) )
    {
      le_name->validateAndSet(cookie->name,0,0,0);
      le_value->validateAndSet(cookie->value,0,0,0);
      le_domain->validateAndSet(cookie->domain,0,0,0);
      le_path->validateAndSet(cookie->path,0,0,0);
      le_expires->validateAndSet(cookie->expireDate,0,0,0);
      le_isSecure->validateAndSet(cookie->secure,0,0,0);
    }
  }
  else
    clearCookieDetails();

  btn_delete->setEnabled(true);
}

void KCookiesManagement::deleteCookie()
{
  CookieListViewItem *item = static_cast<CookieListViewItem*>(lv_cookies->currentItem());
  CookieProp *cookie = item->cookie();
  if(cookie)
  {
    CookieListViewItem *parent = static_cast<CookieListViewItem*>(item->parent());
    CookiePropList *list = deletedCookies.find(parent->domain());
    if(!list)
    {
      list = new CookiePropList;
      list->setAutoDelete(true);
      deletedCookies.insert(parent->domain(), list);
    }
    list->append(item->leaveCookie());
    delete item;
    if(parent->childCount() == 0)
      delete parent;
  }
  else
  {
    deletedDomains.append(item->domain());
    delete item;
  }

  if (lv_cookies->currentItem())
    lv_cookies->setSelected(lv_cookies->currentItem(), true);

  clearCookieDetails();
  btn_delete->setEnabled(lv_cookies->selectedItem());
  btn_deleteAll->setEnabled(lv_cookies->childCount());
  changed();
}

void KCookiesManagement::deleteAllCookies()
{
  defaults();
  m_bDeleteAll = true;
  btn_delete->setEnabled(false);
  btn_deleteAll->setEnabled(false);
  changed();
}

#include "kcookiesmanagement.moc"
