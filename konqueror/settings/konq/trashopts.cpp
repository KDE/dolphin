/* This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//
//
// "Trash" configuration
//
// (c) David Faure 2000

#include <qlabel.h>
#include <qbuttongroup.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

#include "trashopts.h"

#include <kapp.h>
#include <kdialog.h>
#include <konq_defaults.h> // include default values directly from libkonq
#include <klocale.h>
#include <kconfig.h>

//-----------------------------------------------------------------------------

KTrashOptions::KTrashOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule( parent, name ), g_pConfig(config), groupname(group)
{
    QGridLayout *lay = new QGridLayout(this, 2 /* rows */, 1/*2*/,
                                       KDialog::marginHint(),
                                       KDialog::spacingHint());
    lay->setRowStretch(1,1); // last row
//    lay->setColStretch(1,1); // last col

    QButtonGroup *bg = new QButtonGroup( i18n("Ask confirmation for:"), this );
    bg->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, bg->sizePolicy().hasHeightForWidth()) );
    bg->setColumnLayout(0, Qt::Vertical );
    bg->layout()->setSpacing( 0 );
    bg->layout()->setMargin( 0 );
    QVBoxLayout *bgLay = new QVBoxLayout( bg->layout() );
    bgLay->setAlignment( Qt::AlignTop );
    bgLay->setSpacing( KDialog::spacingHint() );
    bgLay->setMargin( KDialog::marginHint() );
    QWhatsThis::add( bg, i18n("This option tells Konqueror whether to ask"
       " for a confirmation when you \"delete\" a file."
       " <ul><li><em>Move To Trash:</em> moves the file to your trash directory,"
       " from where it can be recovered very easily.</li>"
       " <li><em>Delete:</em> simply deletes the file.</li>"
       " <li><em>Shred:</em> not only deletes the file, but overwrites"
       " the area on the disk where the file is stored, making recovery impossible."
       " You should not remove confirmation for this method unless you routinely work"
       " with very confidential information.</li></ul>") );

    connect(bg, SIGNAL( clicked( int ) ), SLOT( changed() ));
    connect(bg, SIGNAL( clicked( int ) ), SLOT( slotDeleteBehaviourChanged( int ) ));

//    bgLay->addSpacing(10);

    cbMoveToTrash = new QCheckBox( i18n("Move To Trash"), bg );
    bgLay->addWidget(cbMoveToTrash);

    cbDelete = new QCheckBox( i18n("Delete"), bg );
    bgLay->addWidget(cbDelete);

    cbShred = new QCheckBox( i18n("Shred"), bg );
    bgLay->addWidget(cbShred);

    lay->addWidget(bg, 0, 0);

    load();
}

void KTrashOptions::load()
{
    // *** load and apply to GUI ***
    g_pConfig->setGroup( groupname );
    cbMoveToTrash->setChecked( g_pConfig->readBoolEntry("ConfirmTrash", DEFAULT_CONFIRMTRASH) );
    cbDelete->setChecked( g_pConfig->readBoolEntry("ConfirmDelete", DEFAULT_CONFIRMDELETE) );
    cbShred->setChecked( g_pConfig->readBoolEntry("ConfirmShred", DEFAULT_CONFIRMSHRED) );
}

void KTrashOptions::defaults()
{
    cbMoveToTrash->setChecked( DEFAULT_CONFIRMTRASH );
    cbDelete->setChecked( DEFAULT_CONFIRMDELETE );
    cbShred->setChecked( DEFAULT_CONFIRMSHRED );
}

void KTrashOptions::save()
{
    g_pConfig->setGroup( groupname );
    g_pConfig->writeEntry( "ConfirmTrash", cbMoveToTrash->isChecked());
    g_pConfig->writeEntry( "ConfirmDelete", cbDelete->isChecked());
    g_pConfig->writeEntry( "ConfirmShred", cbShred->isChecked());
    g_pConfig->sync();
}

QString KTrashOptions::quickHelp() const
{
    return i18n("<h1>Trash Options</h1> Here you can modify the behavior "
                "of Konqueror when you want to delete a file."
                "<h2>On delete:</h2>This option determines what Konqueror "
                "will do with a file you chose to delete (e.g. in a context menu).<ul>"
                "<li><em>Move To Trash</em> will move the file to the trash folder, "
                "instead of deleting it, so you can easily recover it.</li>"
                "<li><em>Delete</em> will simply delete the file.</li>"
                "<li><em>Shred</em> will not only delete the file, but will first "
                "overwrite it with different bit patterns. This makes recovery impossible. "
                "Use it, if you're keeping very sensitive data."
                "<h2>Confirm destructive actions</h2>Check this box if you want Konqueror "
                "to ask \"Are you sure?\" before doing any destructive action (e.g. delete or shred).");
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
