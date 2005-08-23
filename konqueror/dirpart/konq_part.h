/* This file is part of the KDE project
   Copyright (c) 2005 Pascal Létourneau <pascal.letourneau@kdemail.net>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_part_h__
#define __konq_part_h__

#include <konq_dirpart.h>

class QAbstractItemView;
template<typename T> class QList;
class QModelIndex;
typedef QList<QModelIndex> QModelIndexList;
class KAboutData;
class KDirLister;
class KonqModel;
class KonqFileTip;

class KonqPart : public KonqDirPart
{
    Q_OBJECT
public:
    KonqPart( QWidget* parentWidget, const char*, QObject* parent, const char*, const QStringList& );
    virtual ~KonqPart();
    static KAboutData* createAboutData();
private:
    KDirLister* m_dirLister;
    QAbstractItemView* m_view;
    KonqModel* m_model;
    KonqFileTip* m_fileTip;
protected slots:
    virtual void slotNewItems( const KFileItemList& );
    virtual bool openFile() { return true; }
    virtual void disableIcons( const KURL::List& ) {}
    virtual const KFileItem* currentItem();
    virtual void slotStarted() {}
    virtual void slotCanceled() {}
    virtual void slotCompleted();
    virtual void slotDeleteItem( KFileItem* ) {}
    virtual void slotRefreshItems( const KFileItemList& ) {}
    virtual void slotClear();
    virtual void slotRedirection( const KURL& ) {}
    virtual bool doOpenURL( const KURL& );
    virtual bool doCloseURL() { return true; }
private slots:
    void slotExecute( QMouseEvent* ev );
    void slotToolTip( const QModelIndex& index );
    void slotContextMenu( const QPoint& pos, const QModelIndexList& indexes );
};

namespace KParts { template<typename T> class GenericFactory; }
typedef KParts::GenericFactory<KonqPart> KonqListFactory;
#endif

