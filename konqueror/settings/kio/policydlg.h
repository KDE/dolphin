#ifndef _POLICYDLG_H
#define _POLICYDLG_H

#include <qstring.h>
#include <qlineedit.h>
#include <qcombobox.h>

#include <kdialog.h>

class PolicyDialog : public KDialog
{
    Q_OBJECT
    
public:
    PolicyDialog( QWidget *parent = 0, const char *name = 0 );
    ~PolicyDialog() {};
    
    /*
    * @return 1 for "Accept", 2 for "Reject" and 3 for "Ask"
    */
    int policyAdvice() const { return cb_policy->currentItem() + 1; }
    
    /*
    * @return the hostname for whom the policy is being set
    */
    QString domain() const { return le_domain->text(); }
    
    /*
    * Sets the line-edit to be enabled/disabled.
    *
    * This method will set the text in the lineedit if the
    * value is not null.
    *
    * @param state @p true to enable the line-edit, otherwise disabled.
    * @param text  the text to be set in the line-edit. Default is NULL.
    */
    void setDisableEdit( bool /*state*/, const QString& text = QString::null );
    
    /*
    * Sets the default cookie policy.
    *
    * @param value 0 - Accpet, 1 - Reject, 2 - Ask
    */    
    void setDefaultPolicy( int /*value*/ );

protected slots:

    virtual void accept();

private:

    QLineEdit *le_domain;
    QComboBox *cb_policy;
};

#endif
