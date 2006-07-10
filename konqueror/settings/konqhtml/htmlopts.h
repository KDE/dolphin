//
//
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998

#ifndef __KMISCHTML_OPTIONS_H
#define __KMISCHTML_OPTIONS_H

#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>


//-----------------------------------------------------------------------------
// The "Misc Options" Tab for the HTML view contains :

// Change cursor over links
// Underline links
// AutoLoad Images
// ... there is room for others :))


#include <QString>
#include <kcmodule.h>
#include <kconfig.h>
class QRadioButton;
class KIntNumInput;

class KMiscHTMLOptions : public KCModule
{
    Q_OBJECT

public:
    KMiscHTMLOptions( QWidget *parent, const QStringList& );
    ~KMiscHTMLOptions();
    virtual void load();
    virtual void save();
    virtual void defaults();

private Q_SLOTS:
    void slotChanged();
    void launchAdvancedTabDialog();

private:
    KSharedConfig::Ptr m_pConfig;
    QString  m_groupname;

    QComboBox* m_pUnderlineCombo;
    QComboBox* m_pAnimationsCombo;
    QCheckBox* m_cbCursor;
    QCheckBox* m_pAutoLoadImagesCheckBox;
    QCheckBox* m_pUnfinishedImageFrameCheckBox;
    QCheckBox* m_pAutoRedirectCheckBox;
    QCheckBox* m_pOpenMiddleClick;
    QCheckBox* m_pBackRightClick;
    QCheckBox* m_pShowMMBInTabs;
    QCheckBox* m_pFormCompletionCheckBox;
    QCheckBox* m_pDynamicTabbarHide;
    QCheckBox* m_pAdvancedAddBookmarkCheckBox;
    QCheckBox* m_pOnlyMarkedBookmarksCheckBox;
    KIntNumInput* m_pMaxFormCompletionItems;
};

#endif
