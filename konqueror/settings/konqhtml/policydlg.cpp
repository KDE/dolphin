
#include <qstringlist.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qlineedit.h>
#include <qcombobox.h>

#include <kglobal.h>
#include <klocale.h>
#include <kbuttonbox.h>
#include <kmessagebox.h>

#include "policydlg.h"

PolicyDialog::PolicyDialog( bool javascript, bool java, QWidget *parent, const char *name )
             :KDialog(parent, name, true)
{
  QVBoxLayout *topl = new QVBoxLayout(this, marginHint(), spacingHint());

  QGridLayout *grid = new QGridLayout(3, 2);
  grid->setColStretch(1, 1);
  topl->addLayout(grid);

  QLabel *l = new QLabel(i18n("&Host or domain name:"), this);
  grid->addWidget(l, 0, 0);

  le_domain = new QLineEdit(this);
  l->setBuddy( le_domain );
  grid->addWidget(le_domain, 0, 1);

  QWhatsThis::add(le_domain, i18n("Enter the name of a host (like www.kde.org) "
                                  "or a domain, starting with a dot (like .kde.org or .org)") );


  l_javapolicy = new QLabel(i18n("Java policy:"), this);
  grid->addWidget(l_javapolicy, 1, 0);

  cb_javapolicy = new QComboBox(this);
  l->setBuddy( cb_javapolicy );
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


  KButtonBox *bbox = new KButtonBox(this);
  topl->addWidget(bbox);

  bbox->addStretch(1);
  QPushButton *okButton = bbox->addButton(i18n("OK"));
  okButton->setDefault(true);
  connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

  QPushButton *cancelButton = bbox->addButton(i18n("Cancel"));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  //now hide the parts not being used
  if( !java )
  {
    cb_javapolicy->hide();
    l_javapolicy->hide();
  }

  if( !javascript )
  {
    cb_javascriptpolicy->hide();
    l_javascriptpolicy->hide();
  }

  le_domain->setFocus();
}

void PolicyDialog::setDisableEdit( bool state, const QString& text )
{
    if( text.isNull() );
        le_domain->setText( text );

    le_domain->setEnabled( state );

    if( state )
        cb_javapolicy->setFocus();
}

void PolicyDialog::setDefaultPolicy( int javavalue, int javascriptvalue )
{
    if( javavalue > 0 && javavalue <= (int) cb_javapolicy->count() )
    {
        cb_javapolicy->setCurrentItem( javavalue-1 );
    }
    if( javascriptvalue > 0 && javascriptvalue <= (int) cb_javascriptpolicy->count() )
    {
        cb_javascriptpolicy->setCurrentItem( javascriptvalue-1 );
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
