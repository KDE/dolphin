//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998
//
// Redesign and cleanup
// (c) Daniel Molkentin 2000
//
//-----------------------------------------------------------------------------

#ifndef __JAVAOPTS_H__
#define __JAVAOPTS_H__

#include <kcmodule.h>
#include <qmap.h>

class KColorButton;
class KConfig;
class KListView;
class KURLRequester;
class KIntNumInput;

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListViewItem;
class QRadioButton;

class KJavaOptions : public KCModule
{
    Q_OBJECT

public:
    KJavaOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );

    virtual void load();
    virtual void save();
    virtual void defaults();

private slots:
    void changed();
    void importPressed();
    void exportPressed();
    void addPressed();
    void changePressed();
    void deletePressed();
    void toggleJavaControls();

private:
    void updateDomainList(const QStringList &domainConfig);

    KConfig* m_pConfig;
    QString  m_groupname;

    KListView*     domainSpecificLV;
    QCheckBox*     enableJavaGloballyCB;
    QCheckBox*     javaConsoleCB;
    QCheckBox*     javaSecurityManagerCB;
    QCheckBox*     enableShutdownCB;
    KIntNumInput*  serverTimeoutSB;
    QLineEdit*     addArgED;
    KURLRequester* pathED;


    QMap<QListViewItem*, int> javaDomainPolicy;
};


#endif		// __HTML_OPTIONS_H__

