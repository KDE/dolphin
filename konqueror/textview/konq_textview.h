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

#ifndef __konq_textview_h__
#define __konq_textview_h__

#include "browser.h"

#include <qmultilineedit.h>

class QFont;
class KonqSearchDialog;
class QToggleAction;
class QAction;
class KonqTextView;
class KMultiLineEdit;

class KonqTextPrintingExtension : public PrintingExtension
{
  Q_OBJECT
public:
  KonqTextPrintingExtension( KonqTextView *textView );

  virtual void print();

private:
  KonqTextView *m_textView;
};

class KonqTextEditExtension : public EditExtension
{
  Q_OBJECT
public:
  KonqTextEditExtension( KonqTextView *textView );
  
  virtual void can( bool &copy, bool &paste, bool &move );

  virtual void copySelection();
  virtual void pasteSelection();
  virtual void moveSelection( const QString &destinationURL = QString::null );

private:
  KonqTextView *m_textView;  
};

class KonqTextView : public BrowserView
{
  friend class KonqTextPrintingExtension;
  friend class KonqTextEditExtension;

  Q_OBJECT
  
public:
  KonqTextView();
  virtual ~KonqTextView();
  
  virtual void openURL( const QString &url, bool reload = false,
                        int xOffset = 0, int yOffset = 0 );

  virtual QString url();
  virtual int xOffset();
  virtual int yOffset();
  virtual void stop();

  virtual bool eventFilter( QObject *o, QEvent *e );

/*
  virtual void can( bool &copy, bool &paste, bool &move );
  
  virtual void copySelection();
  virtual void pasteSelection();
  virtual void moveSelection( const QCString & );
*/
protected slots:
  void slotFinished( int );
  void slotRedirection( int, const char * );
  void slotData( int, const char *, int );
  void slotError( int, int, const char * );

  void slotFindFirst( const QString &_text, bool backwards, bool caseSensitive );
  void slotFindNext( bool backwards, bool caseSensitive );

  void slotSelectAll();
  void slotEdit();
  void slotFixedFont();
  void slotSearch();

protected:
//  virtual void mousePressEvent( QMouseEvent *e );
  virtual void resizeEvent( QResizeEvent * );

  KMultiLineEdit *multiLineEdit() const { return m_pEdit; }

private:
  KMultiLineEdit *m_pEdit;

  QString m_strURL;

  int m_jobId;
  int m_iXOffset;
  int m_iYOffset;
  bool m_bFixedFont;
  
  QString m_strSearchText;
  int m_iSearchPos;
  int m_iSearchLine;
  KonqSearchDialog *m_pSearchDialog;
  bool m_bFound;
  
  long int m_idFixedFont;
  
  QAction *m_paSelectAll;
  QAction *m_paEdit;
  QAction *m_paSearch;
  QToggleAction *m_ptaFixedFont;
};

class KMultiLineEdit : public QMultiLineEdit
{
  Q_OBJECT
public:
  KMultiLineEdit( QWidget *parent = 0, const char *name = 0 ) : QMultiLineEdit( parent, name ) {}
  ~KMultiLineEdit() {}
  
  bool textMarked() const { return hasMarkedText(); }

  int xScrollOffset() const { return xOffset(); }
  int yScrollOffset() const { return yOffset(); }
  
  void setScrollOffset( int x, int y ) { setOffset( x, y ); }
 
};

#endif
