/*
 *  Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "kcmperformance.h"
#include "konqueror.h"
#include "system.h"

#include <qlayout.h>
#include <qtabwidget.h>
#include <klocale.h>
#include <kdialog.h>

extern "C"
{
KCModule* create_performance( QWidget* parent_P, const char* name_P )
    {
    return new KCMPerformance::Config( parent_P, name_P );
    }

KCModule* create_konqueror( QWidget* parent_P, const char* name_P )
    {
    return new KCMPerformance::KonquerorConfig( parent_P, name_P );
    }
}

namespace KCMPerformance
{

Config::Config( QWidget* parent_P, const char* )
    : KCModule( parent_P, "kcmperformance" )
    {
    QVBoxLayout *topLayout = new QVBoxLayout( this );
    QTabWidget* tabs = new QTabWidget( this );
    konqueror_widget = new Konqueror( tabs );
    konqueror_widget->layout()->setMargin( KDialog::marginHint() );
    connect( konqueror_widget, SIGNAL( changed()), SLOT( configChanged()));
    tabs->addTab( konqueror_widget, i18n( "Konqueror" ));
    system_widget = new SystemWidget( tabs );
    system_widget->layout()->setMargin( KDialog::marginHint() );
    connect( system_widget, SIGNAL( changed()), SLOT( configChanged()));
    tabs->addTab( system_widget, i18n( "System" ));
    topLayout->add( tabs );
    load();
    }

void Config::configChanged()
    {
    emit changed( true );
    }

void Config::load()
    {
    konqueror_widget->load();
    system_widget->load();
    emit changed( false );
    }

void Config::save()
    {
    konqueror_widget->save();
    system_widget->save();
    emit changed( false );
    }

void Config::defaults()
    {
    konqueror_widget->defaults();
    system_widget->defaults();
    emit changed( true );
    }

QString Config::quickHelp() const
    {
    return i18n( "<h1>KDE Performance</h1>"
        " You can configure settings that improve KDE performance here." );
    }


    
KonquerorConfig::KonquerorConfig( QWidget* parent_P, const char* )
    : KCModule( parent_P, "kcmperformance" )
    {
    QVBoxLayout *topLayout = new QVBoxLayout( this );
    widget = new Konqueror( this );
    connect( widget, SIGNAL( changed()), SLOT( configChanged()));
    topLayout->add( widget );
    load();
    }

void KonquerorConfig::configChanged()
    {
    emit changed( true );
    }

void KonquerorConfig::load()
    {
    widget->load();
    emit changed( false );
    }

void KonquerorConfig::save()
    {
    widget->save();
    emit changed( false );
    }

void KonquerorConfig::defaults()
    {
    widget->defaults();
    emit changed( true );
    }

QString KonquerorConfig::quickHelp() const
    {
    return i18n( "<h1>Konqueror Performance</h1>"
        " You can configure several settings that improve Konqueror performance here."
        " These include options for reusing already running instances"
        " and for keeping instances preloaded." );
    }

} // namespace

#include "kcmperformance.moc"
