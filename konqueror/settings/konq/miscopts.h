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
// Tree view follows navigation   (David)
// Preferred terminal             (David)
// AutoLoad Images                (Simon)
// Big ToolBar                    (Simon)
// ... there is room for others :))

class KMiscOptions : public KCModule
{
        Q_OBJECT
public:
        KMiscOptions( QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();

private slots:

	void changed();

private:
        QCheckBox *urlpropsbox;
        QCheckBox *treefollowbox;
        QLineEdit *leTerminal;
        QLineEdit *leEditor;
	QCheckBox *m_pHaveBiiigToolBarCheckBox;
};

#endif // __KFM_MISC_OPTIONS_H
