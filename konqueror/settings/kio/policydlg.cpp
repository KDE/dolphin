
#include <qstringlist.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>

#include <kglobal.h>
#include <klocale.h>
#include <kbuttonbox.h>
#include <kmessagebox.h>

#include "policydlg.h"

PolicyDialog::PolicyDialog( QWidget *parent, const char *name )
             :KDialog(parent, name, true)
{
  QVBoxLayout *topl = new QVBoxLayout(this, marginHint(), spacingHint());

  QGridLayout *grid = new QGridLayout(2, 2);
  grid->setColStretch(1, 1);
  topl->addLayout(grid);

  QLabel *l = new QLabel(i18n("Host name:"), this);
  grid->addWidget(l, 0, 0);

  le_domain = new QLineEdit(this);
  grid->addWidget(le_domain, 0, 1);
  
  l = new QLabel(i18n("Policy:"), this);
  grid->addWidget(l, 1, 0);

  cb_policy = new QComboBox(this);
  QStringList policies;
  policies << i18n( "Accept" ) << i18n( "Reject" ) << i18n( "Ask" );
  cb_policy->insertStringList( policies );
  grid->addWidget(cb_policy, 1, 1);

  QWhatsThis::add(cb_policy, i18n("Select a cookie policy for "
                                     "the above domain name.") );

  KButtonBox *bbox = new KButtonBox(this);
  topl->addWidget(bbox);
  
  bbox->addStretch(1);
  QPushButton *okButton = bbox->addButton(i18n("OK"));
  okButton->setDefault(true);
  connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

  QPushButton *cancelButton = bbox->addButton(i18n("Cancel"));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));  

  le_domain->setFocus();
}

void PolicyDialog::setDisableEdit( bool state, const QString& text )
{
    if( text.isNull() );
        le_domain->setText( text );
        
    le_domain->setEnabled( state );
    
    if( !state )
        cb_policy->setFocus();
}

void PolicyDialog::setDefaultPolicy( int value )
{
    if( value > -1 && value < (int) cb_policy->count() )
    {
        cb_policy->setCurrentItem( value );
    }
}

void PolicyDialog::accept()
{
    if( le_domain->text().isEmpty() )
    {
        KMessageBox::information( 0, i18n("You must first enter a domain name!") );
        return;
    }
    QDialog::accept();    
}

#include "policydlg.moc"
