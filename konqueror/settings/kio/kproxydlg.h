//-----------------------------------------------------------------------------
//
// Proxy Options
//
// (c) Lars Hoss <Lars.Hoss@munich.netsurf.de>
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998

#ifndef __KPROXYDLG_H
#define __KPROXYDLG_H "$Id"

#include <qdialog.h>
#include <qpushbutton.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include <kcontrol.h>
class KConfig;

extern KConfig *g_pConfig;

class KProxyOptions : public KConfigWidget
{
Q_OBJECT
  public:
    KProxyOptions(QWidget *parent = 0L, const char *name = 0L);
    ~KProxyOptions();
    
    virtual void loadSettings();
    virtual void saveSettings();
    virtual void applySettings();
    virtual void defaultSettings();
    
  private:
    bool useProxy;
    
    // a little information for the user
    QLabel *lb_info;
    
    // ftp proxy fields
    QLabel *lb_ftp_url;		// label ftp url
    QLineEdit *le_ftp_url;	// lineedit ftp url
    QLabel *lb_ftp_port;	// and so on :)
    QLineEdit *le_ftp_port;

    // http proxy fields
    QLabel *lb_http_url;
    QLineEdit *le_http_url;
    QLabel *lb_http_port;
    QLineEdit *le_http_port;  

    // "no proxy for" fields
    QLabel *lb_no_prx;
    QLineEdit *le_no_prx;

    // copy down butto
    QPushButton *cp_down;
    // use proxy checker
    QCheckBox *cb_useProxy;

    void setProxy();
    void readOptions();
    
  public slots:
    void copyDown();		// use the http setting for all services
    void changeProxy();
    void updateGUI(QString httpProxy, QString ftpProxy, bool bUseProxy,
                   QString noProxyFor);
};

#endif // __KPROXYDLG_H
