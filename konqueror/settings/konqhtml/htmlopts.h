//
//
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998

#ifndef __KMISCHTML_OPTIONS_H
#define __KMISCHTML_OPTIONS_H

#include <qstrlist.h>
#include <qcheckbox.h>
#include <qlineedit.h>

#include <kcmodule.h>


//-----------------------------------------------------------------------------
// The "Misc Options" Tab for the HTML view contains :

// Change cursor over links
// Underline links
// AutoLoad Images
// ... there is room for others :))


#include <qstring.h>
#include <kconfig.h>
class QRadioButton;
class KIntNumInput;

class KMiscHTMLOptions : public KCModule
{
    Q_OBJECT

public:
    KMiscHTMLOptions(KConfig *config, QString group, QWidget *parent = 0L, const char *name = 0L );
    virtual void load();
    virtual void save();
    virtual void defaults();

private slots:
    void changed();

private:
    KConfig* m_pConfig;
    QString  m_groupname;

    QRadioButton *m_pUnderlineRadio[3];
    QCheckBox* cbCursor;
    QCheckBox* m_pAutoLoadImagesCheckBox;
    QCheckBox* m_pEnableFaviconCheckBox;
    QCheckBox* m_pFormCompletionCheckBox;
    KIntNumInput* m_pMaxFormCompletionItems;

};

#endif
