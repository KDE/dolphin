#ifndef KIOPREFERENCES_H
#define KIOPREFERENCES_H

#include <kcmodule.h>

class QLabel;
class QVGroupBox;
class QCheckBox;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;

class KIntNumInput;

class KIOPreferences : public KCModule
{
    Q_OBJECT

public:
    KIOPreferences( QWidget* parent = 0);
    ~KIOPreferences();

    void load();
    void save();
    void defaults();

    QString quickHelp() const;

protected slots:
    void configChanged() { emit changed(true); }

private:
    QVGroupBox* gb_Ftp;
    QVGroupBox* gb_Timeout;
    QCheckBox* cb_ftpEnablePasv;
    QCheckBox* cb_ftpMarkPartial;

    KIntNumInput* sb_socketRead;
    KIntNumInput* sb_proxyConnect;
    KIntNumInput* sb_serverConnect;
    KIntNumInput* sb_serverResponse;
};

#endif // KIOPREFERENCES_H
