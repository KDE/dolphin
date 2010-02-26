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

#include "nfotranslator.h"
#include <klocale.h>
#include <kstandarddirs.h>

#include <QUrl>

struct TranslationTuple {
    const char* const key;
    const char* const value;
};

// TODO: a lot of NFO's are missing yet
static const TranslationTuple g_translations[] = {
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#comment", I18N_NOOP2("@label", "Comment") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentCreated", I18N_NOOP2("@label creation date", "Created") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentSize", I18N_NOOP2("@label file content size", "Size") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#depends", I18N_NOOP2("@label file depends from", "Depends") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#isPartOf", I18N_NOOP2("@label parent directory", "Part of") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#lastModified", I18N_NOOP2("@label modified date of file", "Modified") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#mimeType", I18N_NOOP2("@label", "MIME Type") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#plainTextContent", I18N_NOOP2("@label", "Content") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#title", I18N_NOOP2("@label music title", "Title") },
    { "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#url", I18N_NOOP2("@label file URL", "Location") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nco#creator", I18N_NOOP2("@label", "Creator") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#averageBitrate", I18N_NOOP2("@label", "Average Bitrate") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#channels", I18N_NOOP2("@label", "Channels") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#characterCount", I18N_NOOP2("@label number of characters", "Characters") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#codec",  I18N_NOOP2("@label", "Codec") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#colorDepth", I18N_NOOP2("@label", "Color Depth") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#fileName", I18N_NOOP2("@label", "File Name") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#height", I18N_NOOP2("@label", "Height") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#interlaceMode", I18N_NOOP2("@label", "Interlace Mode") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#lineCount", I18N_NOOP2("@label number of lines", "Lines") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#programmingLanguage", I18N_NOOP2("@label", "Programming Language") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#sampleRate", I18N_NOOP2("@label", "Sample Rate") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#width", I18N_NOOP2("@label", "Width") },
    { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#wordCount", I18N_NOOP2("@label number of words", "Words") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#apertureValue", I18N_NOOP2("@label EXIF aperture value", "Aperture") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#exposureBiasValue", I18N_NOOP2("@label EXIF", "Exposure Bias Value") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#exposureTime", I18N_NOOP2("@label EXIF", "Exposure Time") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#flash", I18N_NOOP2("@label EXIF", "Flash") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#focalLength", I18N_NOOP2("@label EXIF", "Focal Length") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#focalLengthIn35mmFilm", I18N_NOOP2("@label EXIF", "Focal Length 35 mm") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#isoSpeedRatings", I18N_NOOP2("@label EXIF", "ISO Speed Ratings") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#make", I18N_NOOP2("@label EXIF", "Make") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#meteringMode", I18N_NOOP2("@label EXIF", "Metering Mode") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#model", I18N_NOOP2("@label EXIF", "Model") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#orientation", I18N_NOOP2("@label EXIF", "Orientation") },
    { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#whiteBalance", I18N_NOOP2("@label EXIF", "White Balance") },
    { "http://www.semanticdesktop.org/ontologies/2009/02/19/nmm#genre",  I18N_NOOP2("@label music genre", "Genre") },
    { "http://www.semanticdesktop.org/ontologies/2009/02/19/nmm#musicAlbum", I18N_NOOP2("@label music album", "Album") },
    { "http://www.semanticdesktop.org/ontologies/2009/02/19/nmm#trackNumber", I18N_NOOP2("@label music track number", "Track") },
    { "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", I18N_NOOP2("@label file type", "Type") },
    { 0, 0 } // mandatory last entry
};

class NfoTranslatorSingleton
{
public:
    NfoTranslator instance;
};
K_GLOBAL_STATIC(NfoTranslatorSingleton, s_nfoTranslator)

NfoTranslator& NfoTranslator::instance()
{
    return s_nfoTranslator->instance;
}

QString NfoTranslator::translation(const QUrl& uri) const
{
    const QString key = uri.toString();
    if (m_hash.contains(key)) {
        return m_hash.value(key);
    }

    // fallback if the URI is not translated
    QString translation;
    const int index = key.indexOf(QChar('#'));
    if (index >= 0) {
        translation = key.right(key.size() - index - 1);
    }
    return translation;
}

NfoTranslator::NfoTranslator() :
    m_hash()
{
    const TranslationTuple* tuple = &g_translations[0];
    while (tuple->key != 0) {
        m_hash.insert(tuple->key, i18n(tuple->value));
        ++tuple;
    }
}

NfoTranslator::~NfoTranslator()
{
}
