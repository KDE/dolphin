// kcookiesmanagement.h - View/delete received cookies
//
// Original framework by (Waldo, David, Adawit)?
// This dialog created by Marco Pinelli <pinmc@orion.it>

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
    void init( CookieProp* cookie, QString domain = QString::null,
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
    void showCookieDetails(QListViewItem*);
    void getCookies(QListViewItem*);
    void deleteCookie();
    void deleteAllCookies();

private:
    void getDomains();
    bool getCookieDetails(QString domain, CookieProp *cookie);
    void clearCookieDetails();

    KListView *cLV;
    QLineEdit *domainLE;
    QLineEdit *pathLE;
    QLineEdit *valueLE;
    QLineEdit *setByLE;
    QLineEdit *expiresLE;
    QLineEdit *protoVerLE;
    QLineEdit *isSecureLE;
    QPushButton *delBT;
    QPushButton *delAllBT;
    DCOPClient *dcop;
    QDict<QList<CookieProp> > deletedCookies;
    QStringList deletedDomains;
    bool deleteAll;
};

#endif // __KCOOKIESMANAGEMENT_H
