/*
   Copyright (C) 2008 by Sebastian Trueg <trueg at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "newtagdialog.h"

#include <nepomuk/tag.h>

#include <KDebug>
#include <KLocale>
#include <KTitleWidget>


NewTagDialog::NewTagDialog( QWidget* parent )
    : KDialog( parent )
{
    setCaption( i18nc( "@title:window", "Create new Tag" ) );
    setButtons( Ok|Cancel );
    enableButtonOk( false );

    setupUi( mainWidget() );

    connect( m_editTagLabel, SIGNAL( textChanged(const QString&) ),
             this, SLOT( slotLabelChanged(const QString&) ) );

    // TODO: use KGlobal::config() if NewTagDialog will be moved to kdelibs (KDE 4.2?)
    KConfigGroup group(KSharedConfig::openConfig("dolphinrc"), "NewTagDialog");
    restoreDialogSize(group);
}


NewTagDialog::~NewTagDialog()
{
    // TODO: use KGlobal::config() if NewTagDialog will be moved to kdelibs (KDE 4.2?)
    KConfigGroup group(KSharedConfig::openConfig("dolphinrc"), "NewTagDialog");
    saveDialogSize(group, KConfigBase::Persistent);
}


QSize NewTagDialog::sizeHint() const
{
    return QSize(400, 256);
}

void NewTagDialog::slotLabelChanged( const QString& text )
{
    enableButtonOk( !text.isEmpty() );
}


Nepomuk::Tag NewTagDialog::createTag( QWidget* parent )
{
    NewTagDialog dlg( parent );
    dlg.m_labelTitle->setText( i18nc( "@title:window", "Create New Tag" ) );
    dlg.m_labelTitle->setComment( i18nc( "@title:window subtitle to previous message", "with optional icon and description" ) );
    dlg.m_labelTitle->setPixmap( KIcon( "nepomuk" ).pixmap( 32, 32 ) );

    dlg.m_editTagLabel->setFocus();

    if ( dlg.exec() ) {
        QString name = dlg.m_editTagLabel->text();
        QString comment = dlg.m_editTagComment->text();
        QString icon = dlg.m_buttonTagIcon->icon();

        Nepomuk::Tag newTag( name );
        newTag.setLabel( name );
        newTag.addIdentifier( name );
        if ( !comment.isEmpty() ) {
            newTag.setDescription( comment );
        }
        if ( !icon.isEmpty() ) {
            newTag.addSymbol( icon );
        }
        return newTag;
    }
    else {
        return Nepomuk::Tag();
    }
}

#include "newtagdialog.moc"
