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
class QRadioButton;
class QVButtonGroup;
class KURLRequester;

#include <kcmodule.h>

class KMySpinBox;

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

    /// ------------ Proxy --------

    // ftp proxy fields
    QLabel *lb_ftp_url;		// label ftp url
    QLineEdit *le_ftp_url;	// lineedit ftp url
    QLabel *lb_ftp_port;	// and so on :)
    KMySpinBox *sb_ftp_port;

    // http proxy fields
    QLabel *lb_http_url;
    QLineEdit *le_http_url;
    QLabel *lb_http_port;
    KMySpinBox *sb_http_port;

    // "no proxy for" fields
    QLabel *lb_no_prx;
    QLineEdit *le_no_prx;

    // auto proxy url
    KURLRequester *url_autoProxy;

    // copy down button
    QPushButton *pb_down;
    // use proxy checker
    QCheckBox *cb_useProxy;
    // auto proxy checker
    QCheckBox *cb_autoProxy;

    /// ------------ Cache ----------

    // use cache checker
    QCheckBox *cb_useCache;
    // verify obsolete pages radio button
    QRadioButton *rb_verify;
    // use cache if possible radio button
    QRadioButton *rb_cacheIfPossible;
    // connect at all? radio button
    QRadioButton *rb_offlineMode;

    QVButtonGroup *bg_cacheControl;

    // Maximum Cache Size
    QLabel *lb_max_cache_size;
    KMySpinBox *sb_max_cache_size;

    // Maximum Cache Age
    QLabel *lb_max_cache_age;
    KMySpinBox *sb_max_cache_age;

    // Clear Cache
    QPushButton *pb_clearCache;

    void setProxy();
    void setCache();
    void readOptions();

  private slots:

    void changed();


  public slots:
    void copyDown();		// use the http setting for all services
    void changeProxy();
    void changeCache();
    void clearCache();
    void updateGUI(QString httpProxy, QString ftpProxy, bool bUseProxy,
                   bool bAutoProxy, QString noProxyFor, QString autoProxy);
};

#endif // __KPROXYDLG_H
