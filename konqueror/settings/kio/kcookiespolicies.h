// kcookiespolicies.h - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#ifndef __KCOOKIESPOLICIES_H
#define __KCOOKIESPOLICIES_H

#include <qdialog.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include <kkeydialog.h> // for ksplitlist
#include <kcmodule.h>
#include <kconfig.h>

class DCOPClient;

class KCookiesPolicies : public KCModule
{
Q_OBJECT
  public:
    KCookiesPolicies(QWidget *parent = 0L, const char *name = 0L);
    ~KCookiesPolicies();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp();

  public slots:
    void changeCookiesEnabled();
    void changePressed();
    void deletePressed();
    void updateDomain( int );

  public slots:
    void changed();

  private:
    void removeDomain(const QString& domain);
    void updateDomainList();

    KSplitList *wList;
    QGroupBox  *wSpecificGroupBox;

    // Cookies enabled
    QCheckBox    *cb_enableCookies;

    QButtonGroup *bg1;
    // Global cookie policies
    QRadioButton *rb_gbPolicyAccept;
    QRadioButton *rb_gbPolicyAsk;
    QRadioButton *rb_gbPolicyReject;

    QButtonGroup *bg2;
    
    // Domain cookie policies
    QLineEdit *le_domain;	
    QRadioButton *rb_domPolicyAccept;
    QRadioButton *rb_domPolicyAsk;
    QRadioButton *rb_domPolicyReject;
    QPushButton  *pb_domPolicyChange;
    QPushButton  *pb_domPolicyDelete;

    QStringList domainConfig;
};

#endif // __KCOOKIESPOLICIES_H
