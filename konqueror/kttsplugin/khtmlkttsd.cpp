/***************************************************************************
  Copyright:
  (C) 2002 by George Russell <george.russell@clara.net>
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
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
#include <kinstance.h>
#include <kiconloader.h>
#include <qmessagebox.h>
#include <klocale.h>
#include <qstring.h>
#include <qtimer.h>

#include <kapplication.h>
#include <dcopclient.h>

KHTMLPluginKTTSD::KHTMLPluginKTTSD( QObject* parent, const char* name )
    : Plugin( parent, name )
{
    (void) new KAction( "&Speak Text",
                        "kttsd", 0,
                        this, SLOT(slotReadOut()),
                        actionCollection(), "tools_kttsd" );
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
        KHTMLPart *part = (KHTMLPart *) parent();

        QString query;
        if (part->hasSelection())
          query = part->selectedText();
        else
          query = part->htmlDocument().body().innerText().string();

        DCOPClient *client = kapp->dcopClient();
        // If KTTSD not running, start it.
        if (!client->isApplicationRegistered("kttsd"))
        {
            QString error;
            if (kapp->startServiceByName("KTTSD", QStringList(), &error))
                QMessageBox::warning(0, i18n( "Starting KTTSD failed"), error );
            else
            {
                // Give KTTSD time to load.
                QTimer::singleShot(1000, this, SLOT(slotReadOut()));
                return;
            }
        }
        QByteArray  data;
        QByteArray  data2;
        QCString    replyType;
        QByteArray  replyData;
        QDataStream arg(data, IO_WriteOnly);
        arg << query << "";
        if ( !client->call("kttsd", "kspeech", "setText(QString,QString)",
                           data, replyType, replyData, true) )
           QMessageBox::warning( 0, i18n( "DCOP Call failed" ),
                                    i18n( "The DCOP call setText failed" ));
        QDataStream arg2(data2, IO_WriteOnly);
        arg2 << 0;
        if ( !client->call("kttsd", "kspeech", "startText(uint)",
                           data2, replyType, replyData, true) )
           QMessageBox::warning( 0, i18n( "DCOP Call failed" ),
                                    i18n( "The DCOP call startText failed" ));
    }
}

KPluginFactory::KPluginFactory( QObject* parent, const char* name )
  : KLibFactory( parent, name )
{
  s_instance = new KInstance("KPluginFactory");
}

QObject* KPluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
    QObject *obj = new KHTMLPluginKTTSD( parent, name );
    return obj;
}
KPluginFactory::~KPluginFactory()
{ delete s_instance; }


K_EXPORT_COMPONENT_FACTORY( libkhtmlkttsdplugin, KPluginFactory )

KInstance* KPluginFactory::s_instance = 0L;
