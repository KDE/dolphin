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

#include "konq_textview.h"
#include "konq_textviewitems.h"
#include "konq_factory.h"

#include <kstdaction.h>
#include <klineeditdlg.h>

#include <kcursor.h>
#include <kdirlister.h>
#include <konqfileitem.h>
#include <kio/paste.h>
#include <kio/job.h>
#include <kdebug.h>
#include <konq_propsview.h>
#include <kparts/mainwindow.h>
#include <kparts/partmanager.h>
#include <kparts/factory.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <qkeycode.h>
#include <qlist.h>
#include <qdragobject.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <klocale.h>
#include <klibloader.h>

class KonqTextViewFactory : public KParts::Factory
{
   public:
      KonqTextViewFactory()
      {
         printf("KonqTextViewFactory::KonqTextViewFactory\n");
         KonqFactory::instanceRef();
      }
      virtual ~KonqTextViewFactory()
      {
         KonqFactory::instanceUnref();
      }

      virtual KParts::Part* createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList & )
      {
         printf("KonqFactory::create(new KonqTextView)\n");
         KParts::Part *obj = new KonqTextView( parentWidget, parent, name );
         emit objectCreated( obj );
         return obj;
      }
};

extern "C"
{
  void *init_libkonqtextview()
  {
    printf("init_libkonqtextview\n");
    return new KonqTextViewFactory;
  }
};

TextViewBrowserExtension::TextViewBrowserExtension( KonqTextView *textView )
 : KParts::BrowserExtension( textView )
{
  m_textView = textView;
}

int TextViewBrowserExtension::xOffset()
{
  return m_textView->textViewWidget()->contentsX();
}

int TextViewBrowserExtension::yOffset()
{
  return m_textView->textViewWidget()->contentsY();
}


void TextViewBrowserExtension::updateActions()
{
  // This code is very related to KonqIconViewWidget::slotSelectionChanged

  QValueList<KonqTextViewItem*> selection;
  m_textView->textViewWidget()->selectedItems( selection );

  bool cutcopy, del;
  bool bInTrash = false;
  QValueList<KonqTextViewItem*>::ConstIterator it = selection.begin();
  for (; it != selection.end(); ++it )
  {
    if ( (*it)->item()->url().directory(false) == KUserPaths::trashPath() )
      bInTrash = true;
  }
  cutcopy = del = ( selection.count() > 0 );

  emit enableAction( "copy", cutcopy );
  emit enableAction( "cut", cutcopy );
  emit enableAction( "trash", del && !bInTrash );
  emit enableAction( "del", del );
  emit enableAction( "shred", del );

  bool bKIOClipboard = !KIO::isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  bool paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
    (selection.count() == 1); // Let's allow pasting only on an item, not on the background

  emit enableAction( "pastecut", paste );
  emit enableAction( "pastecopy", paste );
}

void TextViewBrowserExtension::cut()
{
   //TODO: grey out item
   copy();
}

void TextViewBrowserExtension::copy()
{
   QValueList<KonqTextViewItem*> selection;

   m_textView->textViewWidget()->selectedItems( selection );

   QStringList lstURLs;

   QValueList<KonqTextViewItem*>::ConstIterator it = selection.begin();
   QValueList<KonqTextViewItem*>::ConstIterator end = selection.end();
   for (; it != end; ++it )
      lstURLs.append( (*it)->item()->url().url() );

   QUriDrag *urlData = new QUriDrag;
   urlData->setUnicodeUris( lstURLs );
   QApplication::clipboard()->setData( urlData );
}

void TextViewBrowserExtension::pasteSelection( bool move )
{
   QValueList<KonqTextViewItem*> selection;
   m_textView->textViewWidget()->selectedItems( selection );
   assert ( selection.count() == 1 );
   //KIO::pasteClipboard( selection.first()->item()->url().url(), move );
   KIO::pasteClipboard( selection.first()->item()->url(), move );
}

void TextViewBrowserExtension::reparseConfiguration()
{
   // m_pProps is a problem here (what is local, what is global ?)
   // but settings is easy :
   m_textView->textViewWidget()->initConfig();
}

void TextViewBrowserExtension::saveLocalProperties()
{
   // TODO move this to KonqTextView. Ugly.
   m_textView->textViewWidget()->m_pProps->saveLocal( m_textView->url() );
}

void TextViewBrowserExtension::savePropertiesAsDefault()
{
   m_textView->textViewWidget()->m_pProps->saveAsDefault();
}

KonqTextView::KonqTextView( QWidget *parentWidget, QObject *parent, const char *name )
: KParts::ReadOnlyPart( parent, name )
,m_paShowDot(i18n("Show &Dot Files"), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" )
,m_paSelect(i18n("&Select..."), CTRL+Key_Plus, this, SLOT( slotSelect() ), actionCollection(), "select" )
,m_paUnselect(i18n("&Unselect..."), CTRL+Key_Minus, this, SLOT( slotUnselect() ), actionCollection(), "unselect" )
,m_paUnselectAll(i18n("U&nselect All"), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" )
,m_paInvertSelection(i18n("&Invert Selection"), CTRL+Key_Asterisk, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" )
,m_paShowTime(i18n("Show &Modification Time"), 0, this, SLOT( slotShowTime() ), actionCollection(), "show_time" )
,m_paShowSize(i18n("Show Filesize"), 0, this, SLOT( slotShowSize() ), actionCollection(), "show_size" )
,m_paShowOwner(i18n("Show Owner"), 0, this, SLOT( slotShowOwner() ), actionCollection(), "show_owner" )
,m_paShowGroup(i18n("Show Group"), 0, this, SLOT( slotShowGroup() ), actionCollection(), "show_group" )
,m_paShowPermissions(i18n("Show permissions"), 0, this, SLOT( slotShowPermissions() ), actionCollection(), "show_permissions" )
//,m_pTextView( this, parentWidget )
//,m_browser( this )
{
   m_pTextView=new KonqTextViewWidget( this, parentWidget );
   m_browser=new TextViewBrowserExtension( this );
   setInstance( KonqFactory::instance() );
   setXMLFile( "konq_textview.rc" );

   setWidget( m_pTextView );

//   m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), this, "selectall" );

   QObject::connect( m_pTextView, SIGNAL( selectionChanged() ),
                     m_browser, SLOT( updateActions() ) );

/*   m_paShowDot=new KToggleAction(i18n("Show &Dot Files"), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );
   m_paSelect=new KAction(i18n("&Select..."), CTRL+Key_Plus, this, SLOT( slotSelect() ), actionCollection(), "select" );
   m_paUnselect=new KAction(i18n("&Unselect..."), CTRL+Key_Minus, this, SLOT( slotUnselect() ), actionCollection(), "unselect" );
   m_paUnselectAll=new KAction(i18n("U&nselect All"), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
   m_paInvertSelection=new KAction(i18n("&Invert Selection"), CTRL+Key_Asterisk, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );
   m_paShowTime=new KToggleAction(i18n("Show &Modification Time"), 0, this, SLOT( slotShowTime() ), actionCollection(), "show_time" );
   m_paShowSize=new KToggleAction(i18n("Show Filesize"), 0, this, SLOT( slotShowSize() ), actionCollection(), "show_size" );
   m_paShowOwner=new KToggleAction(i18n("Show Owner"), 0, this, SLOT( slotShowOwner() ), actionCollection(), "show_owner" );
   m_paShowGroup=new KToggleAction(i18n("Show Group"), 0, this, SLOT( slotShowGroup() ), actionCollection(), "show_group" );
   m_paShowPermissions=new KToggleAction(i18n("Show permissions"), 0, this, SLOT( slotShowPermissions() ), actionCollection(), "show_permissions" );*/


   m_paShowTime.setChecked(FALSE);
   m_paShowSize.setChecked(TRUE);
   m_paShowOwner.setChecked(FALSE);
   m_paShowGroup.setChecked(FALSE);
   m_paShowPermissions.setChecked(FALSE);
}

KonqTextView::~KonqTextView()
{
   cerr<<"~KonqTextView"<<endl;
}

bool KonqTextView::openURL( const KURL &url )
{
   m_url = url;

   KURL u( url );

   emit setWindowCaption( u.decodedURL() );

   return m_pTextView->openURL( url );
}

bool KonqTextView::closeURL()
{
   m_pTextView->stop();
   return true;
}

void KonqTextView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
   KParts::ReadOnlyPart::guiActivateEvent( event );
   if ( event->activated() )
      m_browser->updateActions();
}

void KonqTextView::slotReloadText()
{
//  m_pTextView->openURL( url(), m_pTextView->contentsX(), m_pTextView->contentsY() );
}

void KonqTextView::slotShowDot()
{
   m_pTextView->dirLister()->setShowingDotFiles( m_paShowDot.isChecked() );
   //m_pTextView.openURL(m_pTextView.url());
}

void KonqTextView::slotShowTime()
{
   m_pTextView->m_settingsChanged=TRUE;
   m_pTextView->m_showTime=m_paShowTime.isChecked();
   if (!m_pTextView->url().isMalformed()) m_pTextView->openURL(m_pTextView->url());
};

void KonqTextView::slotShowSize()
{
   m_pTextView->m_settingsChanged=TRUE;
   m_pTextView->m_showSize=m_paShowSize.isChecked();
   if (!m_pTextView->url().isMalformed()) m_pTextView->openURL(m_pTextView->url());
};

void KonqTextView::slotShowOwner()
{
   m_pTextView->m_settingsChanged=TRUE;
   m_pTextView->m_showOwner=m_paShowOwner.isChecked();
   if (!m_pTextView->url().isMalformed()) m_pTextView->openURL(m_pTextView->url());
};

void KonqTextView::slotShowGroup()
{
   m_pTextView->m_settingsChanged=TRUE;
   m_pTextView->m_showGroup=m_paShowGroup.isChecked();
   if (!m_pTextView->url().isMalformed()) m_pTextView->openURL(m_pTextView->url());
};

void KonqTextView::slotShowPermissions()
{
   m_pTextView->m_settingsChanged=TRUE;
   m_pTextView->m_showPermissions=m_paShowPermissions.isChecked();
   if (!m_pTextView->url().isMalformed()) m_pTextView->openURL(m_pTextView->url());
};

void KonqTextView::slotSelect()
{
   KLineEditDlg l( i18n("Select files:"), "", m_pTextView );
   if ( l.exec() )
   {
      QString pattern = l.text();
      if ( pattern.isEmpty() )
         return;

      QRegExp re( pattern, true, true );

      m_pTextView->blockSignals( true );

      for (KonqTextViewWidget::iterator it = m_pTextView->begin(); it != m_pTextView->end(); it++ )
         if ( re.match( it->text(1) ) != -1 )
            it->setSelected( TRUE);

      m_pTextView->blockSignals( false );
      m_pTextView->repaintContents(0,0,m_pTextView->width(),m_pTextView->height());
      m_pTextView->updateSelectedFilesInfo();

      // do this once, not for each item
      //m_pTextView.slotSelectionChanged();
      //slotDisplayFileSelectionInfo();
   }
}

void KonqTextView::slotUnselect()
{
   KLineEditDlg l( i18n("Unselect files:"), "", m_pTextView );
   if ( l.exec() )
   {
      QString pattern = l.text();
      if ( pattern.isEmpty() )
         return;

      QRegExp re( pattern, TRUE, TRUE );

      m_pTextView->blockSignals(TRUE);

      for (KonqTextViewWidget::iterator it = m_pTextView->begin(); it != m_pTextView->end(); it++ )
         if ( re.match( it->text(1) ) != -1 )
            it->setSelected(FALSE);

      m_pTextView->blockSignals(FALSE);
      m_pTextView->repaintContents(0,0,m_pTextView->width(),m_pTextView->height());
      
      // do this once, not for each item
      //m_pTextView.slotSelectionChanged();
      //slotDisplayFileSelectionInfo();
      m_pTextView->updateSelectedFilesInfo();
   }
}

void KonqTextView::slotSelectAll()
{
   m_pTextView->selectAll(TRUE);
   m_pTextView->updateSelectedFilesInfo();
}

void KonqTextView::slotUnselectAll()
{
    m_pTextView->selectAll(FALSE);
    m_pTextView->updateSelectedFilesInfo();
}


void KonqTextView::slotInvertSelection()
{
    m_pTextView->invertSelection();
    m_pTextView->updateSelectedFilesInfo();

}

#include "konq_textview.moc"


