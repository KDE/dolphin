//-----------------------------------------------------------------------------
//
// Plugin Options
//
// (c) 2001, Daniel Naber, based on javaopts.h
// (c) 2000, Stefan Schimanski <1Stein@gmx.de>, Netscape parts
//
//-----------------------------------------------------------------------------

#ifndef __PLUGINOPTS_H__
#define __PLUGINOPTS_H__

#include <qwidget.h>
#include <qmap.h>

class KConfig;
class QCheckBox;

#include <kcmodule.h>
#include "nsconfigwidget.h"

class QProgressDialog;
class KProcIO;

class KPluginOptions : public KCModule
{
    Q_OBJECT

public:
    KPluginOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );
	~KPluginOptions();

    virtual void load();
    virtual void save();
    virtual void defaults();
    QString quickHelp() const;

signals:
    void changed( bool state );

private slots:
    void changed();

private:

    KConfig* m_pConfig;
    QString  m_groupname;

    QCheckBox*    enablePluginsGloballyCB;
 
 protected slots:
  void progress(KProcIO *);
  void change() { change( true ); };
  void change( bool c ) { emit changed(c); m_changed = c; };

  void scan();
  void scanDone();

 private:
  NSConfigWidget *m_widget;
  bool m_changed;
  QProgressDialog *m_progress;

/******************************************************************************/
 protected:
  void dirInit();
  void dirLoad( KConfig *config );
  void dirSave( KConfig *config );

 protected slots:
  void dirBrowse();
  void dirNew();
  void dirRemove();
  void dirUp();
  void dirDown();
  void dirEdited(const QString &);
  void dirSelect( QListBoxItem * );

/******************************************************************************/
 protected:
  void pluginInit();
  void pluginLoad( KConfig *config );
  void pluginSave( KConfig *config );

};

#endif		// __PLUGINOPTS_H__
