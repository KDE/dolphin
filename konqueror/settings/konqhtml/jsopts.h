//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef __JSOPTS_H__
#define __JSOPTS_H__

#include <kcmodule.h>

#include "domainlistview.h"
#include "jspolicies.h"

class KColorButton;
class KConfig;
class KURLRequester;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QListViewItem;
class QRadioButton;
class QSpinBox;
class QButtonGroup;

class PolicyDialog;

/** JavaScript-specific enhancements to the domain list view
  */
class JSDomainListView : public DomainListView {
  Q_OBJECT
public:
  JSDomainListView(KConfig *config,const QString &group,QWidget *parent,
  		const char *name = 0);
  virtual ~JSDomainListView();

  /** remnant for importing pre KDE 3.2 settings
    */
  void updateDomainListLegacy(const QStringList &domainConfig);

protected:
  virtual JSPolicies *createPolicies();
  virtual JSPolicies *copyPolicies(Policies *pol);
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private:
  QString group;
};

class KJavaScriptOptions : public KCModule
{
  Q_OBJECT
public:
  KJavaScriptOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );

  virtual void load();
  virtual void save();
  virtual void defaults();

  bool _removeJavaScriptDomainAdvice;

private slots:
  void slotChanged();
  void slotChangeJSEnabled();

private:

  KConfig *m_pConfig;
  QString m_groupname;
  JSPolicies js_global_policies;
  QCheckBox *enableJavaScriptGloballyCB;
  QCheckBox *enableJavaScriptDebugCB;
  JSPoliciesFrame *js_policies_frame;
  bool _removeECMADomainSettings;

  JSDomainListView* domainSpecific;
};

#endif		// __JSOPTS_H__

