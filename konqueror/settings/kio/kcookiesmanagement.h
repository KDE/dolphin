/*
    Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
*/
#ifndef __KCOOKIESMANAGEMENT_H
#define __KCOOKIESMANAGEMENT_H

#include <qdict.h>
#include <qstringlist.h>
#include <qlistview.h>

#include <kcmodule.h>

class QPushButton;
class QLineEdit;
class KListView;
class DCOPClient;
struct CookieProp;

class CookieListViewItem : public QListViewItem
{
public:
    CookieListViewItem(QListView *parent, QString dom);
    CookieListViewItem(QListViewItem *parent, CookieProp *cookie);
    ~CookieListViewItem();

    QString domain() const { return mDomain; }
    CookieProp* cookie() const { return mCookie; }
    CookieProp* leaveCookie();
    void setCookiesLoaded() { mCookiesLoaded = true; }
    bool cookiesLoaded() const { return mCookiesLoaded; }
    virtual QString text(int f) const;

private:
    void init( CookieProp* cookie,
               QString domain = QString::null,
               bool cookieLoaded=false );
    CookieProp *mCookie;
    QString mDomain;
    bool mCookiesLoaded;
};

class KCookiesManagement : public KCModule
{
    Q_OBJECT

public:
    KCookiesManagement(QWidget *parent = 0L, const char *name = 0L);
    ~KCookiesManagement();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp() const;

private slots:
    void changed();
    void deleteCookie();
    void deleteAllCookies();
    void getDomains();
    void getCookies(QListViewItem*);
    void showCookieDetails(QListViewItem*);

private:
    bool getCookieDetails(CookieProp *cookie);
    void clearCookieDetails();

    QLineEdit* le_name;
    QLineEdit* le_value;
    QLineEdit* le_domain;
    QLineEdit* le_path;
    QLineEdit* le_expires;
    QLineEdit* le_isSecure;

    KListView* lv_cookies;
    QGroupBox* grp_details;
    QPushButton* btn_delete;
    QPushButton* btn_reload;
    QPushButton* btn_deleteAll;

    DCOPClient* dcop;
    bool m_bDeleteAll;

    QStringList deletedDomains;
    typedef QList<CookieProp> CookiePropList;
    QDict<CookiePropList> deletedCookies;
};

#endif // __KCOOKIESMANAGEMENT_H
