//
//
// "Trash" configuration
//
// (c) David Faure 2000

#include <qlabel.h>
#include <qbuttongroup.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qradiobutton.h>

#include "trashopts.h"

#include <kapp.h>
#include <kdialog.h>
#include <konqdefaults.h> // include default values directly from libkonq
#include <klocale.h>
#include <kconfig.h>

//-----------------------------------------------------------------------------

KTrashOptions::KTrashOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule( parent, name ), g_pConfig(config), groupname(group)
{
    QGridLayout *lay = new QGridLayout(this, 3, 2,
                                       KDialog::marginHint(),
                                       KDialog::spacingHint());
    lay->setRowStretch(2,1); // last row
    lay->setColStretch(1,1); // last col

    QButtonGroup *bg = new QButtonGroup( i18n("On delete:"), this );
    QVBoxLayout *bgLay = new QVBoxLayout(bg, KDialog::marginHint(),
				       KDialog::spacingHint());
    bg->setExclusive( TRUE );
    connect(bg, SIGNAL( clicked( int ) ), SLOT( changed() ));
    connect(bg, SIGNAL( clicked( int ) ), SLOT( slotDeleteBehaviourChanged( int ) ));

    bgLay->addSpacing(10);

    rbMoveToTrash = new QRadioButton( i18n("Move To Trash"), bg );
    bgLay->addWidget(rbMoveToTrash);

    rbDelete = new QRadioButton( i18n("Delete"), bg );
    bgLay->addWidget(rbDelete);

    rbShred = new QRadioButton( i18n("Shred"), bg );
    bgLay->addWidget(rbShred);

    lay->addWidget(bg, 0, 0);

    m_pConfirmDestructive = new QCheckBox(i18n("Confirm destructive actions"),
					  this);
    connect(m_pConfirmDestructive, SIGNAL(clicked()), this, SLOT(changed()));
    lay->addWidget(m_pConfirmDestructive, 1, 0);

    load();
}

void KTrashOptions::load()
{
    // *** load and apply to GUI ***
    g_pConfig->setGroup( groupname );
    deleteAction = g_pConfig->readNumEntry("DeleteAction", DEFAULT_DELETEACTION);
    rbMoveToTrash->setChecked( deleteAction == 1 );
    rbDelete->setChecked( deleteAction == 2 );
    rbShred->setChecked( deleteAction == 3 );
    m_pConfirmDestructive->setChecked(g_pConfig->readBoolEntry("ConfirmDestructive", true));
}

void KTrashOptions::defaults()
{
    rbMoveToTrash->setChecked( DEFAULT_DELETEACTION == 1 );
    rbDelete->setChecked( DEFAULT_DELETEACTION == 2 );
    rbShred->setChecked( DEFAULT_DELETEACTION == 3 );
    m_pConfirmDestructive->setChecked(true);
}

void KTrashOptions::save()
{
    g_pConfig->setGroup( groupname );
    g_pConfig->writeEntry( "DeleteAction", deleteAction );
    g_pConfig->writeEntry( "ConfirmDestructive", m_pConfirmDestructive->isChecked());
    g_pConfig->sync();
}

void KTrashOptions::slotDeleteBehaviourChanged( int b )
{
    deleteAction = b + 1;
}

void KTrashOptions::changed()
{
    emit KCModule::changed(true);
}

#include "trashopts.moc"
