// kcookiesmain.cpp - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#include <qlayout.h>
#include <qtabwidget.h>

#include <klocale.h>
#include <kapp.h>
#include <kbuttonbox.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kdebug.h>
#include <ksimpleconfig.h>
#include <dcopclient.h>

#include "kcookiesmain.h"
#include "kcookiespolicies.h"
#include "kcookiesmanagement.h"

KCookiesMain::KCookiesMain(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
     QString error;
     bool managerOK = true;
     if ( !kapp->dcopClient()->isApplicationRegistered("kcookiejar") )
     {
        if (KApplication::startServiceByDesktopName("kcookiejar", QStringList(), &error ))
        {
          kdDebug(7103) << "kcm_kio: error starting KCookiejar: " << error << "\n" << endl;
          KMessageBox::sorry(0, i18n("This control module could not start the cookie server process\n"
                                     "It will not be possible to manage received cookies"));
          managerOK = false;
        }
     }
    QVBoxLayout *layout = new QVBoxLayout(this);
    tab = new QTabWidget(this);
    layout->addWidget(tab);

    policies = new KCookiesPolicies(this);
    tab->addTab(policies, i18n("&Policy"));
    connect(policies, SIGNAL(changed(bool)), this, SLOT(moduleChanged()));

    if(managerOK)
    {
        management = new KCookiesManagement(this, "Management");
        tab->addTab(management, i18n("&Management"));
        connect(management, SIGNAL(changed(bool)), this, SLOT(moduleChanged()));
    }
}

KCookiesMain::~KCookiesMain()
{
}

void KCookiesMain::load()
{
  policies->load();
  if(management)
      management->load();
}

void KCookiesMain::save()
{
  policies->save();
  if(management)
      management->save();
}

void KCookiesMain::defaults()
{
  policies->defaults();
  if(management)
      management->defaults();
}

void KCookiesMain::moduleChanged()
{
  emit KCModule::changed(true);
}

QString KCookiesMain::quickHelp() const
{
  return i18n("<h1>Cookies</h1> Cookies contain information that Konqueror"
    " (or other KDE applications using the http protocol) stores on your"
    " computer, initiated by a remote Internet server. This means that"
    " a web server can store information about you and your browsing activities"
    " on your machine for later use. You might consider this an invasion of"
    " privacy. <p> However, cookies are useful in certain situations. For example, they"
    " are often used by Internet shops, so you can 'put things into a shopping basket'."
    " Some sites require you have a browser that supports cookies. <p>"
    " Because most people want a compromise between privacy and the benefits cookies offer,"
    " KDE offers you the ability to customize the way it handles cookies. So you might want"
    " to set KDE's default policy to ask you whenever a server wants to set a cookie,"
    " allowing you to decide. For your favorite shopping web sites that you trust, you might"
    " want to set the policy to accept, then you can access the web sites without being prompted"
    " everytime KDE receives a cookie." );
}

#include "kcookiesmain.moc"
