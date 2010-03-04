/*****************************************************************************
 * Copyright (C) 2010 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef KNFOTRANSLATOR_H
#define KNFOTRANSLATOR_H

#include <QHash>
#include <QString>

class QUrl;

/**
 * @brief Returns translations for Nepomuk File Ontology URIs.
 *
 * See http://www.semanticdesktop.org/ontologies/nfo/.
 */
class KNfoTranslator
{
public:
    static KNfoTranslator& instance();
    QString translation(const QUrl& uri) const;

protected:
    KNfoTranslator();
    virtual ~KNfoTranslator();
    friend class KNfoTranslatorSingleton;

private:
    QHash<QString, QString> m_hash;
};

#endif // KNFO_TRANSLATOR_H
