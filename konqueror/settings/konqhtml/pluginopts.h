//-----------------------------------------------------------------------------
//
// Plugin Options
//
// (c) 2001, Daniel Naber, based on javaopts.h
//
//-----------------------------------------------------------------------------

#ifndef __PLUGINOPTS_H__
#define __PLUGINOPTS_H__

#include <kcmodule.h>
#include <qmap.h>

class KConfig;
class QCheckBox;

class KPluginOptions : public KCModule
{
    Q_OBJECT

public:
    KPluginOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );

    virtual void load();
    virtual void save();
    virtual void defaults();

private slots:
    void changed();

private:

    KConfig* m_pConfig;
    QString  m_groupname;

    QCheckBox*    enablePluginsGloballyCB;
};

#endif		// __PLUGINOPTS_H__
