//-----------------------------------------------------------------------------
//
// KDE Help Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef __HTML_OPTIONS_H__
#define __HTML_OPTIONS_H__

#include <qtabdialog.h>
#include <qcombobox.h>
#include <qstrlist.h>
#include <qcheckbox.h>
#include <kconfig.h>
#include <kcolorbtn.h>
#include <qradiobutton.h>
#include <kcharsets.h>

#include <kcmodule.h>
#include <kwm.h>


//-----------------------------------------------------------------------------

class KFontOptions : public KCModule
{
  Q_OBJECT
public:
  KFontOptions( QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();

public slots:
  void slotFontSize( int );
  void slotStandardFont(const QString& n);
  void slotFixedFont(const QString& n);
  void slotCharset( const QString& n);


private slots:

  void changed();


private:
  void getFontList( QStrList &list, const char *pattern );
  void addFont( QStrList &list, const char *xfont );
  void updateGUI();

private:
  QRadioButton* m_pSmall;
  QRadioButton* m_pMedium;
  QRadioButton* m_pLarge;
  QComboBox* m_pFixed;
  QComboBox* m_pStandard;
  QComboBox* m_pCharset;

  int fSize;
  QString stdName;
  QString fixedName;
  QString charsetName;
  QStrList standardFonts;
  QStrList fixedFonts;
  QStringList charsets;

};

//-----------------------------------------------------------------------------

class KColorOptions : public KCModule
{
  Q_OBJECT
public:
  KColorOptions( QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();

protected slots:
  void slotBgColorChanged( const QColor &col );
  void slotTextColorChanged( const QColor &col );
  void slotLinkColorChanged( const QColor &col );
  void slotVLinkColorChanged( const QColor &col );


private slots:

  void changed();


private:
  KColorButton* m_pBg;
  KColorButton* m_pText;
  KColorButton* m_pLink;
  KColorButton* m_pVLink;
  QCheckBox *forceDefaultsbox;
  QColor bgColor;
  QColor textColor;
  QColor linkColor;
  QColor vLinkColor;

};

class KHtmlOptions : public KCModule
{
  Q_OBJECT
public:
  KHtmlOptions( QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();


private slots:
  
  void changed();


private:
    QLineEdit *le_JavaPath;
    QCheckBox *cb_enableJava;
    QCheckBox *cb_enableJavaScript;
    QCheckBox *m_pAutoLoadImagesCheckBox;

};

#endif		// __HTML_OPTIONS_H__

