//-----------------------------------------------------------------------------
//
// Proxy Options
//
// (c) Lars Hoss <Lars.Hoss@munich.netsurf.de>
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998
//
// designer dialog and usage
// (c) Daniel Molkentin <molkentin@kde.org> 2000

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

class KProxyDlgUI;

#include <kcmodule.h>
#include <kprotocolmanager.h>

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

    KProxyDlgUI *ui;

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
    void updateGUI(QString httpProxy, QString ftpProxy,
                   KProtocolManager::ProxyType proxyType,
                   QString noProxyFor, QString autoProxy);
};

#endif // __KPROXYDLG_H
