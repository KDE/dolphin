//
//
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998

#ifndef __KFM_MISC_OPTIONS_H
#define __KFM_MISC_OPTIONS_H

#include <qstrlist.h>
#include <qcheckbox.h>
#include <qlineedit.h>

#include <kcmodule.h>


//-----------------------------------------------------------------------------
// The "Misc Options" Tab contains :

// Allow per-url settings         (Sven)
// Preferred terminal             (David)
// Big ToolBar                    (Simon)  => to move to options menu
// ... there is room for others :))


#include <qstring.h>
#include <kconfig.h>


class KMiscOptions : public KCModule
{
        Q_OBJECT
public:
        KMiscOptions(KConfig *config, QString group, QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();

private slots:

	void changed();

private:

        KConfig *g_pConfig;
	QString groupname;

        QCheckBox *urlpropsbox;
        QLineEdit *leTerminal;
	QCheckBox *m_pHaveBiiigToolBarCheckBox;
	QCheckBox *m_pConfirmDestructive;
};

#endif // __KFM_MISC_OPTIONS_H
