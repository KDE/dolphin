/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
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

#include "configwidget.h"

#include <qlayout.h>
#include <qlistview.h>

#include <klocale.h>

ConfigWidget::ConfigWidget()
{
  m_lstSearchEngines = EngineCfg::self()->engines();

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  
  m_pListView = new QListView( this );
  m_pListView->addColumn( i18n( "Name" ) );
  m_pListView->addColumn( i18n( "Keys" ) );
  m_pListView->addColumn( i18n( "Query" ) );
  
  topLayout->addWidget( m_pListView );
  
  QValueList<EngineCfg::Entry>::ConstIterator it = m_lstSearchEngines.begin();
  QValueList<EngineCfg::Entry>::ConstIterator end = m_lstSearchEngines.end();
  for (; it != end; ++it )
  {
    QString keystr;
    
    QStringList::ConstIterator sIt = (*it).m_lstKeys.begin();
    QStringList::ConstIterator sEnd = (*it).m_lstKeys.end();
    for (; sIt != sEnd; ++sIt )
      keystr += *sIt + ',';
    
    (void)new QListViewItem( m_pListView, (*it).m_strName, keystr, (*it).m_strQuery );
  }
  
  resize( topLayout->sizeHint() );
}

ConfigWidget::~ConfigWidget()
{
}
