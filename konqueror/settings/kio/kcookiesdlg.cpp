// kcookiesdlg.cpp - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#include <qlayout.h> 
#include <qradiobutton.h>

#include <klocale.h>
#include <kapp.h>
#include <kbuttonbox.h>
#include <kmessagebox.h>

#include "kcookiesdlg.h"

#define ROW_ENABLE_COOKIES 1
#define ROW_DEFAULT_ACCEPT 3
#define ROW_CHANGE_DOMAIN 6
#define ROW_BOTTOM 7

KCookiesOptions::KCookiesOptions(QWidget *parent, const char *name)
  : KCModule(parent, name)
{

  QGridLayout *lay = new QGridLayout(this,ROW_BOTTOM+1,5,10,5);
  lay->addRowSpacing(0,10);
  lay->addRowSpacing(2,10);
  lay->addRowSpacing(4,10);
  lay->addRowSpacing(5,30); // no idea why 10 isn't enough... David.
  lay->addColSpacing(0,10);
  lay->addColSpacing(2,10);
  lay->addColSpacing(4,10);
  
  lay->setRowStretch(0,0);
  lay->setRowStretch(1,0); // ROW_ENABLE_COOKIES
  lay->setRowStretch(2,1);
  lay->setRowStretch(3,1); // ROW_DEFAULT_ACCEPT
  lay->setRowStretch(4,1);
  lay->setRowStretch(5,0);
  lay->setRowStretch(6,1); // ROW_CHANGE_DOMAIN
  lay->setRowStretch(7,10); // ROW_BOTTOM
  
  lay->setColStretch(0,0);
  lay->setColStretch(1,1);
  lay->setColStretch(2,0);
  lay->setColStretch(3,1);
  lay->setColStretch(4,0);
  
  cb_enableCookies = new QCheckBox( i18n("&Enable Cookies"), this );
  connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changeCookiesEnabled() ) );
  connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changed() ) );
  lay->addWidget(cb_enableCookies,ROW_ENABLE_COOKIES,1);

  {
    QButtonGroup *bg = new QButtonGroup( i18n("Default accept policy"), this );
    bg1 = bg;
    bg->setExclusive( TRUE );
    QGridLayout *bgLay = new QGridLayout(bg,3,3,10,5);
    bgLay->addRowSpacing(0,10);
    bgLay->addRowSpacing(2,5);
    bgLay->setRowStretch(0,0);
    bgLay->setRowStretch(1,0);
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));

    rb_gbPolicyAccept = new QRadioButton( i18n("Accept"), bg );
    bgLay->addWidget(rb_gbPolicyAccept, 1, 0);

    rb_gbPolicyAsk = new QRadioButton( i18n("Ask"), bg );
    bgLay->addWidget(rb_gbPolicyAsk, 1, 1);

    rb_gbPolicyReject = new QRadioButton( i18n("Reject"), bg );
    bgLay->addWidget(rb_gbPolicyReject, 1, 2);

    bgLay->activate();
    lay->addWidget(bg, ROW_DEFAULT_ACCEPT+1, ROW_DEFAULT_ACCEPT);
  }

  // CREATE SPLIT LIST BOX
  wList = new KSplitList( this );
  wList->setMinimumHeight(80);
  lay->addMultiCellWidget( wList, ROW_DEFAULT_ACCEPT+1, ROW_BOTTOM, 1, 1 );

  // associated label (has to be _after_)
  wListLabel = new QLabel( wList, i18n("Domain specific settings:"), this );
  lay->addWidget( wListLabel, ROW_DEFAULT_ACCEPT, 1 );
  wListLabel->setFixedHeight( wListLabel->sizeHint().height() );

  connect( wList, SIGNAL( highlighted( int ) ), SLOT( updateDomain( int ) ) );
  connect( wList, SIGNAL( selected( int ) ), SLOT( updateDomain( int ) ) );
  connect( wList, SIGNAL( selected( int ) ), this, SLOT(changed() ) );
  {
    QButtonGroup *bg = new QButtonGroup( i18n("Change domain accept policy"), this );
    bg2 = bg;
    bg->setExclusive( TRUE );
    QGridLayout *bgLay = new QGridLayout(bg,6,3,10,5);
    bgLay->addRowSpacing(0,10);
    bgLay->addRowSpacing(2,10);
    bgLay->setRowStretch(0,0);
    bgLay->setRowStretch(1,0);
    bgLay->setRowStretch(2,1);
    bgLay->setRowStretch(3,0);
    bgLay->setRowStretch(4,1);
    bgLay->setRowStretch(5,0);
    connect(bg, SIGNAL(clicked(int)), this, SLOT(changed()));

    le_domain = new QLineEdit(bg);
    bgLay->addMultiCellWidget(le_domain,1,1,0,2);
              
    rb_domPolicyAccept = new QRadioButton( i18n("Accept"), bg );
    bgLay->addWidget(rb_domPolicyAccept, 3, 0);
    connect(rb_domPolicyAccept, SIGNAL(clicked()), this, SLOT(changed()));

    rb_domPolicyAsk = new QRadioButton( i18n("Ask"), bg );
    bgLay->addWidget(rb_domPolicyAsk, 3, 1);
    connect(rb_domPolicyAsk, SIGNAL(clicked()), this, SLOT(changed()));

    rb_domPolicyReject = new QRadioButton( i18n("Reject"), bg );
    rb_domPolicyAsk->setChecked( true );
    bgLay->addWidget(rb_domPolicyReject, 3, 2);
    connect(rb_domPolicyReject, SIGNAL(clicked()), this, SLOT(changed()));

    KButtonBox *bbox = new KButtonBox( bg );
    bbox->addStretch( 20 );
        
    b0 = bbox->addButton( i18n("Change") );
    connect( b0, SIGNAL( clicked() ), this, SLOT( changePressed() ) );
    connect( b0, SIGNAL( clicked() ), this, SLOT( changed() ) );
                
    bbox->addStretch( 10 );
                    
    b1 = bbox->addButton( i18n("Delete") );
    connect( b1, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );
    connect( b1, SIGNAL( clicked() ), this, SLOT( deleted() ) );
                            
    bbox->addStretch( 20 );
                                
    bbox->layout();
    bgLay->addMultiCellWidget( bbox, 5,5,0,2);
                                            
    lay->addWidget(bg,ROW_CHANGE_DOMAIN,3);
  }

  lay->activate();

  // finally read the options
  load();
}

KCookiesOptions::~KCookiesOptions()
{
}

enum KCookieAdvice {
    KCookieDunno=0,
    KCookieAccept,
    KCookieReject,
    KCookieAsk
};

static const char *adviceToStr(KCookieAdvice _advice)
{
    switch( _advice )
    {
    case KCookieAccept: return "Accept";
    case KCookieReject: return "Reject";
    case KCookieAsk: return "Ask";
    default: return "Dunno";
    }
}

static KCookieAdvice strToAdvice(const char *_str)
{
    if (!_str)
        return KCookieDunno;
        
    if (strcasecmp(_str, "Accept") == 0)
	return KCookieAccept;
    else if (strcasecmp(_str, "Reject") == 0)
	return KCookieReject;
    else if (strcasecmp(_str, "Ask") == 0)
	return KCookieAsk;

    return KCookieDunno;	
}

static void splitDomainAdvice(const char *configStr, 
                              QString &domain,
                              KCookieAdvice &advice)
{
    QString tmp(configStr);
    int splitIndex = tmp.find(':');
    if ( splitIndex == -1)
    {
        domain = configStr;
        advice = KCookieDunno;
    }
    else
    {
        domain = tmp.left(splitIndex);
        advice = strToAdvice( tmp.mid( splitIndex+1, tmp.length()).ascii() );
    }
}                            

void KCookiesOptions::removeDomain(const char *domain)
{
    const char *configStr = 0L;
    QString searchFor(domain);
    searchFor += ":";
    
    for( configStr = domainConfig.first();
         configStr != 0;
         configStr = domainConfig.next())
    {
       if (strncasecmp(configStr, searchFor.data(), searchFor.length()) == 0)
       {
           domainConfig.removeRef(configStr);
           return;
       }
    }
}

void KCookiesOptions::changePressed()
{
    QString domain = le_domain->text();
    const char *advice;

    if (domain.isEmpty())
    {
    	KMessageBox::information( 0, i18n("You must enter a domain first !"));
        return;
    }

    if (rb_domPolicyAccept->isChecked())
        advice = adviceToStr(KCookieAccept);
    else if (rb_domPolicyReject->isChecked())
        advice = adviceToStr(KCookieReject);
    else
        advice = adviceToStr(KCookieAsk);
    
    QString configStr(domain);
    
    if (configStr[0] != '.')
        configStr = "." + configStr;    

    removeDomain(configStr.data());
    
    configStr += ":";
    
    configStr += advice;

    domainConfig.inSort(configStr.data());
    
    int index = domainConfig.find(configStr.data());

    updateDomainList();
    
    wList->setCurrentItem(index);
}

void KCookiesOptions::deletePressed()
{
    QString domain(le_domain->text());
    
    if (domain[0] != '.')
        domain = "." + domain;

    removeDomain(domain.data());
    updateDomainList();

    le_domain->setText("");
    rb_domPolicyAccept->setChecked( false );
    rb_domPolicyReject->setChecked( false );
    rb_domPolicyAsk->setChecked( true );

    if (wList->count() > 0)
        wList->setCurrentItem(0);
}

void KCookiesOptions::updateDomain(int index)
{
  const char *configStr;
  QString domain;
  KCookieAdvice advice;

  if ((index < 0) || (index >= (int) domainConfig.count()))
      return;
  
  int id = 0;

  for( configStr = domainConfig.first();
       configStr != 0;
       configStr = domainConfig.next())
  {
       if (id == index)
           break; 
       id++;                                                                                                                                 
  }
  if (!configStr)
      return;

  splitDomainAdvice(configStr, domain, advice);

  le_domain->setText(domain);  
  rb_domPolicyAccept->setChecked( advice == KCookieAccept );
  rb_domPolicyReject->setChecked( advice == KCookieReject );
  rb_domPolicyAsk->setChecked( (advice != KCookieAccept) &&
                              (advice != KCookieReject) );
}

void KCookiesOptions::changeCookiesEnabled()
{
  bool enabled = cb_enableCookies->isChecked();
    
  rb_gbPolicyAccept->setEnabled( enabled);
  rb_gbPolicyReject->setEnabled( enabled);
  rb_gbPolicyAsk->setEnabled( enabled );

  rb_domPolicyAccept->setEnabled( enabled);
  rb_domPolicyReject->setEnabled( enabled);
  rb_domPolicyAsk->setEnabled( enabled );
  bg1->setEnabled( enabled );
  bg2->setEnabled( enabled );

  b0->setEnabled( enabled );
  b1->setEnabled( enabled );

  wListLabel->setEnabled( enabled );    
  wList->setEnabled( enabled );    
  le_domain->setEnabled( enabled );
}

void KCookiesOptions::updateDomainList()
{
  wList->setAutoUpdate(false);
  wList->clear();

  int id = 0;

  for( const char *domain = domainConfig.first();
       domain != 0;
       domain = domainConfig.next())
  {
       KSplitListItem *sli = new KSplitListItem( domain, id);
       connect( wList, SIGNAL( newWidth( int ) ),
	       sli, SLOT( setWidth( int ) ) );
       sli->setWidth(wList->width());
       wList->inSort( sli );
       id++;                                                                                                                                 
  }
  wList->setAutoUpdate(true);
  wList->update();
}

void KCookiesOptions::load()
{
  KConfig *g_pConfig = new KConfig("kioslaverc", false, false);

  QString tmp;
  g_pConfig->setGroup( "Browser Settings/HTTP" );

  tmp = g_pConfig->readEntry("CookieGlobalAdvice", "Ask");
  KCookieAdvice globalAdvice = strToAdvice(tmp.data());

  cb_enableCookies->setChecked( g_pConfig->readBoolEntry( "Cookies", true ));

  rb_gbPolicyAccept->setChecked( globalAdvice == KCookieAccept );
  rb_gbPolicyReject->setChecked( globalAdvice == KCookieReject );
  rb_gbPolicyAsk->setChecked( (globalAdvice != KCookieAccept) &&
                              (globalAdvice != KCookieReject) );

  (void) g_pConfig->readListEntry("CookieDomainAdvice", domainConfig);

  updateDomainList();
  changeCookiesEnabled();
  
  delete g_pConfig;
}

void KCookiesOptions::save()
{
  KConfig *g_pConfig = new KConfig("kioslaverc", false, false);

  const char *advice;
  g_pConfig->setGroup( "Browser Settings/HTTP" );
  g_pConfig->writeEntry( "Cookies", cb_enableCookies->isChecked() );

  if (rb_gbPolicyAccept->isChecked())
      advice = adviceToStr(KCookieAccept);
  else if (rb_gbPolicyReject->isChecked())
      advice = adviceToStr(KCookieReject);
  else
      advice = adviceToStr(KCookieAsk);
  g_pConfig->writeEntry("CookieGlobalAdvice", advice);
  g_pConfig->writeEntry("CookieDomainAdvice", domainConfig);

  g_pConfig->sync();

  delete g_pConfig;
}


void KCookiesOptions::defaults()
{
  cb_enableCookies->setChecked( true );

  rb_gbPolicyAccept->setChecked( false );
  rb_gbPolicyReject->setChecked( false );
  rb_gbPolicyAsk->setChecked( true );
  domainConfig.clear();
  updateDomainList();
  le_domain->setText("");  
  rb_domPolicyAccept->setChecked( false );
  rb_domPolicyReject->setChecked( false );
  rb_domPolicyAsk->setChecked( true );

  changeCookiesEnabled();
}


void KCookiesOptions::changed()
{
  emit KCModule::changed(true);
}


#include "kcookiesdlg.moc"
