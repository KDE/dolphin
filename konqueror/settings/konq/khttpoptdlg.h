// khttpoptdlg.h - extra HTTP configuration by Jacek Konieczy <jajcus@zeus.polsl.gliwice.pl>
#ifndef __KHTTPOPTDLG_H
#define __KHTTPOPTDLG_H

#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcheckbox.h>
 
#include <kcontrol.h>
class KConfig;
 
extern KConfig *g_pConfig;

/**
*  Dialog for configuring HTTP Options like charset and language negotiation
*  and assuming that file got from HTTP is HTML if no Content-Type is given
*/
class KHTTPOptions : public KConfigWidget
{
Q_OBJECT
  public:
    KHTTPOptions(QWidget *parent = 0L, const char *name = 0L);
    ~KHTTPOptions();

    virtual void loadSettings();
    virtual void saveSettings();
    virtual void applySettings();
    virtual void defaultSettings();
    
  private:
    // Acceptable languages "LANG" - locale selected languages
    QLabel *lb_languages;	
    QLineEdit *le_languages;	

    // Acceptable charsets "CHARSET" - locale selected charset
    QLabel *lb_charsets;	
    QLineEdit *le_charsets;	

    // Assume HTML if mime type is not known
    QCheckBox *cb_assumeHTML;

    QString defaultCharsets;
};

#endif // __KHTTPOPTDLG_H
