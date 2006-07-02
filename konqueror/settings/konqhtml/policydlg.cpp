// (C) < 2002 to whoever created and edited this file before
// (C) 2002 Leo Savernik <l.savernik@aon.at>
//	Generalizing the policy dialog

#include <QLayout>
#include <QLabel>
#include <QComboBox>

#include <klocale.h>
#include <kbuttonbox.h>
#include <kmessagebox.h>

#include <QPushButton>

#include "policydlg.h"
#include "policies.h"

PolicyDialog::PolicyDialog( Policies *policies, QWidget *parent, const char *name )
    : KDialog(parent),
      policies(policies)
{
  setObjectName( name );
  setModal( true );
  setButtons( Ok|Cancel );
  showButtonSeparator( true );

  QFrame *main = new QFrame( this );
  setMainWidget( main );

  insertIdx = 1;	// index where to insert additional panels
  topl = new QVBoxLayout(main);
  topl->setMargin(0);
  topl->setSpacing(spacingHint());

  QGridLayout *grid = new QGridLayout();
  topl->addItem( grid );
  grid->setColumnStretch(1, 1);

  QLabel *l = new QLabel(i18n("&Host or domain name:"), main);
  grid->addWidget(l, 0, 0);

  le_domain = new QLineEdit(main);
  l->setBuddy( le_domain );
  grid->addWidget(le_domain, 0, 1);
  connect( le_domain,SIGNAL(textChanged( const QString & )),
      SLOT(slotTextChanged( const QString &)));

  le_domain->setWhatsThis( i18n("Enter the name of a host (like www.kde.org) "
                                  "or a domain, starting with a dot (like .kde.org or .org)") );

  l_feature_policy = new QLabel(main);
  grid->addWidget(l_feature_policy, 1, 0);

  cb_feature_policy = new QComboBox(main);
  l_feature_policy->setBuddy( cb_feature_policy );
  policy_values << i18n("Use Global") << i18n("Accept") << i18n("Reject");
  cb_feature_policy->addItems(policy_values);
  grid->addWidget(cb_feature_policy, 1, 1);

  le_domain->setFocus();

  enableButtonOk(!le_domain->text().isEmpty());
}

PolicyDialog::FeatureEnabledPolicy PolicyDialog::featureEnabledPolicy() const {
    return (FeatureEnabledPolicy)cb_feature_policy->currentIndex();
}

void PolicyDialog::slotTextChanged( const QString &text)
{
    enableButtonOk(!text.isEmpty());
}

void PolicyDialog::setDisableEdit( bool state, const QString& text )
{
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
    cb_feature_policy->setCurrentIndex(pol);
}

void PolicyDialog::setFeatureEnabledLabel(const QString &text) {
  l_feature_policy->setText(text);
}

void PolicyDialog::setFeatureEnabledWhatsThis(const QString &text) {
  cb_feature_policy->setWhatsThis(text);
}

void PolicyDialog::addPolicyPanel(QWidget *panel) {
  topl->insertWidget(insertIdx++,panel);
}

QString PolicyDialog::featureEnabledPolicyText() const {
  int pol = cb_feature_policy->currentIndex();
  if (pol >= 0 && pol < 3) // Keep in sync with FeatureEnabledPolicy
    return policy_values[pol];
  else
    return QString();
}

void PolicyDialog::accept()
{
    if( le_domain->text().isEmpty() )
    {
        KMessageBox::information( 0, i18n("You must first enter a domain name.") );
        return;
    }

    FeatureEnabledPolicy pol = (FeatureEnabledPolicy)
    		cb_feature_policy->currentIndex();
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
