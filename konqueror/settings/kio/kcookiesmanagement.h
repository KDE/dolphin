/**
 * kcookiesmanagement.h - Cookies manager
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __KCOOKIESMANAGEMENT_H
#define __KCOOKIESMANAGEMENT_H

#include <q3dict.h>
#include <QStringList>
#include <q3listview.h>
#include <Q3PtrList>

#include <kcmodule.h>


class DCOPClient;
class KCookiesManagementDlgUI;

struct CookieProp;

class CookieListViewItem : public Q3ListViewItem
{
public:
    CookieListViewItem(Q3ListView *parent, QString dom);
    CookieListViewItem(Q3ListViewItem *parent, CookieProp *cookie);
    ~CookieListViewItem();

    QString domain() const { return mDomain; }
    CookieProp* cookie() const { return mCookie; }
    CookieProp* leaveCookie();
    void setCookiesLoaded() { mCookiesLoaded = true; }
    bool cookiesLoaded() const { return mCookiesLoaded; }
    virtual QString text(int f) const;

private:
    void init( CookieProp* cookie,
               QString domain = QString(),
               bool cookieLoaded=false );
    CookieProp *mCookie;
    QString mDomain;
    bool mCookiesLoaded;
};

class KCookiesManagement : public KCModule
{
    Q_OBJECT

public:
    KCookiesManagement( KInstance *inst, QWidget *parent );
    ~KCookiesManagement();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp() const;

private Q_SLOTS:
    void deleteCookie();
    void deleteAllCookies();
    void getDomains();
    void getCookies(Q3ListViewItem*);
    void showCookieDetails(Q3ListViewItem*);
    void doPolicy();

private:
    void reset (bool deleteAll = false);
    bool cookieDetails(CookieProp *cookie);
    void clearCookieDetails();
    bool policyenabled();
    bool m_bDeleteAll;

    QWidget* mainWidget;
    KCookiesManagementDlgUI* dlg;

    QStringList deletedDomains;
    typedef Q3PtrList<CookieProp> CookiePropList;
    Q3Dict<CookiePropList> deletedCookies;
};

#endif // __KCOOKIESMANAGEMENT_H
