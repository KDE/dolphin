/***************************************************************************
  Copyright (C) 2002 by George Russell <george.russell@clara.net>
  Copyright (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <khtml_part.h> // this plugin applies to a khtml part
#include <dom/html_document.h>
#include <dom/html_element.h>
#include <dom/dom_string.h>
#include <kdebug.h>
#include "khtmlkttsd.h"
#include <kaction.h>
#include <kgenericfactory.h>
#include <kicon.h>
#include <kiconloader.h>
#include <QMessageBox>
#include <klocale.h>
#include <QString>
#include <QTimer>
#include <kspeech.h>
#include <QBuffer>
#include <QtDBus>
#include <kapplication.h>
#include <kservicetypetrader.h>
#include <ktoolinvocation.h>

KHTMLPluginKTTSD::KHTMLPluginKTTSD( QObject* parent, const QStringList& )
    : Plugin( parent )
{
    // If KTTSD is not installed, hide action.
    KService::List offers = KServiceTypeTrader::self()->query("DCOP/Text-to-Speech", "Name == 'KTTSD'");
    if (offers.count() > 0)
    {
        KAction *action = new KAction(KIcon("kttsd"),  i18n("&Speak Text"), actionCollection(), "tools_kttsd" );
        connect(action, SIGNAL(triggered(bool) ), SLOT(slotReadOut()));
    }
    else
        kDebug() << "KHTMLPLuginKTTSD::KHTMLPluginKTTSD: KTrader did not find KTTSD." << endl;
}

KHTMLPluginKTTSD::~KHTMLPluginKTTSD()
{
}

void KHTMLPluginKTTSD::slotReadOut()
{
    // The parent is assumed to be a KHTMLPart
    if ( !parent()->inherits("KHTMLPart") )
       QMessageBox::warning( 0, i18n( "Cannot Read source" ),
                                i18n( "You cannot read anything except web pages with\n"
                                      "this plugin, sorry." ));
    else
    {
        if (!QDBus::sessionBus().interface()->isServiceRegistered("kttsd"))
        {
            QString error;
            if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error))
                QMessageBox::warning(0, i18n( "Starting KTTSD Failed"), error );
        }

        // Find out if KTTSD supports xhtml (rich speak).
        bool supportsXhtml = false;
		QDBusInterface kttsd( "org.kde.KSpeech", "/KSpeech", "org.kde.KSpeech" );
		QDBusReply<bool> reply = kttsd.call("supportsMarkup", "", KSpeech::soHtml);
        if ( !reply.isValid())
            QMessageBox::warning( 0, i18n( "DBUS Call Failed" ),
                                     i18n( "The DBUS call supportsMarkup failed." ));
        else
        {
			supportsXhtml = reply;
        }

        KHTMLPart *part = (KHTMLPart *) parent();

        QString query;
        if (supportsXhtml)
        {
            kDebug() << "KTTS claims to support rich speak (XHTML to SSML)." << endl;
            if (part->hasSelection())
                query = part->selectedTextAsHTML();
            else
            {
                // TODO: Fooling around with the selection probably has unwanted
                // side effects, but until a method is supplied to get valid xhtml
                // from entire document..
                // query = part->document().toString().string();
                part->selectAll();
                query = part->selectedTextAsHTML();
                // Restore no selection.
                part->setSelection(part->document().createRange());
            }
        } else {
            if (part->hasSelection())
                query = part->selectedText();
            else
                query = part->htmlDocument().body().innerText().string();
        }
        // kDebug() << "KHTMLPluginKTTSD::slotReadOut: query = " << query << endl;

		reply = kttsd.call("setText", query, "");
        if ( !reply.isValid())
            QMessageBox::warning( 0, i18n( "DBUS Call Failed" ),
                                     i18n( "The DBUS call setText failed." ));
		reply = kttsd.call("startText", 0);
        if ( !reply.isValid())
            QMessageBox::warning( 0, i18n( "DBUS Call Failed" ),
                                     i18n( "The DBUS call startText failed." ));
    }
}

K_EXPORT_COMPONENT_FACTORY( libkhtmlkttsdplugin, KGenericFactory<KHTMLPluginKTTSD>("khtmlkttsd") )

#include "khtmlkttsd.moc"
