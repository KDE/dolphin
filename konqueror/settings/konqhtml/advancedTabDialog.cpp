/*
 *  advancedTabDialog.cpp
 *
 *  Copyright (c) 2002 Aaron J. Seigo <aseigo@olympusproject.org>
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
 */

#include <QButtonGroup>
#include <QCheckBox>
#include <QLayout>
#include <kpushbutton.h>
#include <QRadioButton>
#include <QSlider>

#include <kapplication.h>
#include <kcolorbutton.h>
#include <klocale.h>
#include <kconfig.h>

#include "advancedTabDialog.h"
#include "main.h"

advancedTabDialog::advancedTabDialog(QWidget* parent, KSharedConfig::Ptr config, const char* name)
    : KDialog( parent ),
                  m_pConfig(config)
{
    setCaption( i18n("Advanced Options") );
    setObjectName( name );
    setModal( true );
    showButtonSeparator( true );

    connect(this, SIGNAL(applyClicked()),
            this, SLOT(save()));
    connect(this, SIGNAL(okClicked()),
            this, SLOT(save()));
    enableButton(Apply, false);

    QFrame* page = new QFrame( this );
    setMainWidget( page );
    QVBoxLayout* layout = new QVBoxLayout(page);
    m_advancedWidget = new advancedTabOptions(page);
    layout->addWidget(m_advancedWidget);
    layout->addSpacing( 20 );
    layout->addStretch();

    connect(m_advancedWidget->m_pNewTabsInBackground, SIGNAL(clicked()), this, SLOT(changed()));
    connect(m_advancedWidget->m_pOpenAfterCurrentPage, SIGNAL(clicked()), this, SLOT(changed()));
    connect(m_advancedWidget->m_pTabConfirm, SIGNAL(clicked()), this, SLOT(changed()));
    connect(m_advancedWidget->m_pTabCloseActivatePrevious, SIGNAL(clicked()), this, SLOT(changed()));
    connect(m_advancedWidget->m_pPermanentCloseButton, SIGNAL(clicked()), this, SLOT(changed()));
    connect(m_advancedWidget->m_pKonquerorTabforExternalURL, SIGNAL(clicked()), this, SLOT(changed()));
    connect(m_advancedWidget->m_pPopupsWithinTabs, SIGNAL(clicked()), this, SLOT(changed()));

    load();
}

advancedTabDialog::~advancedTabDialog()
{
}

void advancedTabDialog::load()
{
    m_pConfig->setGroup("FMSettings");
    m_advancedWidget->m_pNewTabsInBackground->setChecked( ! (m_pConfig->readEntry( "NewTabsInFront", QVariant(false )).toBool()) );
    m_advancedWidget->m_pOpenAfterCurrentPage->setChecked( m_pConfig->readEntry( "OpenAfterCurrentPage", QVariant(false )).toBool() );
    m_advancedWidget->m_pPermanentCloseButton->setChecked( m_pConfig->readEntry( "PermanentCloseButton", QVariant(false )).toBool() );
    m_advancedWidget->m_pKonquerorTabforExternalURL->setChecked( m_pConfig->readEntry( "KonquerorTabforExternalURL", QVariant(false )).toBool() );
    m_advancedWidget->m_pPopupsWithinTabs->setChecked( m_pConfig->readEntry( "PopupsWithinTabs", QVariant(false )).toBool() );
    m_advancedWidget->m_pTabCloseActivatePrevious->setChecked( m_pConfig->readEntry( "TabCloseActivatePrevious", QVariant(false )).toBool() );

    m_pConfig->setGroup("Notification Messages");
    m_advancedWidget->m_pTabConfirm->setChecked( !m_pConfig->hasKey("MultipleTabConfirm") );

    enableButton(Apply, false);
}

void advancedTabDialog::save()
{
    m_pConfig->setGroup("FMSettings");
    m_pConfig->writeEntry( "NewTabsInFront", !(m_advancedWidget->m_pNewTabsInBackground->isChecked()) );
    m_pConfig->writeEntry( "OpenAfterCurrentPage", m_advancedWidget->m_pOpenAfterCurrentPage->isChecked() );
    m_pConfig->writeEntry( "PermanentCloseButton", m_advancedWidget->m_pPermanentCloseButton->isChecked() );
    m_pConfig->writeEntry( "KonquerorTabforExternalURL", m_advancedWidget->m_pKonquerorTabforExternalURL->isChecked() );
    m_pConfig->writeEntry( "PopupsWithinTabs", m_advancedWidget->m_pPopupsWithinTabs->isChecked() );
    m_pConfig->writeEntry( "TabCloseActivatePrevious", m_advancedWidget->m_pTabCloseActivatePrevious->isChecked() );
    m_pConfig->sync();

    // It only matters wether the key is present, its value has no meaning
    m_pConfig->setGroup("Notification Messages");
    if ( m_advancedWidget->m_pTabConfirm->isChecked() ) m_pConfig->deleteEntry( "MultipleTabConfirm" );
    else m_pConfig->writeEntry( "MultipleTabConfirm", true );

#warning "kde4: port konqueror* dcop call"
#if 0
    KApplication::kApplication()->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );
#endif
    enableButton(Apply, false);
}

void advancedTabDialog::changed()
{
    enableButton(Apply, true);
}

#include "advancedTabDialog.moc"
