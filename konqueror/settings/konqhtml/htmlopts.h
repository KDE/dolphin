//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef __HTML_OPTIONS_H__
#define __HTML_OPTIONS_H__

#include <kcmodule.h>
#include <qmap.h>

class KColorButton;
class KConfig;
class KListView;
class KURLRequester;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QListViewItem;
class QRadioButton;
class QSpinBox;

//-----------------------------------------------------------------------------

class KAppearanceOptions : public KCModule
{
  Q_OBJECT
public:
  KAppearanceOptions(KConfig *config, QString group, QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();

public slots:
  void slotFontSize( int );
  void slotMinimumFontSize( int );
  void slotStandardFont(const QString& n);
  void slotFixedFont(const QString& n);
  void slotSerifFont( const QString& n );
  void slotSansSerifFont( const QString& n );
  void slotCursiveFont( const QString& n );
  void slotFantasyFont( const QString& n );
  void slotEncoding( const QString& n);
    void slotCharset( const QString &n );

private slots:
  void changed();

private:
  void updateGUI();

private:

  KConfig *m_pConfig;
  QString m_groupname;

  QRadioButton* m_pSmall;
  QRadioButton* m_pMedium;
  QRadioButton* m_pLarge;
  QSpinBox* minSizeSB;
  QComboBox* m_pFixed;
  QComboBox* m_pStandard;
  QComboBox* m_pSerif;
  QComboBox* m_pSansSerif;
  QComboBox* m_pCursive;
  QComboBox* m_pFantasy;
  QComboBox* m_pEncoding;
  QComboBox* m_pChset;

  int fSize;
  int fMinSize;
    QMap<QString, QStringList> fontsForCharset;
  QStringList encodings;
  QStringList chSets;
    QString charset;
    QStringList fonts;
    QStringList defaultFonts;
    QString encodingName;
};


class KJavaScriptOptions : public KCModule
{
  Q_OBJECT
public:
  KJavaScriptOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );

  virtual void load();
  virtual void save();
  virtual void defaults();

private slots:
  void changed();
  void importPressed(); 
  void exportPressed();
  void addPressed();
  void changePressed();
  void deletePressed();
  void toggleJavaControls();

private:
  void changeJavaEnabled();
  void changeJavaScriptEnabled();
  void updateDomainList(const QStringList &domainConfig);

  KConfig *m_pConfig;
  QString m_groupname;

  QCheckBox* enableJavaGloballyCB;
  QCheckBox* enableJavaScriptGloballyCB;
  KListView* domainSpecificLV;
  QCheckBox* javaConsoleCB;
  QRadioButton* autoDetectRB;
  QRadioButton* userSpecifiedRB;
  QLineEdit* addArgED;
  KURLRequester* pathED;
  QMap<QListViewItem*, int> javaDomainPolicy;
  QMap<QListViewItem*, int> javaScriptDomainPolicy;
};


#endif		// __HTML_OPTIONS_H__

