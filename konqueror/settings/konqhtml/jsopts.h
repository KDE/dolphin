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
#include <qmap.h>

#include "jspolicies.h"

class KColorButton;
class KConfig;
class KListView;
class KURLRequester;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QListViewItem;
class QRadioButton;
class QSpinBox;
class QButtonGroup;

class PolicyDialog;

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
  void importPressed();
  void exportPressed();
  void addPressed();
  void changePressed();
  void deletePressed();

private:
  void setupPolicyDlg(PolicyDialog &,JSPolicies &copy);
  void changeJavaScriptEnabled();
  void updateDomainList(const QStringList &domainConfig);
  void updateDomainListLegacy(const QStringList &domainConfig); // old format

  KConfig *m_pConfig;
  QString m_groupname;
  JSPolicies js_global_policies;
  QCheckBox *enableJavaScriptGloballyCB;
  QCheckBox *enableJavaScriptDebugCB;
  JSPoliciesFrame *js_policies_frame;
  KListView* domainSpecificLV;
  bool _removeECMADomainSettings;

  typedef QMap<QListViewItem*, JSPolicies> DomainPolicyMap;
  DomainPolicyMap javaScriptDomainPolicy;
};


#endif		// __HTML_OPTIONS_H__

