/*  This file is part of the KDE project
    Copyright (C) 2000 Keunwoo Lee <klee@cs.washington.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
// -*- mode: c++; c-basic-offset: 2 -*-

#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>

/**
 * Provides functions that, given a collection of QStrings, will
 * automatically and intelligently assign menu accelerators to the
 * QStrings in the collection.
 *
 * NOTE: When this file speaks of "accelerators", we really mean
 * accelerators as defined by the KDE User Interface Guidelines.  We
 * do NOT mean "shortcuts", which are what's handled by most other KDE
 * libraries with "accel" in the name.
 *
 * In the Qt library, the mechanism for adding a keyboard accelerator
 * to a menu item is to insert an '&' before the letter. Since we
 * usually don't want to disturb the original collection, the idiom in
 * these functions is to populate a "target" QStringList parameter
 * with the input collectin's QStrings, plus possibly some added '&'
 * characters.
 *
 * That is the mechanism. Here is the policy, in order of decreasing
 * importance (it may seem like these are implementation details, but
 * IMHO the policy is an important part of the interface):
 *
 * 1. If the string already contains an '&' character, skip this
 * string, because we consider such strings to be "user-specified"
 * accelerators.
 *
 * 2. No accelerator may clash with a previously defined accelerator,
 * including any legal (alphanumeric) user-specified accelerator
 * anywhere in the collection
 *
 * 3. Prefer alphanumerics at the start of the string.
 *
 * 4. Otherwise, prefer alphanumerics at the start of a word.
 *
 * 5. Otherwise, choose any alphanumeric character not already
 * taken. If no such character is available, give up & skip this
 * string.
 *
 * A typical use of these functions would be to automatically assign
 * accelerators to a dynamically populated popup menu.  For example,
 * the core code was written to automatically set accelerators for the
 * "Load View Profile" popup menu for Konqueror.  We quickly realized
 * that it would be useful to make this facility more generally
 * available, so I abstracted it out into a set of templates.
 *
 * TODO:
 *
 * + Add sugar functions for more collections.
 *
 * + Add more Deref classes so that we can access a wider variety of
 * collections.
 * */
namespace KAccelGen
{

// HELPERS

/** Static dereference class, for use as a template parameter. */
template <class Iter>
class Deref
{
public:
    static QString deref(Iter i) { return *i; }
};

/** Static dereference class that calls the key() method on its
    target; for use as a template parameter. */
template <class Iter>
class Deref_Key
{
public:
    static QString deref(Iter i) { return i.key(); }
};

/**
 * Helper to determine if the given offset in the string could be a
 * legal alphanumeric accelerator.
 *
 * @param str   base string
 * @param index offset to check
 */
inline bool
isLegalAccelerator(const QString& str, uint index)
{
    return index < str.length()
        && str[index].isLetterOrNumber();
}

/**
 * Loads all legal predefined accelerators in the (implicitly
 * specified) collection into the given QMap.
 *
 * @param begin start iterator
 * @param end   (last+1) iterator
 * @param keys map to store output
 */
template <class Iter, class Deref>
inline void
loadPredefined(Iter begin, Iter end, QMap<QChar,bool>& keys)
{
    for (Iter i = begin; i != end; ++i) {
        QString item = Deref::deref(i);
        int user_ampersand = item.find(QChar('&'));
        if( user_ampersand >= 0 ) {
            // Sanity check.  Note that we don't try to find an
            // accelerator if the user shoots him/herself in the foot
            // by adding a bad '&'.
            if( isLegalAccelerator(item, user_ampersand+1) ) {
                keys.insert(item[user_ampersand+1], true);
            }
        }
    }
}


// ///////////////////////////////////////////////////////////////////
// MAIN USER FUNCTIONS


/**
 * Main, maximally flexible template function that assigns
 * accelerators to the elements of a collection of QStrings. Clients
 * will seldom use this directly, as it's usually easier to use one of
 * the wrapper functions that simply takes a collection (see below).
 *
 * The Deref template parameter is a class containing a static
 * dereferencing function, modeled after the comparison class C in
 * Stroustrup 13.4.
 *
 * @param begin  (you know)
 * @param end    (you know)
 * @param target collection to store generated strings
 */
template <class Iter, class Deref>
void
generate(Iter begin, Iter end, QStringList& target)
{
    // Will keep track of used accelerator chars
    QMap<QChar,bool> used_accels;

    // Prepass to detect manually user-coded accelerators
    loadPredefined<Iter,Deref>(begin, end, used_accels);

    // Main pass
    for (Iter i = begin; i != end; ++i) {
        QString item = Deref::deref(i);

        // Attempt to find a good accelerator, but only if the user
        // has not manually hardcoded one.
        int user_ampersand = item.find(QChar('&'));
        if( user_ampersand < 0 ) {
            bool found = false;
            uint found_idx;
            uint j;

            // Check word-starting letters first.
            for( j=0; j < item.length(); ++j ) {
                if( isLegalAccelerator(item, j)
                    && !used_accels.contains(item[j])
                    && (0 == j || j > 0 && item[j-1].isSpace()) ) {
                    found = true;
                    found_idx = j;
                    break;
                }
            }

            if( !found ) {
                // No word-starting letter; search for any letter.
                for( j=0; j < item.length(); ++j ) {
                    if( isLegalAccelerator(item, j)
                        && !used_accels.contains(item[j]) ) {
                        found = true;
                        found_idx = j;
                        break;
                    }
                }
            }

            if( found ) {
                used_accels.insert(item[j],true);
                item.insert(j,QChar('&'));
            }
        }

        target.append( item );
    }
}

/**
 * Convenience alias for the maximally flexible version of this
 * function. Assumes the default dereferencing class Deref<Iter>.
 * Actually, we only need this because g++ doesn't support default
 * template parameters properly.
 *
 * @param begin
 * @param end
 * @param target
 */
template <class Iter>
inline void
generate(Iter begin, Iter end, QStringList& target)
{
    generate< Iter, Deref<Iter> >(begin, end, target);
}

/**
 * Another convenience function; looks up the key instead of
 * dereferencing directly for the given iterator.
 *
 * @param begin
 * @param end
 * @param target
 */
template <class Iter>
inline void
generateFromKeys(Iter begin, Iter end, QStringList& target)
{
    generate< Iter, Deref_Key<Iter> >(begin, end, target);
}


/**
 * Convenience function; generates accelerators for all the items in
 * a QStringList.
 *
 * @param source Strings for which to generate accelerators
 * @param target Output for accelerator-added strings */
inline void
generate(const QStringList& source, QStringList& target)
{
    generate(source.begin(), source.end(), target);
}

/**
 * Convenience function; generates accelerators for all the values in
 * a QMap<T,QString>.
 *
 * @param source Map with input strings as VALUES.
 * @param target Output for accelerator-added strings */
template <class Key>
inline void
generateFromValues(const QMap<Key,QString>& source, QStringList& target)
{
    generate(source.begin(), source.end(), target);
}

/**
 * Convenience function; generates an accelerator mapping from all the
 * keys in a QMap<QString,T>
 *
 * @param source Map with input strings as KEYS.
 * @param target Output for accelerator-added strings */
template <class Data>
inline void
generateFromKeys(const QMap<QString,Data>& source, QStringList& target)
{
    generateFromKeys(source.begin(), source.end(), target);
}


} // end namespace KAccelGen
