/***********************************************************************
 *
 *  kftabdlg.cpp
 *
 **********************************************************************/

#include <QButtonGroup>
#include <QRadioButton>
#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QWhatsThis>
#include <QToolTip>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>

#include <kcalendarsystem.h>
#include <kglobal.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kregexpeditorinterface.h>
#include <kparts/componentfactory.h>
#include <kstandarddirs.h>
#include "kquery.h"
#include "kftabdlg.h"

// Static utility functions
static void save_pattern(QComboBox *, const QString &, const QString &);

#define SPECIAL_TYPES 7

struct LessMimeType_ByComment
{
    bool operator()(const KMimeType::Ptr& lhs, const KMimeType::Ptr& rhs) const
    {
        return lhs->comment() < rhs->comment();
    }
};

KfindTabWidget::KfindTabWidget(QWidget *parent, const char *name)
  : QTabWidget( parent ), regExpDialog(0)
{
    setObjectName( name );

    // This validator will be used for all numeric edit fields
    //KDigitValidator *digitV = new KDigitValidator(this);

    // ************ Page One ************

    pages[0] = new QWidget( this );
    pages[0]->setObjectName( "page1" );

    nameBox = new KComboBox(pages[0]);
    nameBox->setEditable( true );
    nameBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);  // allow smaller than widest entry
    QLabel * namedL = new QLabel(i18n("&Named:"), pages[0]);
    namedL->setBuddy( nameBox );
    namedL->setObjectName( "named" );
    namedL->setToolTip( i18n("You can use wildcard matching and \";\" for separating multiple names") );
    dirBox  = new KComboBox(pages[0]);
    dirBox->setEditable( true );
    dirBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);  // allow smaller than widest entry
    QLabel * lookinL = new QLabel(i18n("Look &in:"), pages[0]);
    lookinL->setBuddy( dirBox );
    lookinL->setObjectName( "lookin" );
    subdirsCb  = new QCheckBox(i18n("Include &subfolders"), pages[0]);
    caseSensCb  = new QCheckBox(i18n("Case s&ensitive search"), pages[0]);
    browseB    = new QPushButton(i18n("&Browse..."), pages[0]);
    useLocateCb = new QCheckBox(i18n("&Use files index"), pages[0]);

    // Setup

    subdirsCb->setChecked(true);
    caseSensCb->setChecked(false);
    useLocateCb->setChecked(false);
    if(KStandardDirs::findExe("locate").isEmpty())
    	useLocateCb->setEnabled(false);

    nameBox->setDuplicatesEnabled(false);
    nameBox->setFocus();
    dirBox->setDuplicatesEnabled(false);

    nameBox->setInsertPolicy(QComboBox::AtTop);
    dirBox->setInsertPolicy(QComboBox::AtTop);

    const QString nameWhatsThis
      = i18n("<qt>Enter the filename you are looking for. <br>"
	     "Alternatives may be separated by a semicolon \";\".<br>"
	     "<br>"
	     "The filename may contain the following special characters:"
	     "<ul>"
	     "<li><b>?</b> matches any single character</li>"
	     "<li><b>*</b> matches zero or more of any characters</li>"
	     "<li><b>[...]</b> matches any of the characters in braces</li>"
	     "</ul>"
	     "<br>"
	     "Example searches:"
	     "<ul>"
	     "<li><b>*.kwd;*.txt</b> finds all files ending with .kwd or .txt</li>"
	     "<li><b>go[dt]</b> finds god and got</li>"
	     "<li><b>Hel?o</b> finds all files that start with \"Hel\" and end with \"o\", "
	     "having one character in between</li>"
	     "<li><b>My Document.kwd</b> finds a file of exactly that name</li>"
	     "</ul></qt>");
    nameBox->setWhatsThis(nameWhatsThis);
    namedL->setWhatsThis(nameWhatsThis);
    const QString whatsfileindex
        = i18n("<qt>This lets you use the files' index created by the <i>slocate</i> "
               "package to speed-up the search; remember to update the index from time to time "
               "(using <i>updatedb</i>)."
               "</qt>");
    useLocateCb->setWhatsThis(whatsfileindex);

    // Layout

    QGridLayout *grid = new QGridLayout( pages[0] );
    grid->setMargin( KDialog::marginHint() );
    grid->setSpacing( KDialog::spacingHint() );
    QBoxLayout *subgrid = new QVBoxLayout();
    grid->addWidget( namedL, 0, 0 );
    grid->addWidget( nameBox, 0, 1, 1, 3 );
    grid->addWidget( lookinL, 1, 0 );
    grid->addWidget( dirBox, 1, 1 );
    grid->addWidget( browseB, 1, 2);
    grid->setColumnStretch(1,1);
    grid->addLayout( subgrid, 2, 1, 1, 2 );
    subgrid->addWidget( subdirsCb );
    subgrid->addWidget( caseSensCb);
    subgrid->addWidget( useLocateCb );
    subgrid->addStretch(1);

    // Signals

    connect( browseB, SIGNAL(clicked()),
             this, SLOT(getDirectory()) );

    connect( nameBox, SIGNAL(activated(int)),
             this, SIGNAL(startSearch()));

    // ************ Page Two

    pages[1] = new QWidget( this );
    pages[1]->setObjectName( "page2" );

    findCreated =  new QCheckBox(i18n("Find all files created or &modified:"), pages[1]);
    bg  = new QButtonGroup();
    rb[0] = new QRadioButton(i18n("&between"), pages[1] );
    rb[1] = new QRadioButton(i18n("&during the previous"), pages[1] );
    andL = new QLabel(i18n("and"), pages[1]);
    andL->setObjectName( "and" );
    betweenType = new KComboBox( pages[1] );
    betweenType->setEditable( false );
    betweenType->setObjectName( "comboBetweenType" );
    betweenType->addItem(i18n("minute(s)"));
    betweenType->addItem(i18n("hour(s)"));
    betweenType->addItem(i18n("day(s)"));
    betweenType->addItem(i18n("month(s)"));
    betweenType->addItem(i18n("year(s)"));
    betweenType->setCurrentIndex(1);


    QDate dt = KGlobal::locale()->calendar()->addYears(QDate::currentDate(), -1);

    fromDate = new KDateCombo(dt, pages[1] );
    fromDate->setObjectName( "fromDate" );
    toDate = new KDateCombo(pages[1] );
    toDate->setObjectName( "toDate" );
    timeBox = new QSpinBox( pages[1] );
    timeBox->setRange( 1, 60 );
    timeBox->setSingleStep( 1 );
    timeBox->setObjectName( "timeBox" );

    sizeBox =new KComboBox( pages[1] );
    sizeBox->setEditable( false );
    sizeBox->setObjectName( "sizeBox" );
    QLabel * sizeL   =new QLabel(i18n("File &size is:"), pages[1]);
    sizeL->setBuddy( sizeBox );
    sizeEdit=new QSpinBox(pages[1] );
    sizeEdit->setRange( 0, INT_MAX );
    sizeEdit->setSingleStep( 1 );
    sizeEdit->setObjectName( "sizeEdit" );
    sizeEdit->setValue(1);
    sizeUnitBox =new KComboBox( pages[1] );
    sizeUnitBox->setEditable( false );
    sizeUnitBox->setObjectName( "sizeUnitBox" );

    m_usernameBox = new KComboBox( pages[1] );
    m_usernameBox->setEditable( true );
    m_usernameBox->setObjectName( "m_combo1");
    QLabel *usernameLabel= new QLabel(i18n("Files owned by &user:"),pages[1]);
    usernameLabel->setBuddy( m_usernameBox );
    m_groupBox = new KComboBox( pages[1] );
    m_groupBox->setEditable( true );
    m_groupBox->setObjectName( "m_combo2" );
    QLabel *groupLabel= new QLabel(i18n("Owned by &group:"),pages[1]);
    groupLabel->setBuddy( m_groupBox );

    sizeBox ->addItem( i18n("(none)") );
    sizeBox ->addItem( i18n("At Least") );
    sizeBox ->addItem( i18n("At Most") );
    sizeBox ->addItem( i18n("Equal To") );

    sizeUnitBox ->addItem( i18n("Bytes") );
    sizeUnitBox ->addItem( i18n("KB") );
    sizeUnitBox ->addItem( i18n("MB") );
    sizeUnitBox ->addItem( i18n("GB") );
    sizeUnitBox ->setCurrentIndex(1);

    int tmp = sizeEdit->fontMetrics().width(" 000000000 ");
    sizeEdit->setMinimumSize(tmp, sizeEdit->sizeHint().height());

    m_usernameBox->setDuplicatesEnabled(false);
    m_groupBox->setDuplicatesEnabled(false);
    m_usernameBox->setInsertPolicy(QComboBox::AtTop);
    m_groupBox->setInsertPolicy(QComboBox::AtTop);


    // Setup
    timeBox->setButtonSymbols(QSpinBox::PlusMinus);
    rb[0]->setChecked(true);
    bg->addButton( rb[0] );
    bg->addButton( rb[1] );

    // Layout

    QGridLayout *grid1 = new QGridLayout( pages[1] );
    grid1->setMargin( KDialog::marginHint() );
    grid1->setSpacing( KDialog::spacingHint() );

    grid1->addWidget(findCreated, 0, 0, 1, 3 );
    grid1->addItem( new QSpacerItem(KDialog::spacingHint(), 0), 0, 0 );

    grid1->addWidget(rb[0], 1, 1 );
    grid1->addWidget(fromDate, 1, 2 );
    grid1->addWidget(andL, 1, 3, Qt::AlignHCenter );
    grid1->addWidget(toDate, 1, 4 );

    grid1->addWidget(rb[1], 2, 1 );
    grid1->addWidget(timeBox, 2, 2, 1, 2);
    grid1->addWidget(betweenType, 2, 4 );

    grid1->addWidget(sizeL,3,0,1,2);
    grid1->addWidget(sizeBox,3,2);
    grid1->addWidget(sizeEdit,3,3);
    grid1->addWidget(sizeUnitBox,3,4);

    grid1->addWidget(usernameLabel,4,0,1,2);
    grid1->addWidget(m_usernameBox,4,2);
    grid1->addWidget(groupLabel,4,3);
    grid1->addWidget(m_groupBox,4,4);

    for (int c=1; c<=4; c++)
       grid1->setColumnStretch(c,1);

    grid1->setRowStretch(6,1);

    // Connect
    connect( findCreated, SIGNAL(toggled(bool)),  SLOT(fixLayout()) );
    connect( bg, SIGNAL(buttonClicked(QAbstractButton*)), this,  SLOT(fixLayout()) );
    connect( sizeBox, SIGNAL(highlighted(int)), this, SLOT(slotSizeBoxChanged(int)));


    // ************ Page Three

    pages[2] = new QWidget( this );
    pages[2]->setObjectName( "page3" );

    typeBox =new KComboBox( pages[2] );
    typeBox->setEditable( false );
    typeBox->setObjectName( "typeBox" );
    typeBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);  // allow smaller than widest entry
    QLabel * typeL   =new QLabel( i18n("File &type:"), pages[2] );
    typeL->setBuddy( typeBox );
    textEdit=new KLineEdit(pages[2]);
    textEdit->setObjectName( "textEdit" );
    QLabel * textL   =new QLabel(i18n("C&ontaining text:"), pages[2]);
    textL->setBuddy( textEdit );

    connect( textEdit, SIGNAL(returnPressed(const QString &)), SIGNAL( startSearch()));

    const QString containingtext
      = i18n("<qt>If specified, only files that contain this text"
	      " are found. Note that not all file types from the list"
		  " above are supported. Please refer to the documentation"
		  " for a list of supported file types."
	      "</qt>");
    textEdit->setToolTip(containingtext);
    textL->setWhatsThis(containingtext);

    caseContextCb  =new QCheckBox(i18n("Case s&ensitive"), pages[2]);
    binaryContextCb  =new QCheckBox(i18n("Include &binary files"), pages[2]);
    regexpContentCb  =new QCheckBox(i18n("Regular e&xpression"), pages[2]);

    const QString binaryTooltip
      = i18n("<qt>This lets you search in any type of file, "
       "even those that usually do not contain text (for example "
	   "program files and images).</qt>");
    binaryContextCb->setToolTip(binaryTooltip);

    QPushButton* editRegExp = 0;
    if ( !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty() ) {
        // The editor is available, so lets use it.
        editRegExp = new QPushButton(i18n("&Edit..."), pages[2]);
        editRegExp->setObjectName( "editRegExp" );
    }

    metainfokeyEdit=new KLineEdit(pages[2]);
    metainfoEdit=new KLineEdit(pages[2]);
    QLabel * textMetaInfo = new QLabel(i18n("fo&r:"), pages[2]);
    textMetaInfo->setBuddy( metainfoEdit );
    QLabel * textMetaKey = new QLabel(i18n("Search &metainfo sections:"), pages[2]);
    textMetaKey->setBuddy( metainfokeyEdit );

    // Setup
    typeBox->addItem(i18n("All Files & Folders"));
    typeBox->addItem(i18n("Files"));
    typeBox->addItem(i18n("Folders"));
    typeBox->addItem(i18n("Symbolic Links"));
    typeBox->addItem(i18n("Special Files (Sockets, Device Files, ...)"));
    typeBox->addItem(i18n("Executable Files"));
    typeBox->addItem(i18n("SUID Executable Files"));
    typeBox->addItem(i18n("All Images"));
    typeBox->addItem(i18n("All Video"));
    typeBox->addItem(i18n("All Sounds"));

    initMimeTypes();
    initSpecialMimeTypes();

    for ( KMimeType::List::ConstIterator it = m_types.begin();
          it != m_types.end(); ++it )
    {
      KMimeType::Ptr typ = *it;
      typeBox->addItem(typ->pixmap( K3Icon::Small ), typ->comment());
    }

    if ( editRegExp ) {
      // The editor was available, so lets use it.
      connect( regexpContentCb, SIGNAL(toggled(bool) ), editRegExp, SLOT(setEnabled(bool)) );
      editRegExp->setEnabled(false);
      connect( editRegExp, SIGNAL(clicked()), this, SLOT( slotEditRegExp() ) );
    }
    else
        regexpContentCb->hide();

    // Layout
    tmp = sizeEdit->fontMetrics().width(" 00000 ");
    sizeEdit->setMinimumSize(tmp, sizeEdit->sizeHint().height());

    QGridLayout *grid2 = new QGridLayout( pages[2] );
    grid2->setMargin( KDialog::marginHint() );
    grid2->setSpacing( KDialog::spacingHint() );
    grid2->addWidget( typeL, 0, 0 );
    grid2->addWidget( textL, 1, 0 );
    grid2->addWidget( typeBox, 0, 1, 1, 3 );
    grid2->addWidget( textEdit, 1, 1, 1, 3 );
    grid2->addWidget( regexpContentCb, 2, 2);
    grid2->addWidget( caseContextCb, 2, 1 );
    grid2->addWidget( binaryContextCb, 3, 1);

    grid2->addWidget( textMetaKey, 4, 0 );
    grid2->addWidget( metainfokeyEdit, 4, 1 );
    grid2->addWidget( textMetaInfo, 4, 2, Qt::AlignHCenter  );
    grid2->addWidget( metainfoEdit, 4, 3 );

    metainfokeyEdit->setText("*");

    if ( editRegExp ) {
      // The editor was available, so lets use it.
      grid2->addWidget( editRegExp, 2, 3 );
    }

    addTab( pages[0], i18n("Name/&Location") );
    addTab( pages[2], i18n("C&ontents") );
    addTab( pages[1], i18n("&Properties") );


    // Setup
    const QString whatsmetainfo
      = i18n("<qt>Search within files' specific comments/metainfo<br>"
	     "These are some examples:<br>"
	     "<ul>"
	     "<li><b>Audio files (mp3...)</b> Search in id3 tag for a title, an album</li>"
	     "<li><b>Images (png...)</b> Search images with a special resolution, comment...</li>"
	     "</ul>"
	     "</qt>");
    const QString whatsmetainfokey
      = i18n("<qt>If specified, search only in this field<br>"
	     "<ul>"
	     "<li><b>Audio files (mp3...)</b> This can be Title, Album...</li>"
	     "<li><b>Images (png...)</b> Search only in Resolution, Bitdepth...</li>"
	     "</ul>"
	     "</qt>");
    textMetaInfo->setWhatsThis(whatsmetainfo);
    metainfoEdit->setToolTip(whatsmetainfo);
    textMetaKey->setWhatsThis(whatsmetainfokey);
    metainfokeyEdit->setToolTip(whatsmetainfokey);


    fixLayout();
    loadHistory();
}

KfindTabWidget::~KfindTabWidget()
{
  delete pages[0];
  delete pages[1];
  delete pages[2];
  delete bg;
}

void KfindTabWidget::setURL( const KUrl & url )
{
  KConfig *conf = KGlobal::config();
  conf->setGroup("History");
  m_url = url;
  QStringList sl = conf->readPathListEntry("Directories");
  dirBox->clear(); // make sure there is no old Stuff in there

  if(!sl.isEmpty()) {
    dirBox->addItems(sl);
    // If the _searchPath already exists in the list we do not
    // want to add it again
    int indx = sl.indexOf(m_url.url());
    if(indx == -1)
      dirBox->addItem(m_url.url(), 0); // make it the first one
    else
      dirBox->setCurrentIndex(indx);
  }
  else {
    QDir m_dir("/lib");
    dirBox ->addItem( m_url.url() );
    dirBox ->addItem( "file:" + QDir::homePath() );
    dirBox ->addItem( "file:/" );
    dirBox ->addItem( "file:/usr" );
    if (m_dir.exists())
      dirBox ->addItem( "file:/lib" );
    dirBox ->addItem( "file:/home" );
    dirBox ->addItem( "file:/etc" );
    dirBox ->addItem( "file:/var" );
    dirBox ->addItem( "file:/mnt" );
  }
}

void KfindTabWidget::initMimeTypes()
{
    KMimeType::List sortedList;
    foreach ( KMimeType::Ptr type, KMimeType::allMimeTypes() )
    {
      if ( (!type->comment().isEmpty())
           && (!type->name().startsWith("kdedevice/"))
           && (!type->name().startsWith("all/")) )
        sortedList.append(type);
    }
    qSort( sortedList.begin(), sortedList.end(), LessMimeType_ByComment() );
    m_types += sortedList;
}

void KfindTabWidget::initSpecialMimeTypes()
{
    KMimeType::List tmp = KMimeType::allMimeTypes();

    for ( KMimeType::List::ConstIterator it = tmp.begin(); it != tmp.end(); ++it )
    {
	  const KMimeType* type = (*it).data();

      if(!type->comment().isEmpty()) {
        if(type->name().startsWith("image/"))
           m_ImageTypes.append(type->name());
        else if(type->name().startsWith("video/"))
          m_VideoTypes.append(type->name());
        else if(type->name().startsWith("audio/"))
          m_AudioTypes.append(type->name());
      }
    }
}

void KfindTabWidget::saveHistory()
{
  save_pattern(nameBox, "History", "Patterns");
  save_pattern(dirBox, "History", "Directories");
}

void KfindTabWidget::loadHistory()
{
  // Load pattern history
  KConfig *conf = KGlobal::config();
  conf->setGroup("History");
  QStringList sl = conf->readEntry("Patterns", QStringList());
  if(!sl.isEmpty())
    nameBox->addItems(sl);
  else
    nameBox->addItem("*");

  sl = conf->readPathListEntry("Directories");
  if(!sl.isEmpty()) {
    dirBox->addItems(sl);
    // If the _searchPath already exists in the list we do not
    // want to add it again
    int indx = sl.indexOf(m_url.url());
    if(indx == -1)
      dirBox->addItem(m_url.url(), 0); // make it the first one
    else
      dirBox->setCurrentIndex(indx);
  }
  else {
    QDir m_dir("/lib");
    dirBox ->addItem( m_url.url() );
    dirBox ->addItem( "file:" + QDir::homePath() );
    dirBox ->addItem( "file:/" );
    dirBox ->addItem( "file:/usr" );
    if (m_dir.exists())
      dirBox ->addItem( "file:/lib" );
    dirBox ->addItem( "file:/home" );
    dirBox ->addItem( "file:/etc" );
    dirBox ->addItem( "file:/var" );
    dirBox ->addItem( "file:/mnt" );
  }
}

void KfindTabWidget::slotEditRegExp()
{
  if ( ! regExpDialog )
    regExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString(), this );

  KRegExpEditorInterface *iface = dynamic_cast<KRegExpEditorInterface *>( regExpDialog );
  if ( !iface )
       return;

  iface->setRegExp( textEdit->text() );
  bool ok = regExpDialog->exec();
  if ( ok )
    textEdit->setText( iface->regExp() );
}

void KfindTabWidget::setFocus()
{
  nameBox->setFocus();
  nameBox->lineEdit()->selectAll();
}

void KfindTabWidget::slotSizeBoxChanged(int index)
{
  sizeEdit->setEnabled((bool)(index != 0));
  sizeUnitBox->setEnabled((bool)(index != 0));
}

void KfindTabWidget::setDefaults()
{
    QDate dt = KGlobal::locale()->calendar()->addYears(QDate::currentDate(), -1);

    fromDate ->setDate(dt);
    toDate ->setDate(QDate::currentDate());

    timeBox->setValue(1);
    betweenType->setCurrentIndex(1);

    typeBox ->setCurrentIndex(0);
    sizeBox ->setCurrentIndex(0);
    sizeUnitBox ->setCurrentIndex(1);
    sizeEdit->setValue(1);
}

/*
  Checks if dates are correct and popups a error box
  if they are not.
*/
bool KfindTabWidget::isDateValid()
{
  // All files
  if ( !findCreated->isChecked() ) return true;

  if (rb[1]->isChecked())
  {
    if (timeBox->value() > 0 ) return true;

    KMessageBox::sorry(this, i18n("Unable to search within a period which is less than a minute."));
    return false;
  }

  // If we can not parse either of the dates or
  // "from" date is bigger than "to" date return false.
  QDate hi1, hi2;

  QString str;
  if ( ! fromDate->getDate(&hi1).isValid() ||
       ! toDate->getDate(&hi2).isValid() )
    str = i18n("The date is not valid.");
  else if ( hi1 > hi2 )
    str = i18n("Invalid date range.");
  else if ( QDate::currentDate() < hi1 )
    str = i18n("Unable to search dates in the future.");

  if (!str.isNull()) {
    KMessageBox::sorry(0, str);
    return false;
  }
  return true;
}

void KfindTabWidget::setQuery(KQuery *query)
{
  KIO::filesize_t size;
  KIO::filesize_t sizeunit;
  bool itemAlreadyContained(false);
  // only start if we have valid dates
  if (!isDateValid()) return;

  query->setPath(KUrl::fromPathOrURL(dirBox->currentText().trimmed()));

  for (int idx=0; idx<dirBox->count(); idx++)
     if (dirBox->itemText(idx)==dirBox->currentText())
        itemAlreadyContained=true;

  if (!itemAlreadyContained)
     dirBox->addItem(dirBox->currentText().trimmed(),0);

  QString regex = nameBox->currentText().isEmpty() ? "*" : nameBox->currentText();
  query->setRegExp(regex, caseSensCb->isChecked());
  itemAlreadyContained=false;
  for (int idx=0; idx<nameBox->count(); idx++)
     if (nameBox->itemText(idx)==nameBox->currentText())
        itemAlreadyContained=true;

  if (!itemAlreadyContained)
     nameBox->addItem(nameBox->currentText(),0);

  query->setRecursive(subdirsCb->isChecked());

  switch (sizeUnitBox->currentIndex())
  {
     case 0:
         sizeunit = 1; //one byte
			break;
     case 2:
         sizeunit = 1048576; //1M
			break;
     case 3:
         sizeunit = 1073741824; //1G
			break;
     case 1: // fall to default case
     default:
         sizeunit=1024; //1k
            break;
  }
  size = sizeEdit->value() * sizeunit;

// TODO: troeder: do we need this check since it is very unlikely 
// to exceed ULLONG_MAX with INT_MAX * 1024^3. 
// Or is there an arch where this can happen?
#if 0 
  if (size < 0)  // overflow
  {
	if (KMessageBox::warningYesNo(this, i18n("Size is too big. Set maximum size value?"), i18n("Error"),i18n("Set"),i18n("Do Not Set"))
           == KMessageBox::Yes)
		{
         sizeEdit->setValue(INT_MAX);
		 sizeUnitBox->setCurrentIndex(0);
		 size = INT_MAX;
		}
     else
        return;
  }
#endif

  // set range mode and size value
  query->setSizeRange(sizeBox->currentIndex(),size,0);

  // dates
  QDateTime epoch;
  epoch.setTime_t(0);

  // Add date predicate
  if (findCreated->isChecked()) { // Modified
    if (rb[0]->isChecked()) { // Between dates
      QDate q1, q2;
      fromDate->getDate(&q1);
      toDate->getDate(&q2);

      // do not generate negative numbers .. find doesn't handle that
      time_t time1 = epoch.secsTo(QDateTime(q1));
      time_t time2 = epoch.secsTo(QDateTime(q2.addDays(1))) - 1; // Include the last day

      query->setTimeRange(time1, time2);
    }
    else
    {
       time_t cur = time(NULL);
       time_t minutes = cur;

       switch (betweenType->currentIndex())
       {
          case 0: // minutes
                 minutes = timeBox->value();
 	              break;
          case 1: // hours
                 minutes = 60 * timeBox->value();
 	              break;
          case 2: // days
                 minutes = 60 * 24 * timeBox->value();
 	              break;
          case 3: // months
                 minutes = 60 * 24 * (time_t)(timeBox->value() * 30.41667);
 	              break;
          case 4: // years
                 minutes = 12 * 60 * 24 * (time_t)(timeBox->value() * 30.41667);
 	              break;
       }

       query->setTimeRange(cur - minutes * 60, 0);
    }
  }
  else
    query->setTimeRange(0, 0);

  query->setUsername( m_usernameBox->currentText() );
  query->setGroupname( m_groupBox->currentText() );

  query->setFileType(typeBox->currentIndex());

  int id = typeBox->currentIndex()-10;

  if ((id >= -3) && (id < (int) m_types.count()))
  {
    switch(id)
    {
      case -3:
        query->setMimeType( m_ImageTypes );
        break;
      case -2:
        query->setMimeType( m_VideoTypes );
        break;
      case -1:
        query->setMimeType( m_AudioTypes );
        break;
      default:
        query->setMimeType( QStringList() += m_types[id]->name() );
     }
  }
  else
  {
     query->setMimeType( QStringList() );
  }

  //Metainfo
  query->setMetaInfo(metainfoEdit->text(), metainfokeyEdit->text());

  //Use locate to speed-up search ?
  query->setUseFileIndex(useLocateCb->isChecked());

  query->setContext(textEdit->text(), caseContextCb->isChecked(),
  	binaryContextCb->isChecked(), regexpContentCb->isChecked());
}

QString KfindTabWidget::date2String(const QDate & date) {
  return(KGlobal::locale()->formatDate(date, true));
}

QDate &KfindTabWidget::string2Date(const QString & str, QDate *qd) {
  return *qd = KGlobal::locale()->readDate(str);
}

void KfindTabWidget::getDirectory()
{
  QString result =
  KFileDialog::getExistingDirectory( dirBox->currentText().trimmed(),
                                     this );

  if (!result.isEmpty())
  {
    for (int i = 0; i < dirBox->count(); i++)
      if (result == dirBox->itemText(i)) {
	dirBox->setCurrentIndex(i);
	return;
      }
    dirBox->addItem(result, 0);
    dirBox->setCurrentIndex(0);
  }
}

void KfindTabWidget::beginSearch()
{
///  dirlister->openURL(KUrl::fromPathOrURL(dirBox->currentText().trimmed()));

  saveHistory();
  setEnabled( false );
}

void KfindTabWidget::endSearch()
{
  setEnabled( true );
}

/*
  Disables/enables all edit fields depending on their
  respective check buttons.
*/
void KfindTabWidget::fixLayout()
{
  int i;
  // If "All files" is checked - disable all edits
  // and second radio group on page two

  if(! findCreated->isChecked())  {
    fromDate->setEnabled(false);
    toDate->setEnabled(false);
    andL->setEnabled(false);
    timeBox->setEnabled(false);
    for(i=0; i<2; i++)
      rb[i]->setEnabled(false);
    betweenType->setEnabled(false);
  }
  else {
    for(i=0; i<2; i++)
      rb[i]->setEnabled(true);

    fromDate->setEnabled(rb[0]->isChecked());
    toDate->setEnabled(rb[0]->isChecked());
    andL->setEnabled(rb[0]->isEnabled());
    timeBox->setEnabled(rb[1]->isChecked());
    betweenType->setEnabled(rb[1]->isChecked());
  }

  // Size box on page three
  sizeEdit->setEnabled(sizeBox->currentIndex() != 0);
  sizeUnitBox->setEnabled(sizeBox->currentIndex() != 0);
}

bool KfindTabWidget::isSearchRecursive()
{
  return subdirsCb->isChecked();
}


/**
   Digit validator. Allows only digits to be typed.
**/
KDigitValidator::KDigitValidator( QWidget * parent, const char *name )
  : QValidator( parent )
{
  setObjectName( name );

  r = new QRegExp("^[0-9]*$");
}

KDigitValidator::~KDigitValidator()
{
  delete r;
}

QValidator::State KDigitValidator::validate( QString & input, int & ) const
{
  if (r->indexIn(input) < 0) {
    // Beep on user if he enters non-digit
    QApplication::beep();
    return QValidator::Invalid;
  }
  else
    return QValidator::Acceptable;
}

//*******************************************************
//             Static utility functions
//*******************************************************
static void save_pattern(QComboBox *obj,
			 const QString & group, const QString & entry)
{
  // QComboBox allows insertion of items more than specified by
  // maxCount() (QT bug?). This API call will truncate list if needed.
  obj->setMaxCount(15);

  // make sure the current item is saved first so it will be the
  // default when started next time
  QStringList sl;
  QString cur = obj->itemText(obj->currentIndex());
  sl.append(cur);
  for (int i = 0; i < obj->count(); i++) {
    if( cur != obj->itemText(i) ) {
      sl.append(obj->itemText(i));
    }
  }

  KConfig *conf = KGlobal::config();
  conf->setGroup(group);
  conf->writePathEntry(entry, sl);
}

QSize KfindTabWidget::sizeHint() const
{
  // #44662: avoid a huge default size when the comboboxes have very large items
  // Like in minicli, we changed the combobox size policy so that they can resize down,
  // and then we simply provide a reasonable size hint for the whole window, depending
  // on the screen width.
  QSize sz = QTabWidget::sizeHint();
  KfindTabWidget* me = const_cast<KfindTabWidget*>( this );
  const int screenWidth = qApp->desktop()->screenGeometry(me).width();
  if ( sz.width() > screenWidth / 2 )
    sz.setWidth( screenWidth / 2 );
  return sz;
}

#include "kftabdlg.moc"
