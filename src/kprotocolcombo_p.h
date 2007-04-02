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
#ifndef KPROTOCOLCOMBO_P_H
#define KPROTOCOLCOMBO_P_H

#include "kurlnavigatorbutton_p.h"

class KUrlNavigator;

/**
 * A combobox listing available protocols
 */
class KProtocolCombo : public KUrlNavigatorButton
{
    Q_OBJECT

public:
    explicit KProtocolCombo(const QString& protocol, KUrlNavigator* parent = 0);

    QString currentProtocol() const;

public Q_SLOTS:
    void setProtocol(const QString& protocol);
    void setProtocol(int index);

Q_SIGNALS:
    void activated(const QString& protocol);

private:
    QStringList m_protocols;
};

#endif
