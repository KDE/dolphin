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
// Proxy autoconfig
// (c) Malte Starostik <malte@kde.org> 2000

#ifndef __KPROXYDLG_H
#define __KPROXYDLG_H "$Id"

class QLabel;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QVButtonGroup;
class KURLRequester;

class KProxyDlgUI;

#include <qspinbox.h>
#include <qlineedit.h>

#include <kcmodule.h>
#include <kprotocolmanager.h>
#include <kdialogbase.h>


// ##############################################################

class KMySpinBox : public QSpinBox
{
public:
   KMySpinBox( QWidget* parent, char *name )
    : QSpinBox(parent, name) { }
   KMySpinBox( int minValue, int maxValue, int step, QWidget* parent)
    : QSpinBox(minValue, maxValue, step, parent) { }
   QLineEdit *editor() const { return QSpinBox::editor(); }
   int value() const
   {
      #ifdef __GNUC__
      #warning workaround for a bug of QSpinBox in >= Qt 2.2.0
      #endif
      if ( editor()->edited() )
      {
          KMySpinBox *non_const_this  = const_cast<KMySpinBox*>(this);
          bool b = non_const_this->signalsBlocked();
          non_const_this->blockSignals(true);
          non_const_this->interpretText();
          non_const_this->blockSignals(b);
      }
      return QSpinBox::value();
   }
};

// ##############################################################

// Abstract class for the Proxy settings dialogs.
class KProxySetDlgBase : public KDialogBase
{
Q_OBJECT
public:
  KProxySetDlgBase(QWidget *parent = 0L, const char *name = 0L);
//  ~KProxySetDlgBase();

  QLabel *title;
  QLineEdit *input;
public slots:
    void textChanged ( const QString & );
};

class KAddHostDlg : public KProxySetDlgBase
{
Q_OBJECT
public:
  KAddHostDlg(QWidget *parent = 0L, const char *name = 0L);
//  ~KAddHostDlg();

  virtual void accept();

signals:
   void sigHaveNewHost(QString);
};

class KEditHostDlg : public KProxySetDlgBase
{
Q_OBJECT
public:
  KEditHostDlg(const QString &host, QWidget *parent = 0L, const char *name = 0L);
//  ~KEditHostDlg();

    virtual void accept();

signals:
   void sigHaveChangedHost(QString);
};

// ######################################################

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
    void updateButtons();

  private slots:

    void changed();


  public slots:
    void copyDown();		// use the http setting for all services
    void changeProxy();
    void changeCache();
    void clearCache();
    void updateGUI(QString httpProxy, QString httpsProxy, QString ftpProxy,
                   KProtocolManager::ProxyType proxyType,
                   QString noProxyFor, QString autoProxy);

    void slotEnableButtons();

    void slotAddHost();
    void slotRemoveHost();
    void slotEditHost();

    void slotAddToList(QString host);
    void slotModifyActiveListEntry(QString host);
};

#endif // __KPROXYDLG_H
