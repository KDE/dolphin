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

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QSpinBox;

#include <kcmodule.h>


class KProxyOptions : public KCModule
{
Q_OBJECT
  public:
    KProxyOptions(QWidget *parent = 0L, const char *name = 0L);
    ~KProxyOptions();
    
    virtual void load();
    virtual void save();
    virtual void defaults();

    QString quickHelp() const;
    
  private:
    // a little information for the user
    QLabel *lb_info;
    
    // ftp proxy fields
    QLabel *lb_ftp_url;		// label ftp url
    QLineEdit *le_ftp_url;	// lineedit ftp url
    QLabel *lb_ftp_port;	// and so on :)
    QSpinBox *sb_ftp_port;

    // http proxy fields
    QLabel *lb_http_url;
    QLineEdit *le_http_url;
    QLabel *lb_http_port;
    QSpinBox *sb_http_port;  

    // "no proxy for" fields
    QLabel *lb_no_prx;
    QLineEdit *le_no_prx;

    // Maximum Cache Size
    QLabel *lb_max_cache_size;
    QSpinBox *sb_max_cache_size;

    // Maximum Cache Age
    QLabel *lb_max_cache_age;
    QSpinBox *sb_max_cache_age;

    // copy down butto
    QPushButton *cp_down;
    // use proxy checker
    QCheckBox *cb_useProxy;
    // use cache checker
    QCheckBox *cb_useCache;

    void setProxy();
    void setCache();
    void readOptions();

  private slots:

    void changed();

    
  public slots:
    void copyDown();		// use the http setting for all services
    void changeProxy();
    void changeCache();
    void updateGUI(QString httpProxy, QString ftpProxy, bool bUseProxy,
                   QString noProxyFor);
};

#endif // __KPROXYDLG_H
