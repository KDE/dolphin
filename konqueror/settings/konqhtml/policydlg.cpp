// (C) < 2002 to whoever created and edited this file before
// (C) 2002 Leo Savernik <l.savernik@aon.at>
//	Generalizing the policy dialog

#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qcombobox.h>

#include <klocale.h>
#include <kbuttonbox.h>
#include <kmessagebox.h>

#include <qpushbutton.h>

#include "policydlg.h"
#include "policies.h"

PolicyDialog::PolicyDialog( Policies *policies, QWidget *parent, const char *name )
             :KDialog(parent, name, true), policies(policies)
{
  insertIdx = 1;	// index where to insert additional panels
  topl = new QVBoxLayout(this, marginHint(), spacingHint());

  QGridLayout *grid = new QGridLayout(2, 2);
  grid->setColStretch(1, 1);
  topl->addLayout(grid);

  QLabel *l = new QLabel(i18n("&Host or domain name:"), this);
  grid->addWidget(l, 0, 0);

  le_domain = new QLineEdit(this);
  l->setBuddy( le_domain );
  grid->addWidget(le_domain, 0, 1);
  connect( le_domain,SIGNAL(textChanged ( const QString & )),this,SLOT(slotTextChanged( const QString &)));

  QWhatsThis::add(le_domain, i18n("Enter the name of a host (like www.kde.org) "
                                  "or a domain, starting with a dot (like .kde.org or .org)") );

#if 0
  l_javapolicy = new QLabel(i18n("&Java policy:"), this);
  grid->addWidget(l_javapolicy, 1, 0);

  cb_javapolicy = new QComboBox(this);
  l_javapolicy->setBuddy( cb_javapolicy );
  QStringList policies;
  policies << i18n( "Accept" ) << i18n( "Reject" );
  cb_javapolicy->insertStringList( policies );
  grid->addWidget(cb_javapolicy, 1, 1);

  QWhatsThis::add(cb_javapolicy, i18n("Select a Java policy for "
                                    "the above host or domain.") );


  l_javascriptpolicy = new QLabel(i18n("JavaScript policy:"), this);
  grid->addWidget(l_javascriptpolicy, 2, 0);

  cb_javascriptpolicy = new QComboBox(this);
  cb_javascriptpolicy->insertStringList( policies );
  l->setBuddy( cb_javascriptpolicy );
  grid->addWidget(cb_javascriptpolicy, 2, 1);

  QWhatsThis::add(cb_javascriptpolicy, i18n("Select a JavaScript policy for "
                                          "the above host or domain.") );
#endif

  l_feature_policy = new QLabel(this);
  grid->addWidget(l_feature_policy, 1, 0);

  cb_feature_policy = new QComboBox(this);
  l_feature_policy->setBuddy( cb_feature_policy );
  policy_values << i18n("Use Global") << i18n("Accept") << i18n("Reject");
  cb_feature_policy->insertStringList(policy_values);
  grid->addWidget(cb_feature_policy, 1, 1);

  KButtonBox *bbox = new KButtonBox(this);
  topl->addWidget(bbox);

  bbox->addStretch(1);
  okButton = bbox->addButton(i18n("&OK"));
  okButton->setDefault(true);
  connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));


  QPushButton *cancelButton = bbox->addButton(i18n("&Cancel"));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  le_domain->setFocus();
  okButton->setEnabled( !le_domain->text().isEmpty());
}

PolicyDialog::FeatureEnabledPolicy PolicyDialog::featureEnabledPolicy() const {
    return (FeatureEnabledPolicy)cb_feature_policy->currentItem();
}

void PolicyDialog::slotTextChanged( const QString &text)
{
    okButton->setEnabled( !text.isEmpty());
}

void PolicyDialog::setDisableEdit( bool state, const QString& text )
{
    if( text.isNull() );
        le_domain->setText( text );

    le_domain->setEnabled( state );

    if( state )
        cb_feature_policy->setFocus();
}

void PolicyDialog::refresh() {
    FeatureEnabledPolicy pol;

    if (policies->isFeatureEnabledPolicyInherited())
      pol = InheritGlobal;
    else if (policies->isFeatureEnabled())
      pol = Accept;
    else
      pol = Reject;
    cb_feature_policy->setCurrentItem(pol);
}

void PolicyDialog::setFeatureEnabledLabel(const QString &text) {
  l_feature_policy->setText(text);
}

void PolicyDialog::setFeatureEnabledWhatsThis(const QString &text) {
  QWhatsThis::add(cb_feature_policy, text);
}

void PolicyDialog::addPolicyPanel(QWidget *panel) {
  topl->insertWidget(insertIdx++,panel);
}

QString PolicyDialog::featureEnabledPolicyText() const {
  int pol = cb_feature_policy->currentItem();
  if (pol >= 0 && pol < 3) // Keep in sync with FeatureEnabledPolicy
    return policy_values[pol];
  else
    return QString::null;
}

void PolicyDialog::accept()
{
    if( le_domain->text().isEmpty() )
    {
        KMessageBox::information( 0, i18n("You must first enter a domain name!") );
        return;
    }

    FeatureEnabledPolicy pol = (FeatureEnabledPolicy)
    		cb_feature_policy->currentItem();
    if (pol == InheritGlobal) {
      policies->inheritFeatureEnabledPolicy();
    } else if (pol == Reject) {
      policies->setFeatureEnabled(false);
    } else {
      policies->setFeatureEnabled(true);
    }
    QDialog::accept();
}

#include "policydlg.moc"
