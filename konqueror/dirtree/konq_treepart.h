/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef konq_treepart_h
#define konq_treepart_h

#include "konq_treemodule.h"
#include <kparts/factory.h>
#include <kparts/browserextension.h>
#include <kparts/part.h>

class KonqTreeBrowserExtension;
class KonqTree;
class KonqTreePart;

// The KParts interface for the multi-purpose tree
// Factory, Browser Extension, Part

class KonqTreeFactory : public KParts::Factory
{
public:
    KonqTreeFactory() {}
    virtual ~KonqTreeFactory();
    virtual KParts::Part* createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList & );

    static KInstance *instance();
private:
    static KInstance *s_instance;
};

class KonqTreeBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    KonqTreeBrowserExtension( KonqTreePart *parent, KonqTree *tree );

    void enableActions( bool copy, bool cut, bool paste,
                        bool trash, bool del, bool shred,
                        bool rename = false );

protected slots:
    void copy();
    void cut();
    void paste();
    void trash();
    void del();
    void shred();
    void rename();

protected:
    KURL::List selectedUrls();

private:
    KonqTree *m_pTree;
};

class KonqTreePart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_PROPERTY( bool supportsUndo READ supportsUndo )
public:
    KonqTreePart( QWidget *parentWidget, QObject *parent, const char *name = 0L );
    virtual ~KonqTreePart();

    virtual bool openURL( const KURL & );

    virtual bool closeURL() { return true; }
    virtual bool openFile() { return true; }

    bool supportsUndo() const { return true; }

    void mmbClicked( KFileItem * fileItem );

    KonqTreeBrowserExtension * extension() const
    { return m_extension; }

    KonqTree * tree() const
    { return m_pTree; }

    void emitStatusBarText( const QString& text );

private:
    KonqTreeBrowserExtension * m_extension;
    KonqTree * m_pTree;
};


#endif
