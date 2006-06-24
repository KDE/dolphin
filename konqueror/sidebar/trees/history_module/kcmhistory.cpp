/*
 *  kcmhistory.cpp
 *  Copyright (c) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>
 *  Copyright (c) 2002 Stephan Binner <binner@kde.org>
 *
 *  based on kcmtaskbar.cpp
 *  Copyright (c) 2000 Kurt Granroth <granroth@kde.org>
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

#include <QCheckBox>
#include <QComboBox>
#include <QLayout>
#include <QPushButton>
#include <QRadioButton>

#include <kapplication.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kfontdialog.h>
#include <kgenericfactory.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include "history_dlg.h"

#include "konq_historymgr.h"

#include "kcmhistory.h"
#include "history_settings.h"

typedef KGenericFactory<HistorySidebarConfig, QWidget > KCMHistoryFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_history, KCMHistoryFactory("kcmhistory") )

HistorySidebarConfig::HistorySidebarConfig( QWidget *parent, const QStringList & )
    : KCModule (KCMHistoryFactory::instance(), parent, QStringList())
{
    KGlobal::locale()->insertCatalog("konqueror");

    m_settings = new KonqSidebarHistorySettings( this );
    m_settings->readSettings( false );

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());
    dialog = new KonqSidebarHistoryDlg(this);

    dialog->spinEntries->setRange( 0, INT_MAX );
    dialog->spinExpire->setRange(  0, INT_MAX );

    dialog->spinNewer->setRange( 0, INT_MAX );
    dialog->spinOlder->setRange( 0, INT_MAX );

    dialog->comboNewer->insertItem( KonqSidebarHistorySettings::MINUTES,
		                    i18n("Minutes") );
    dialog->comboNewer->insertItem( KonqSidebarHistorySettings::DAYS,
		                    i18n("Days") );

    dialog->comboOlder->insertItem( KonqSidebarHistorySettings::MINUTES,
		                    i18n("Minutes") );
    dialog->comboOlder->insertItem( KonqSidebarHistorySettings::DAYS,
		                    i18n("Days") );

    connect( dialog->cbExpire, SIGNAL( toggled( bool )),
	     dialog->spinExpire, SLOT( setEnabled( bool )));
    connect( dialog->spinExpire, SIGNAL( valueChanged( int )),
	     this, SLOT( slotExpireChanged( int )));

    connect( dialog->spinNewer, SIGNAL( valueChanged( int )),
	     SLOT( slotNewerChanged( int )));
    connect( dialog->spinOlder, SIGNAL( valueChanged( int )),
	     SLOT( slotOlderChanged( int )));

    connect( dialog->btnFontNewer, SIGNAL( clicked() ),
             SLOT( slotGetFontNewer() ));
    connect( dialog->btnFontOlder, SIGNAL( clicked() ),
             SLOT( slotGetFontOlder() ));
    connect( dialog->btnClearHistory, SIGNAL( clicked() ),
             SLOT( slotClearHistory() ));

    connect( dialog->cbDetailedTips, SIGNAL( toggled( bool )),
             SLOT( configChanged() ));
    connect( dialog->cbExpire, SIGNAL( toggled( bool )),
             SLOT( configChanged() ));
    connect( dialog->spinEntries, SIGNAL( valueChanged( int )),
             SLOT( configChanged() ));
    connect( dialog->comboNewer, SIGNAL( activated( int )),
             SLOT( configChanged() ));
    connect( dialog->comboOlder, SIGNAL( activated( int )),
             SLOT( configChanged() ));

    dialog->show();
    topLayout->addWidget(dialog);
    load();
}

void HistorySidebarConfig::configChanged()
{
    emit changed(true);
}

void HistorySidebarConfig::load()
{
    KConfig config("konquerorrc");
    config.setGroup("HistorySettings");
    dialog->spinExpire->setValue( config.readEntry( "Maximum age of History entries", 90) );
    dialog->spinEntries->setValue( config.readEntry( "Maximum of History entries", 500 ) );
    dialog->cbExpire->setChecked( dialog->spinExpire->value() > 0 );

    dialog->spinNewer->setValue( m_settings->m_valueYoungerThan );
    dialog->spinOlder->setValue( m_settings->m_valueOlderThan );

    dialog->comboNewer->setCurrentIndex( m_settings->m_metricYoungerThan );
    dialog->comboOlder->setCurrentIndex( m_settings->m_metricOlderThan );

    dialog->cbDetailedTips->setChecked( m_settings->m_detailedTips );

    m_fontNewer = m_settings->m_fontYoungerThan;
    m_fontOlder = m_settings->m_fontOlderThan;

    // enable/disable widgets
    dialog->spinExpire->setEnabled( dialog->cbExpire->isChecked() );

    slotExpireChanged( dialog->spinExpire->value() );
    slotNewerChanged( dialog->spinNewer->value() );
    slotOlderChanged( dialog->spinOlder->value() );

    emit changed(false);
}

void HistorySidebarConfig::save()
{
    quint32 age   = dialog->cbExpire->isChecked() ? dialog->spinExpire->value() : 0;
    quint32 count = dialog->spinEntries->value();

    KonqHistoryManager::kself()->emitSetMaxAge( age );
    KonqHistoryManager::kself()->emitSetMaxCount( count );

    m_settings->m_valueYoungerThan = dialog->spinNewer->value();
    m_settings->m_valueOlderThan   = dialog->spinOlder->value();

    m_settings->m_metricYoungerThan = dialog->comboNewer->currentIndex();
    m_settings->m_metricOlderThan   = dialog->comboOlder->currentIndex();

    m_settings->m_detailedTips = dialog->cbDetailedTips->isChecked();

    m_settings->m_fontYoungerThan = m_fontNewer;
    m_settings->m_fontOlderThan   = m_fontOlder;

    m_settings->applySettings();

    emit changed(false);
}

void HistorySidebarConfig::defaults()
{
    dialog->spinEntries->setValue( 500 );
    dialog->cbExpire->setChecked( true );
    dialog->spinExpire->setValue( 90 );

    dialog->spinNewer->setValue( 1 );
    dialog->spinOlder->setValue( 2 );

    dialog->comboNewer->setCurrentIndex( KonqSidebarHistorySettings::DAYS );
    dialog->comboOlder->setCurrentIndex( KonqSidebarHistorySettings::DAYS );

    dialog->cbDetailedTips->setChecked( true );

    m_fontNewer = QFont();
    m_fontNewer.setItalic( true );
    m_fontOlder = QFont();

    emit changed(true);
}

QString HistorySidebarConfig::quickHelp() const
{
    return i18n("<h1>History Sidebar</h1>"
                " You can configure the history sidebar here.");
}

void HistorySidebarConfig::slotExpireChanged( int value )
{
    dialog->spinExpire->setSuffix( i18np(" day", " days", value) );
    configChanged();
}

// change hour to days, minute to minutes and the other way round,
// depending on the value of the spinbox, and synchronize the two spinBoxes
// to enfore newer <= older.
void HistorySidebarConfig::slotNewerChanged( int value )
{
    dialog->comboNewer->setItemText( KonqSidebarHistorySettings::DAYS,
                                     i18np ( "Day", "Days", value) );
    dialog->comboNewer->setItemText( KonqSidebarHistorySettings::MINUTES,
		                     i18np ( "Minute", "Minutes", value) );

    if ( dialog->spinNewer->value() > dialog->spinOlder->value() )
	dialog->spinOlder->setValue( dialog->spinNewer->value() );
    configChanged();
}

void HistorySidebarConfig::slotOlderChanged( int value )
{
    dialog->comboOlder->setItemText( KonqSidebarHistorySettings::DAYS,
		                     i18np ( "Day", "Days", value) );
    dialog->comboOlder->setItemText( KonqSidebarHistorySettings::MINUTES,
		                     i18np ( "Minute", "Minutes", value) );

    if ( dialog->spinNewer->value() > dialog->spinOlder->value() )
	dialog->spinNewer->setValue( dialog->spinOlder->value() );

    configChanged();
}

void HistorySidebarConfig::slotGetFontNewer()
{
    int result = KFontDialog::getFont( m_fontNewer, false, this );
    if ( result == KFontDialog::Accepted )
        configChanged();
}

void HistorySidebarConfig::slotGetFontOlder()
{
    int result = KFontDialog::getFont( m_fontOlder, false, this );
    if ( result == KFontDialog::Accepted )
        configChanged();
}

void HistorySidebarConfig::slotClearHistory()
{
    KGuiItem guiitem = KStdGuiItem::clear();
    guiitem.setIcon( SmallIconSet("history_clear"));
    if ( KMessageBox::warningContinueCancel( this,
				     i18n("Do you really want to clear "
					  "the entire history?"),
				     i18n("Clear History?"), guiitem )
	 == KMessageBox::Continue ) {
        KonqHistoryManager::kself()->emitClear();
    }
}

#include "kcmhistory.moc"
