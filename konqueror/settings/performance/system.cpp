/*
 *  Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
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
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "system.h"

#include <kconfig.h>

#include <qcheckbox.h>
#include <qlabel.h>
#include <klocale.h>

namespace KCMPerformance
{

SystemWidget::SystemWidget( QWidget* parent_P )
    : System_ui( parent_P )
    {
    QString tmp = 
        i18n( "<p>During startup KDE needs to perform a check of its system configuration"
              " (mimetypes, installed applications, etc.), and in case the configuration"
              " has changed since the last time, the system configuration cache (KSyCoCa)"
              " needs to be updated.</p>"
              "<p>This option delays the check, which avoid scanning all directories containing"
              " files describing the system during KDE startup, thus"
              " making KDE startup faster. However, in the rare case the system configuration"
              " has changed since the last time, and the change is needed before this"
              " delayed check takes place, this option may lead to various problems"
              " (missing applications in the K Menu, reports from applications about missing"
              " required mimetypes, etc.).</p>"
              "<p>Changes of system configuration mostly happen by (un)installing applications."
              " It is therefore recommended to turn this option temporarily off while"
              " (un)installing applications.</p>"
              "<p>For this reason, usage of this option is not recommended. The KDE crash"
              " handler will refuse to provide backtrace for the bugreport with this option"
              " turned on (you will need to reproduce it again with this option turned off,"
              " or turn on the developer mode for the crash handler).</p>" );
    cb_disable_kbuildsycoca->setWhatsThis( tmp );
    label_kbuildsycoca->setWhatsThis( tmp );
    connect( cb_disable_kbuildsycoca, SIGNAL( clicked()), SIGNAL( changed()));
    defaults();
    }

void SystemWidget::load()
    {
    KConfig cfg( "kdedrc", true );
    cfg.setGroup( "General" );
    cb_disable_kbuildsycoca->setChecked( cfg.readBoolEntry( "DelayedCheck", false ));
    }

void SystemWidget::save()
    {
    KConfig cfg( "kdedrc" );
    cfg.setGroup( "General" );
    cfg.writeEntry( "DelayedCheck", cb_disable_kbuildsycoca->isChecked());
    }

void SystemWidget::defaults()
    {
    cb_disable_kbuildsycoca->setChecked( false );
    }

} // namespace

#include "system.moc"
