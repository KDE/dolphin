/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_textview_h__
#define __konq_textview_h__

#include <kparts/browserextension.h>
#include <kuserpaths.h>

#include "konq_textviewwidget.h"

#include <konqoperations.h>
#include <qvaluelist.h>
#include <qlistview.h>
#include <qstringlist.h>
#include <kaction.h>

#include <iostream.h>

class KonqTextViewWidget;
class KonqTextView;
class KToggleAction;
class TextViewBrowserExtension;

class KonqTextView : public KParts::ReadOnlyPart
{
   friend class KonqTextViewWidget;
   Q_OBJECT
   public:
      KonqTextView( QWidget *parentWidget, QObject *parent, const char *name );
      virtual ~KonqTextView();

      virtual bool openURL( const KURL &url );
      virtual bool closeURL();
      virtual bool openFile() { return true; }

      KonqTextViewWidget *textViewWidget() const { return (KonqTextViewWidget*)m_pTextView; }
      TextViewBrowserExtension *extension() const { return (TextViewBrowserExtension*)m_browser; }

   public slots:
      void slotSelect();
      void slotUnselect();
      void slotSelectAll();
      void slotUnselectAll();
      void slotInvertSelection();

   protected:
      virtual void guiActivateEvent( KParts::GUIActivateEvent *event );

   protected slots:
      void slotReloadText();
      void slotShowDot();

      void slotShowTime();
      void slotShowSize();
      void slotShowOwner();
      void slotShowGroup();
      void slotShowPermissions();
   private:
      KToggleAction m_paShowDot;
      KAction m_paSelect;
      KAction m_paUnselect;
//      KAction *m_paSelectAll;
      KAction m_paUnselectAll;
      KAction m_paInvertSelection;
      
      KToggleAction m_paShowTime;
      KToggleAction m_paShowSize;
      KToggleAction m_paShowOwner;
      KToggleAction m_paShowGroup;
      KToggleAction m_paShowPermissions;
      
      KonqTextViewWidget *m_pTextView;
      TextViewBrowserExtension *m_browser;
};


class TextViewBrowserExtension : public KParts::BrowserExtension
{
   Q_OBJECT
   friend class KonqTextView;
   friend class KonqTextViewWidget;
   public:
      TextViewBrowserExtension( KonqTextView *textView );
      virtual ~TextViewBrowserExtension() {   cerr<<"~KonqTextViewBrowserExtension"<<endl;};
      virtual int xOffset();
      virtual int yOffset();

   protected slots:
      void updateActions();

      void copy();
      void cut();
      void pastecut() { pasteSelection( true ); }
      void pastecopy() { pasteSelection( false ); }
      void trash() { KonqOperations::del(KonqOperations::TRASH,
                                         m_textView->textViewWidget()->selectedUrls());}
      void del() { KonqOperations::del(KonqOperations::DEL,
                                       m_textView->textViewWidget()->selectedUrls());}
      void shred() { KonqOperations::del(KonqOperations::SHRED,
                                         m_textView->textViewWidget()->selectedUrls());}

      void reparseConfiguration();
      void saveLocalProperties();
      void savePropertiesAsDefault();

   private:
      void pasteSelection( bool move );

      KonqTextView *m_textView;
};


#endif
