/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef _NEPOMUK_RESOURCE_TAGGING_WIDGET_H_
#define _NEPOMUK_RESOURCE_TAGGING_WIDGET_H_

#include <QtGui/QWidget>

#include <nepomuk/tag.h>

class QEvent;
class QContextMenuEvent;

namespace Nepomuk {
    class ResourceTaggingWidget : public QWidget
    {
        Q_OBJECT

    public:
        ResourceTaggingWidget( QWidget* parent = 0 );
        ~ResourceTaggingWidget();

    Q_SIGNALS:
        void tagClicked( const Nepomuk::Tag& tag );

    public Q_SLOTS:
        void setResource( const Nepomuk::Resource& );
        void setResources( const QList<Nepomuk::Resource>& );
        void showTagPopup( const QPoint& pos );

    private Q_SLOTS:
        void slotTagToggled( const Nepomuk::Tag& tag, bool enabled );
        void slotTagAdded( const Nepomuk::Tag& tag );

    protected:
        void contextMenuEvent( QContextMenuEvent* e );

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_slotShowTaggingPopup() )
    };
}

#endif
