/***************************************************************************
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include <QMenu>

#include <kdebug.h>
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>

#include "kprotocolcombo_p.h"

const static int customProtocolIndex = 0;

KProtocolCombo::KProtocolCombo(const QString& protocol, KUrlNavigator* parent)
    : KUrlNavigatorButton(-1, parent),
      m_protocols(KProtocolInfo::protocols())
{
    qSort(m_protocols);
    QStringList::iterator it = m_protocols.begin();
    QStringList::iterator itEnd = m_protocols.end();
    QMenu* menu = new QMenu(this);
    while (it != itEnd)
    {
        //kDebug() << "info for " << *it << " "
        //          << KProtocolInfo::protocolClass(*it) << endl;
        //TODO: wow this is ugly. or .. is it? ;) we need a way to determine
        //      if a protocol is appropriate for use in a file manager. hum!
        //if (KProtocolInfo::capabilities(*it).findIndex("filemanager") == -1)

        // DF: why not just supportsListing?

        if (KProtocolInfo::protocolClass(*it) == ":" ||
            !KProtocolManager::supportsWriting(*it))
        {
        //kDebug() << "!!! removing " << *it << endl;
            QStringList::iterator tempIt = it;
            ++tempIt;
            m_protocols.erase(it);
            it = tempIt;
        }
        else
        {
            ++it;
        }
    }

//    setEditable(true);
//    menu->insertItem("", customProtocolIndex);
//    menu->insertStringList(m_protocols);
    int i = 0;
    for (QStringList::const_iterator it = m_protocols.constBegin();
         it != m_protocols.constEnd();
         ++it, ++i)
    {
        menu->insertItem(*it, i);
    }
    //menu->insertItems(m_protocols);
    connect(menu, SIGNAL(activated(int)), this, SLOT(setProtocol(int)));
    setText(protocol);
    setMenu(menu);
    setFlat(true);
}


// #include <kurl.h>
// #include "urlnavigator.h"
void KProtocolCombo::setProtocol(const QString& protocol)
{
    setText(protocol);
//    if (KProtocolInfo::isKnownProtocol(protocol))
//     int index = m_protocols.findIndex(protocol);
//     if (index == -1)
//     {
//         changeItem(protocol, customProtocolIndex);
//         setCurrentItem(customProtocolIndex);
//     }
//     else
//     {
//         setCurrentItem(index + 1);
//     }
}

void KProtocolCombo::setProtocol(int index)
{
    if (index < 0 || index > m_protocols.count())
    {
        return;
    }

    QString protocol = m_protocols[index];
kDebug() << "setProtocol " << index << " " << protocol << endl;
    setText(protocol);
    emit activated(protocol);
/*    */
}

QString KProtocolCombo::currentProtocol() const
{
    return text(); //currentText();
}

#include "kprotocolcombo_p.moc"
