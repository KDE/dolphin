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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "konqueror.h"

#include <dcopref.h>
#include <kconfig.h>
#include <qwhatsthis.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <klocale.h>

namespace KCMPerformance
{

Konqueror::Konqueror( QWidget* parent_P )
    : Konqueror_ui( parent_P )
    {
    QWhatsThis::add( rb_never_reuse,
        i18n( "Disables the minimization of memory usage and allows you "
              "to make each browsing activity independent from the others" ));
    QWhatsThis::add( rb_file_browsing_reuse,
        i18n( "With this option activated, only one instance of Konqueror "
              "used for file browsing will exist in the memory of your computer "
              "at any moment, "
              "no matter how many file browsing windows you open, "
              "thus reducing resource requirements."
              "<p>Be aware that this also means that, if something goes wrong, "
              "all your file browsing windows will be closed simultaneously" ));
    QWhatsThis::add( rb_always_reuse,
        i18n( "With this option activated, only one instance of Konqueror "
              "will exist in the memory of your computer at any moment, "
              "no matter how many browsing windows you open, "
              "thus reducing resource requirements."
              "<p>Be aware that this also means that, if something goes wrong, "
              "all your browsing windows will be closed simultaneously." ));
    connect( rb_never_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    connect( rb_file_browsing_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    connect( rb_always_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    rb_file_browsing_reuse->setChecked( true );
    
    QString tmp =
        i18n( "If non-zero, this option allows keeping Konqueror instances "
              "in memory after all their windows have been closed, up to the "
              "number specified in this option."
              "<p>When a new Konqueror instance is needed, one of these preloaded "
              "instances will be reused instead, improving responsiveness at "
              "the expense of the memory required by the preloaded instances." );
    QWhatsThis::add( sb_preload_count, tmp );
    QWhatsThis::add( lb_preload_count, tmp );
    QWhatsThis::add( cb_preload_on_startup,
	i18n( "If enabled, one Konqueror instance will be preloaded after KDE "
	      "startup."
	      "<p>This will make also the first Konqueror window opening faster, "
	      "at the expense of KDE startup taking longer (but Konqueror will "
	      "be started at the end of KDE startup, allowing you to work meanwhile, "
	      "so it's possible you even won't notice it's taking longer." ));
    QWhatsThis::add( cb_always_have_preloaded,
	i18n( "If enabled, KDE will try to have always one preloaded Konqueror instance ready, "
	      "preloading new instance in the background whenever there's none available, "
	      "so that opening Konqueror windows should always be fast."
	      "<p><b>WARNING:</b> It's possible this option will in some cases actually "
	      "have negative effect on the perceived performance." ));
    connect( sb_preload_count, SIGNAL( valueChanged( int )), SLOT( preload_count_changed( int )));
    connect( sb_preload_count, SIGNAL( valueChanged( int )), SIGNAL( changed()));
    connect( cb_preload_on_startup, SIGNAL( clicked()), SIGNAL( changed()));
    connect( cb_always_have_preloaded, SIGNAL( clicked()), SIGNAL( changed()));
    defaults();
    }

void Konqueror::preload_count_changed( int count )
    {
    cb_preload_on_startup->setEnabled( count >= 1 );
    // forcing preloading with count == 1 can often do more harm than good, because
    // if there's one konqy preloaded, and the user requests "starting" new konqueror,
    // the preloaded instance will be used, new one will be preloaded, and if the user soon
    // "quits" konqueror, one of the instances will have to be terminated
    cb_always_have_preloaded->setEnabled( count >= 2 );
    }
        
void Konqueror::load()
    {
    KConfig cfg( "konquerorrc", true );
    cfg.setGroup( "Reusing" );
    allowed_parts = cfg.readEntry( "SafeParts", "SAFE" );
    if( allowed_parts == "ALL" )
        rb_always_reuse->setChecked( true );
    else if( allowed_parts.isEmpty())
        rb_never_reuse->setChecked( true );
    else
        rb_file_browsing_reuse->setChecked( true );
    sb_preload_count->setValue( cfg.readNumEntry( "MaxPreloadCount", 1 ));
    cb_always_have_preloaded->setChecked( cfg.readBoolEntry( "AlwaysHavePreloaded", false ));
    cb_preload_on_startup->setChecked( cfg.readBoolEntry( "PreloadOnStartup", false ));
    }
    
void Konqueror::save()
    {
    KConfig cfg( "konquerorrc" );
    cfg.setGroup( "Reusing" );
    if( rb_always_reuse->isChecked())
        allowed_parts = "ALL";
    else if( rb_never_reuse->isChecked())
        allowed_parts = "";
    else
        {
        if( allowed_parts.isEmpty() || allowed_parts == "ALL" )
            allowed_parts = "SAFE";
        // else - keep allowed_parts as read from the file, as the user may have modified the list there
        }
    cfg.writeEntry( "SafeParts", allowed_parts );
    int count = sb_preload_count->value();
    cfg.writeEntry( "MaxPreloadCount", count );
    cfg.writeEntry( "PreloadOnStartup", cb_preload_on_startup->isChecked() && count >= 1 );
    cfg.writeEntry( "AlwaysHavePreloaded", cb_always_have_preloaded->isChecked() && count >= 2 );
    cfg.sync();
    DCOPRef ref1( "konqueror*", "KonquerorIface" );
    ref1.send( "reparseConfiguration()" );
    DCOPRef ref2( "kded", "konqy_preloader" );
    ref2.send( "reconfigure()" );
    }
    
void Konqueror::defaults()
    {
    rb_file_browsing_reuse->setChecked( true );
    allowed_parts = "SAFE";
    sb_preload_count->setValue( 1 );
    cb_preload_on_startup->setChecked( false );
    cb_always_have_preloaded->setChecked( false );
    preload_count_changed( sb_preload_count->value());
    }
    
} // namespace

#include "konqueror.moc"
