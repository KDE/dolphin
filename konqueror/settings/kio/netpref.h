#ifndef KIOPREFERENCES_H
#define KIOPREFERENCES_H

#include <kcmodule.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QGroupBox;
class QLabel;
class QSpinBox;
class QCheckBox;

class KIOPreferences : public KCModule
{
    Q_OBJECT

public:
    KIOPreferences( QWidget* parent = 0, const char* name = 0 );
    ~KIOPreferences();

    void load();
    void save();
    void defaults();
    QString quickHelp() const;

signals:
    void changed(bool really);

protected slots:
    virtual void timeoutChanged(int val);
 void configChanged(){emit changed(true);}
private:
    QGroupBox* gb_Timeout;
    QSpinBox* sb_socketRead;
    QSpinBox* sb_proxyConnect;
    QSpinBox* sb_serverConnect;
    QSpinBox* sb_serverResponse;
    QGroupBox* gb_Ftp;
    QCheckBox* cb_ftpEnablePasv;
};

#endif // KIOPREFERENCES_H
