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

#include "domainlistview.h"
#include "policies.h"

class KColorButton;
class KConfig;
class KListView;
class KURLRequester;
class KIntNumInput;

class QCheckBox;
class QComboBox;
class QLineEdit;
class QRadioButton;

class KJavaOptions;

/** policies with java-specific constructor
  */
class JavaPolicies : public Policies {
public:
  /**
   * constructor
   * @param config configuration to initialize this instance from
   * @param group config group to use if this instance contains the global
   *	policies (global == true)
   * @param global true if this instance contains the global policy settings,
   *	false if this instance contains policies specific for a domain.
   * @param domain name of the domain this instance is used to configure the
   *	policies for (case insensitive, ignored if global == true)
   */
  JavaPolicies(KConfig* config, const QString &group, bool global,
  		const QString &domain = QString::null);

  /** empty constructur to make QMap happy
   * don't use for constructing a policies instance.
   * @internal
   */
  JavaPolicies();

  virtual ~JavaPolicies();
};

/** Java-specific enhancements to the domain list view
  */
class JavaDomainListView : public DomainListView {
  Q_OBJECT
public:
  JavaDomainListView(KConfig *config,const QString &group,KJavaOptions *opt,
  		QWidget *parent,const char *name = 0);
  virtual ~JavaDomainListView();

  /** remnant for importing pre KDE 3.2 settings
    */
  void updateDomainListLegacy(const QStringList &domainConfig);

protected:
  virtual JavaPolicies *createPolicies();
  virtual JavaPolicies *copyPolicies(Policies *pol);
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private:
  QString group;
  KJavaOptions *options;
};

class KJavaOptions : public KCModule
{
    Q_OBJECT

public:
    KJavaOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );

    virtual void load();
    virtual void save();
    virtual void defaults();

    bool _removeJavaScriptDomainAdvice;

private slots:
    void slotChanged();
    void toggleJavaControls();

private:

    KConfig* m_pConfig;
    QString  m_groupname;
    JavaPolicies java_global_policies;

    QCheckBox*     enableJavaGloballyCB;
    QCheckBox*     javaSecurityManagerCB;
    QCheckBox*     useKioCB;
    QCheckBox*     enableShutdownCB;
    KIntNumInput*  serverTimeoutSB;
    QLineEdit*     addArgED;
    KURLRequester* pathED;
    bool           _removeJavaDomainSettings;

    JavaDomainListView *domainSpecific;

    friend class JavaDomainListView;
};

#endif		// __HTML_OPTIONS_H__

