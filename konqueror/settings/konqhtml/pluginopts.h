//-----------------------------------------------------------------------------
//
// Plugin Options
//
// (c) 2001, Daniel Naber, based on javaopts.h
//
//-----------------------------------------------------------------------------

#ifndef __PLUGINOPTS_H__
#define __PLUGINOPTS_H__

#include <qwidget.h>
#include <qmap.h>

class KConfig;
class QCheckBox;

#include <kcmodule.h>

class KPluginOptions : public KCModule
{
    Q_OBJECT

public:
    KPluginOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );
	~KPluginOptions();

    virtual void load();
    virtual void save();
    virtual void defaults();

signals:
    void changed( bool state );

private slots:
    void changed();

private:

    KConfig* m_pConfig;
    QString  m_groupname;

    QCheckBox*    enablePluginsGloballyCB;
};

#endif		// __PLUGINOPTS_H__
