/**
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvalidator.h>

#include <klineedit.h>
#include <kcombobox.h>
#include <klocale.h>

#include "policydlg.h"
#include "policydlg_ui.h"


class DomainLineValidator : public QValidator
{
public:
  DomainLineValidator(QObject *parent)
  :QValidator(parent, "domainValidator")
  {
  }

  State validate(QString &input, int &) const
  {
    if (input.isEmpty() || (input == "."))
      return Intermediate;
    
    int length = input.length();
        
    for(int i = 0 ; i < length; i++)
    {
      if (!input[i].isLetterOrNumber() && input[i] != '.' && input[i] != '-')
        return Invalid;
    }

    return Acceptable;
  }
};


PolicyDlg::PolicyDlg (const QString& caption, QWidget *parent,
                                    const char *name)
                 :KDialog(parent, name, true)
{
  setCaption( caption );

  QVBoxLayout* mainLayout = new QVBoxLayout(this, 0, 0);

  dlg = new PolicyDlgUI (this);
  mainLayout->addWidget(dlg);

  dlg->leDomain->setValidator(new DomainLineValidator(dlg->leDomain));

  QString wstr = i18n("Enter the host or domain to which this policy applies, "
                      "e.g. <i>www.kde.org</i> or <i>.kde.org</i>");
  QWhatsThis::add( dlg->leDomain, wstr );

  dlg->cbPolicy->setMinimumWidth( dlg->cbPolicy->fontMetrics().width('W') * 25 );
  wstr = i18n("Select the desired policy:"
              "<ul><li><b>Accept</b> - Allows this site to set cookie</li>"
              "<li><b>Reject</b> - Refuse all cookies sent from this site</li>"
              "<li><b>Ask</b> - Prompt when cookies are received from this site</li></ul>");
  QWhatsThis::add( dlg->cbPolicy, wstr );

  dlg->cbPolicy->clear ();
  dlg->cbPolicy->insertItem (i18n("Accept"));
  dlg->cbPolicy->insertItem (i18n("Reject"));
  dlg->cbPolicy->insertItem (i18n("Ask"));

  connect (dlg->pbOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect (dlg->pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(dlg->leDomain, SIGNAL(textChanged(const QString&)), SLOT(slotTextChanged(const QString&)));

  setFixedSize (sizeHint());
  dlg->leDomain->setFocus ();
}

void PolicyDlg::setEnableHostEdit( bool state, const QString& host )
{
  if ( !host.isEmpty() )
    dlg->leDomain->setText( host );
  dlg->leDomain->setEnabled( state );
}

void PolicyDlg::setPolicy (int policy)
{
  if ( policy > -1 && policy < static_cast<int>(dlg->cbPolicy->count()) )
    dlg->cbPolicy->setCurrentItem(policy-1);

  if ( !dlg->leDomain->isEnabled() )
    dlg->cbPolicy->setFocus();
}

int PolicyDlg::advice () const
{
  return dlg->cbPolicy->currentItem() + 1;
}

QString PolicyDlg::domain () const
{
  return dlg->leDomain->text();
}

void PolicyDlg::keyPressEvent( QKeyEvent* e )
{
  int key = e->key();
  if ( key == Qt::Key_Escape )
  {
    e->accept();
    dlg->pbCancel->animateClick();
  }
  KDialog::keyPressEvent( e );
}

void PolicyDlg::slotTextChanged( const QString& text )
{
  dlg->pbOK->setEnabled( text.length() > 1 );
}
#include "policydlg.moc"
