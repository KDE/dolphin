// khttpoptdlg.h - extra HTTP configuration by Jacek Konieczy <jajcus@zeus.polsl.gliwice.pl>
#ifndef __KHTTPOPTDLG_H
#define __KHTTPOPTDLG_H


#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qstring.h>

#include <kcmodule.h>
#include <kconfig.h>


/**
*  Dialog for configuring HTTP Options like charset and language negotiation
*  and assuming that file got from HTTP is HTML if no Content-Type is given
*/
class KHTTPOptions : public KCModule
{
Q_OBJECT
  public:
    KHTTPOptions(KConfig *config, QString group, QWidget *parent = 0L, const char *name = 0L);

    virtual void load();
    virtual void save();
    virtual void defaults();

  private:

    KConfig *m_pConfig;
    QString m_groupname;

    // Acceptable languages "LANG" - locale selected languages
    QLabel *lb_languages;	
    QLineEdit *le_languages;	

    // Acceptable charsets "CHARSET" - locale selected charset
    QLabel *lb_charsets;	
    QLineEdit *le_charsets;	

    QString defaultCharsets;

private slots:

  void changed();

};

#endif // __KHTTPOPTDLG_H
