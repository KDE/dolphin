// kcookiesmain.h - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#ifndef __KCOOKIESMAIN_H
#define __KCOOKIESMAIN_H

#include <kcmodule.h>

class QTabWidget;
class DCOPClient;
class KCookiesPolicies;
class KCookiesManagement;

class KCookiesMain : public KCModule
{
    Q_OBJECT
public:
    KCookiesMain(QWidget *parent, const QStringList &args);
    ~KCookiesMain();

    KCookiesPolicies* policyDlg() { return policies; }

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp() const;

private:

    QTabWidget* tab;
    KCookiesPolicies* policies;
    KCookiesManagement* management;
};

#endif // __KCOOKIESMAIN_H
