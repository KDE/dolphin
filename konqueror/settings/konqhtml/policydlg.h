// (C) < 2002 to whoever created and edited this file before
// (C) 2002 Leo Savernik <l.savernik@aon.at>
//	Generalizing the policy dialog

#ifndef _POLICYDLG_H
#define _POLICYDLG_H

#include <kdialogbase.h>

#include <QLineEdit>
#include <QStringList>

class QLabel;
class QComboBox;
class QString;
class QVBoxLayout;
class QPushButton;

class Policies;

/**
 * A dialog for editing domain-specific policies.
 *
 * Each dialog must be associated with a relevant Policies instance which
 * will be updated within this dialog appropriately.
 *
 * Additionally you can insert your own widget containing controls for
 * specific policies with addPolicyPanel.
 *
 * @author unknown
 */
class PolicyDialog : public KDialogBase
{
    Q_OBJECT

public:
    /**
     * Enumerates the possible return values for the "feature enabled"
     * policy
     */
    enum FeatureEnabledPolicy { InheritGlobal = 0, Accept, Reject };

    /** constructor
     * @param policies policies object this dialog will write the settings
     *		into. Note that it always reflects the current settings,
     *		even if the dialog has been canceled.
     * @param parent parent widget this belongs to
     * @param name internal name
     */
    PolicyDialog(Policies *policies, QWidget *parent = 0, const char *name = 0 );

    virtual ~PolicyDialog() {};

    /*
    * @return whether this feature should be activated, deactivated or
    *	inherited from the respective global policy.
    */
    FeatureEnabledPolicy featureEnabledPolicy() const;

    /**
     * @return the textual representation of the current "feature enabled"
     * policy
     */
    QString featureEnabledPolicyText() const;

    /*
    * @return the hostname for which the policy is being set
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
    void setDisableEdit( bool /*state*/, const QString& text = QString() );

    /**
     * Sets the label for the "feature enabled" policy
     * @param text label text
     */
    void setFeatureEnabledLabel(const QString &text);

    /**
     * Sets the "What's This" text for the "feature enabled" policy
     * combo box.
     * @param text what's-this text
     */
    void setFeatureEnabledWhatsThis(const QString &text);

    /**
     * Syncs the controls with the current content of the
     * associated policies object.
     */
    void refresh();

    /**
     * Adds another panel which contains controls for more policies.
     *
     * The widget is inserted between the "feature enabled" combo box and
     * the dialog buttons at the bottom.
     *
     * Currently at most one widget can be added.
     * @param panel pointer to widget to insert. The dialog takes ownership
     *		of it, but does not reparent it.
     */
    void addPolicyPanel(QWidget *panel);

protected Q_SLOTS:

    virtual void accept();
    void slotTextChanged( const QString &text);

private:
    Policies *policies;
    QVBoxLayout *topl;
    int insertIdx;
    QLineEdit *le_domain;
    QLabel *l_feature_policy;
    QComboBox *cb_feature_policy;
    QWidget *panel;
    QStringList policy_values;
    QPushButton *okButton;
};

#endif
