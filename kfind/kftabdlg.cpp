/***********************************************************************
 *
 *  kftabdlg.cpp
 *
 **********************************************************************/

#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kregexpeditorinterface.h>
#include <kparts/componentfactory.h>

#include "kquery.h"
#include <qpushbutton.h>
#include <kglobal.h>
#include "kftabdlg.moc"

// Static utility functions
static void save_pattern(QComboBox *, const QString &, const QString &);

#define SPECIAL_TYPES 7

class KSortedMimeTypeList : public QPtrList<KMimeType>
{
public:
  KSortedMimeTypeList() { };
  int compareItems(QPtrCollection::Item s1, QPtrCollection::Item s2)
  {
     KMimeType *item1 = (KMimeType *) s1;
     KMimeType *item2 = (KMimeType *) s2;
     if (item1->comment() > item2->comment()) return 1;
     if (item1->comment() == item2->comment()) return 0;
     return -1;
  }
};

KfindTabWidget::KfindTabWidget(QWidget *parent, const char *name)
  : QTabWidget( parent, name ), regExpDialog(0)
{
    // This validator will be used for all numeric edit fields
    KDigitValidator *digitV = new KDigitValidator(this);

    // ************ Page One ************

    pages[0] = new QWidget( this, "page1" );

    nameBox = new QComboBox(TRUE, pages[0], "combo1");
    QLabel * namedL = new QLabel(nameBox, i18n("&Named:"), pages[0], "named");
    dirBox  = new QComboBox(TRUE, pages[0], "combo2");
    QLabel * lookinL = new QLabel(dirBox, i18n("&Look in:"), pages[0], "named");
    subdirsCb  = new QCheckBox(i18n("Include &subdirectories"), pages[0]);
    caseSensCb  = new QCheckBox(i18n("&Case sensitive search"), pages[0]);
    browseB    = new QPushButton(i18n("&Browse..."), pages[0]);

    // Setup

    subdirsCb->setChecked(true);
    caseSensCb->setChecked(true);

    nameBox->setDuplicatesEnabled(FALSE);
    dirBox->setDuplicatesEnabled(FALSE);

    nameBox->setInsertionPolicy(QComboBox::AtTop);
    dirBox->setInsertionPolicy(QComboBox::AtTop);

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
    QWhatsThis::add(nameBox,nameWhatsThis);
    QWhatsThis::add(namedL,nameWhatsThis);

    // Layout

    QGridLayout *grid = new QGridLayout( pages[0], 3, 3,
					 KDialog::marginHint(),
					 KDialog::spacingHint() );
    QBoxLayout *subgrid = new QHBoxLayout( -1 , "subgrid" );
    grid->addWidget( namedL, 0, 0 );
    grid->addMultiCellWidget( nameBox, 0, 0, 1, 2 );
    grid->addWidget( lookinL, 1, 0 );
    grid->addWidget( dirBox, 1, 1 );
    grid->addWidget( browseB, 1, 2);
    grid->setColStretch(1,1);
    grid->addMultiCellLayout( subgrid, 2, 2, 1, 2 );
    subgrid->addWidget( subdirsCb );
    subgrid->addSpacing( KDialog::spacingHint() );
    subgrid->addWidget( caseSensCb);
    subgrid->addStretch(1);

    // Signals

    connect( browseB, SIGNAL(clicked()),
             this, SLOT(getDirectory()) );

    addTab( pages[0], i18n(" Name/Location ") );

    // ************ Page Two

    pages[1] = new QWidget( this, "page2" );

    findCreated =  new QCheckBox(i18n("Find all files created or &modified:"), pages[1]);
    bg  = new QButtonGroup();
    rb[0] = new QRadioButton(i18n("&between"), pages[1] );
    rb[1] = new QRadioButton(i18n("during the previou&s"), pages[1] );
    QLabel * andL   = new QLabel(i18n("and"), pages[1], "and");
    betweenType = new QComboBox(FALSE, pages[1], "comboBetweenType");
    betweenType->insertItem(i18n("minute(s)"));
    betweenType->insertItem(i18n("hour(s)"));
    betweenType->insertItem(i18n("day(s)"));
    betweenType->insertItem(i18n("month(s)"));
    betweenType->insertItem(i18n("year(s)"));
    betweenType->setCurrentItem(1);

    fromDate = new KDateCombo(QDate(2000,1,1), pages[1], "fromDate");
    toDate = new KDateCombo(pages[1], "toDate");
    timeBox = new QSpinBox(1, 60, 1, pages[1], "timeBox");

    rb[0]->setChecked(true);

    // Setup
    timeBox->setButtonSymbols(QSpinBox::PlusMinus);
    bg->insert( rb[0] );
    bg->insert( rb[1] );

    // Layout

    QGridLayout *grid1 = new QGridLayout( pages[1], 5,  4,
					  KDialog::marginHint(),
					  KDialog::spacingHint() );
    grid1->addMultiCellWidget(findCreated, 0, 0, 0, 6 );
    grid1->addColSpacing(0, KDialog::spacingHint());
    grid1->addWidget(rb[0], 2, 1 );
    grid1->addWidget(fromDate, 2, 2 );
    grid1->addWidget(andL, 2, 3, AlignHCenter );
    grid1->addWidget(toDate, 2, 4 );
    grid1->addWidget(rb[1], 3, 1 );
    grid1->addWidget(timeBox, 3, 2);
    grid1->addWidget(betweenType, 3, 3 );

    // Connect
    connect( findCreated,  SIGNAL(toggled(bool)), this,   SLOT(fixLayout()) );
    connect( bg,  SIGNAL(clicked(int)), this,   SLOT(fixLayout()) );

    addTab( pages[1], i18n(" Date Range ") );

    // ************ Page Three

    pages[2] = new QWidget( this, "page3" );

    typeBox =new QComboBox(FALSE, pages[2], "typeBox");
    QLabel * typeL   =new QLabel(typeBox, i18n("Of &type:"), pages[2], "type");
    textEdit=new QLineEdit(pages[2], "textEdit" );
    QLabel * textL   =new QLabel(textEdit, i18n("&Containing Text:"), pages[2], "text");

    caseContextCb  =new QCheckBox(i18n("Case S&ensitive (content)"), pages[2]);
    regexpContentCb  =new QCheckBox(i18n("Use &Regular Expression Matching"), pages[2]);

    QPushButton* editRegExp = 0;
    regExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString::null, this );
    if ( regExpDialog ) {
      // The editor was available, so lets use it.
      editRegExp = new QPushButton(i18n("&Edit Regular Expression"), pages[2], "editRegExp");
    }

    sizeBox =new QComboBox(FALSE, pages[2], "sizeBox");
    QLabel * sizeL   =new QLabel(sizeBox,i18n("&Size is:"), pages[2],"size");
    sizeEdit=new QSpinBox(1, INT_MAX, 1, pages[2], "sizeEdit" );
    sizeUnitBox =new QComboBox(FALSE, pages[2], "sizeUnitBox");

    // Setup

    typeBox->insertItem(i18n("All Files and Directories"));
    typeBox->insertItem(i18n("Files"));
    typeBox->insertItem(i18n("Directories"));
    typeBox->insertItem(i18n("Symbolic Links"));
    typeBox->insertItem(i18n("Special Files (Sockets, Device Files...)"));
    typeBox->insertItem(i18n("Executable Files"));
    typeBox->insertItem(i18n("SUID Executable Files"));

    initMimeTypes();

    for ( KMimeType::List::ConstIterator it = m_types.begin();
          it != m_types.end(); ++it )
    {
      KMimeType::Ptr typ = *it;
      typeBox->insertItem(typ->comment());
    }

    sizeBox ->insertItem( i18n("(none)") );
    sizeBox ->insertItem( i18n("At Least") );
    sizeBox ->insertItem( i18n("At Most") );
    sizeBox ->insertItem( i18n("Equal to") );

    sizeUnitBox ->insertItem( "Bytes" );
    sizeUnitBox ->insertItem( "Kb" );
    sizeUnitBox ->insertItem( "Mb" );
    sizeUnitBox ->setCurrentItem(1);

    sizeEdit->setButtonSymbols(QSpinBox::PlusMinus);
    int tmp = sizeEdit->fontMetrics().width(" 000000000 ");
    sizeEdit->setMinimumSize(tmp, sizeEdit->sizeHint().height());

    // Connect
    connect( sizeBox, SIGNAL(highlighted(int)),
	     this, SLOT(slotSizeBoxChanged(int)));

    if ( regExpDialog ) {
      // The editor was available, so lets use it.
      connect( regexpContentCb, SIGNAL(toggled(bool) ), editRegExp, SLOT(setEnabled(bool)) );
      editRegExp->setEnabled(false);
      connect( editRegExp, SIGNAL(clicked()), this, SLOT( slotEditRegExp() ) );
    }

    // Layout
    tmp = sizeEdit->fontMetrics().width(" 00000 ");
    sizeEdit->setMinimumSize(tmp, sizeEdit->sizeHint().height());

    QGridLayout *grid2 = new QGridLayout( pages[2], 4, 6,
					  KDialog::marginHint(),
					  KDialog::spacingHint() );
    grid2->addWidget( typeL, 0, 0 );
    grid2->addWidget( textL, 1, 0 );
    grid2->addWidget( sizeL, 2, 0 );
    grid2->addMultiCellWidget( typeBox, 0, 0, 1, 6 );
    grid2->addMultiCellWidget( textEdit, 1, 1, 1, 6 );
    grid2->addWidget( caseContextCb, 2, 5 );
    grid2->addWidget( sizeBox, 2, 1 );
    grid2->addWidget( sizeEdit, 2, 2 );
    grid2->addWidget( sizeUnitBox, 2, 3 );
    grid2->addMultiCellWidget( regexpContentCb, 3, 3, 1, 2);

    if ( regExpDialog ) {
      // The editor was available, so lets use it.
      grid2->addWidget( editRegExp, 3, 3 );
    }
    
    grid2->addColSpacing(4, KDialog::spacingHint());
    grid2->setColStretch(6,1);

    addTab( pages[2], i18n(" Advanced ") );

    fixLayout();
    loadHistory();
}

KfindTabWidget::~KfindTabWidget()
{
  delete pages[0];
  delete pages[1];
  delete pages[2];
}

void KfindTabWidget::setURL( const KURL & url )
{
  KConfig *conf = KGlobal::config();
  conf->setGroup("History");
  m_url = url;
  QStringList sl = conf->readListEntry("Directories", ',');
  dirBox->clear(); // make sure there is no old Stuff in there

  if(!sl.isEmpty()) {
    dirBox->insertStringList(sl);
    // If the _searchPath already exists in the list we do not
    // want to add it again
    int indx = sl.findIndex(m_url.url());
    if(indx == -1)
      dirBox->insertItem(m_url.url(), 0); // make it the first one
    else
      dirBox->setCurrentItem(indx);
  }
  else {
    QDir m_dir("/lib");
    dirBox ->insertItem( m_url.url() );
    dirBox ->insertItem( "file:/" );
    dirBox ->insertItem( "file:/usr" );
    if (m_dir.exists())
      dirBox ->insertItem( "file:/lib" );
    dirBox ->insertItem( "file:/home" );
    dirBox ->insertItem( "file:/etc" );
    dirBox ->insertItem( "file:/var" );
    dirBox ->insertItem( "file:/mnt" );
  }
}

void
KfindTabWidget::initMimeTypes()
{
    KMimeType::List tmp = KMimeType::allMimeTypes();
    KSortedMimeTypeList sortedList;
    for ( KMimeType::List::ConstIterator it = tmp.begin();
          it != tmp.end(); ++it )
    {
      KMimeType * type = *it;
      sortedList.append(type);
    }
    sortedList.sort();
    for ( KMimeType *type = sortedList.first(); type; type = sortedList.next())
    {
       m_types.append(type);
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
  QStringList sl = conf->readListEntry("Patterns", ',');
  if(!sl.isEmpty())
    nameBox->insertStringList(sl);
  else
    nameBox->insertItem("*");

  sl = conf->readListEntry("Directories", ',');
  if(!sl.isEmpty()) {
    dirBox->insertStringList(sl);
    // If the _searchPath already exists in the list we do not
    // want to add it again
    int indx = sl.findIndex(m_url.url());
    if(indx == -1)
      dirBox->insertItem(m_url.url(), 0); // make it the first one
    else
      dirBox->setCurrentItem(indx);
  }
  else {
    QDir m_dir("/lib");
    dirBox ->insertItem( m_url.url() );
    dirBox ->insertItem( "file:/" );
    dirBox ->insertItem( "file:/usr" );
    if (m_dir.exists())
      dirBox ->insertItem( "file:/lib" );
    dirBox ->insertItem( "file:/home" );
    dirBox ->insertItem( "file:/etc" );
    dirBox ->insertItem( "file:/var" );
    dirBox ->insertItem( "file:/mnt" );
  }
}

void KfindTabWidget::slotEditRegExp() 
{
  if ( ! regExpDialog )
    regExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString::null, this );

  KRegExpEditorInterface *iface = dynamic_cast<KRegExpEditorInterface *>( regExpDialog );
  if ( !iface )
      return;

  iface->setRegExp( textEdit->text() );
  bool ok = regExpDialog->exec();
  if ( ok )
    textEdit->setText( iface->regExp() );
}

void KfindTabWidget::slotSizeBoxChanged(int index)
{
  sizeEdit->setEnabled((bool)(index != 0));
  sizeUnitBox->setEnabled((bool)(index != 0));
}

void KfindTabWidget::setDefaults()
{
    fromDate ->setDate(QDate(2000,1,1));
    toDate ->setDate(QDate::currentDate());

    timeBox->setValue(1);
    betweenType->setCurrentItem(1);

    typeBox ->setCurrentItem(0);
    sizeBox ->setCurrentItem(0);
    sizeUnitBox ->setCurrentItem(1);
    sizeEdit->setValue(1);
}

/*
  Checks if dates are correct and popups a error box
  if they are not.
*/
bool KfindTabWidget::isDateValid()
{
  // All files
  if ( !findCreated->isChecked() ) return TRUE;

  if (rb[1]->isChecked())
  {
    if (timeBox->value() > 0 ) return TRUE;

    KMessageBox::sorry(this, i18n("Can't search in a period which doesn't last a single minute."));
    return FALSE;
  }

  // If we can not parse either of the dates or
  // "from" date is bigger than "to" date return FALSE.
  QDate hi1, hi2;

  QString str;
  if ( ! fromDate->getDate(&hi1).isValid() ||
       ! toDate->getDate(&hi2).isValid() )
    str = i18n("The date is not valid!");
  else if ( hi1 > hi2 )
    str = i18n("Invalid date range!");
  else if ( QDate::currentDate() < hi1 )
    str = i18n("Well, how can I search dates in the future ?");

  if (!str.isNull()) {
    KMessageBox::sorry(0, str);
    return FALSE;
  }
  return TRUE;
}

void KfindTabWidget::setQuery(KQuery *query)
{
	int size;
  bool itemAlreadyContained(false);
  // only start if we have valid dates
  if (!isDateValid()) return;

  query->setPath(KURL(dirBox->currentText()));

  for (int idx=0; idx<dirBox->count(); idx++)
     if (dirBox->text(idx)==dirBox->currentText())
        itemAlreadyContained=true;

  if (!itemAlreadyContained)
     dirBox->insertItem(dirBox->currentText().stripWhiteSpace(),0);

  query->setRegExp(nameBox->currentText(), caseSensCb->isChecked());
  itemAlreadyContained=false;
  for (int idx=0; idx<nameBox->count(); idx++)
     if (nameBox->text(idx)==nameBox->currentText())
        itemAlreadyContained=true;

  if (!itemAlreadyContained)
     nameBox->insertItem(nameBox->currentText(),0);

  query->setRecursive(subdirsCb->isChecked());

  switch (sizeUnitBox->currentItem())
  {
     case 0:
         size = 1; //one byte
			break;
     case 2:
         size = 1048576; //1M
			break;
		case 1:
		default:
			size=1024; //1k
			break;
  }
  size = sizeEdit->value() * size;
  if (size < 0)  // overflow
     if (KMessageBox::warningYesNo(this, i18n("Size is too big... would you set max size value ?"), i18n("Sorry"))
           == KMessageBox::Yes)
		{
         sizeEdit->setValue(INT_MAX);
	   	sizeUnitBox->setCurrentItem(0);
		   size = INT_MAX;
		}
     else
        return;

  switch (sizeBox->currentItem())
  {
    case 1:
      query->setSizeRange(sizeEdit->text().toInt() * 1024, -1);
      break;
    case 2:
      query->setSizeRange(-1, sizeEdit->text().toInt() * 1024);
      break;
    default:
      query->setSizeRange(-1, -1);
  }

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
      time_t time1 = epoch.secsTo(q1);
      time_t time2 = epoch.secsTo(q2);

      query->setTimeRange(time1, time2);
    }
    else
    {
       time_t cur = time(NULL);
       time_t minutes = cur;

       switch (betweenType->currentItem())
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

  query->setFileType(typeBox->currentItem());

  int id = typeBox->currentItem()-7;
  if ((id >= 0) && (id < (int) m_types.count()))
  {
     query->setMimeType( m_types[id]->name() );
  }
  else
  {
     query->setMimeType( QString::null );
  }

  query->setContext(textEdit->text(), caseContextCb->isChecked(), regexpContentCb->isChecked());
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
  KFileDialog::getExistingDirectory( dirBox->text(dirBox->currentItem()),
                                     this );

  if (!result.isEmpty())
  {
    for (int i = 0; i < dirBox->count(); i++)
      if (result == dirBox->text(i)) {
	dirBox->setCurrentItem(i);
	return;
      }
    dirBox->insertItem(result, 0);
    dirBox->setCurrentItem(0);
  }
}

void KfindTabWidget::beginSearch()
{
  saveHistory();
  setEnabled( FALSE );
}

void KfindTabWidget::endSearch()
{
  setEnabled( TRUE );
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
    fromDate->setEnabled(FALSE);
    toDate->setEnabled(FALSE);
    timeBox->setEnabled(FALSE);
    for(i=0; i<2; i++)
      rb[i]->setEnabled(FALSE);
    betweenType->setEnabled(FALSE);
  }
  else {
    for(i=0; i<2; i++)
      rb[i]->setEnabled(TRUE);

    fromDate->setEnabled(rb[0]->isChecked());
    toDate->setEnabled(rb[0]->isChecked());
    timeBox->setEnabled(rb[1]->isChecked());
    betweenType->setEnabled(rb[1]->isChecked());
  }

  // Size box on page three
  sizeEdit->setEnabled(sizeBox->currentItem() != 0);
  sizeUnitBox->setEnabled(sizeBox->currentItem() != 0);
}

/**
   Digit validator. Allows only digits to be typed.
**/
KDigitValidator::KDigitValidator( QWidget * parent, const char *name )
  : QValidator( parent, name )
{
  r = new QRegExp("^[0-9]*$");
}

KDigitValidator::~KDigitValidator()
{
  delete r;
}

QValidator::State KDigitValidator::validate( QString & input, int & ) const
{
  if (r->search(input) < 0) {
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

  QStringList sl;
  for (int i = 0; i < obj->count(); i++)
    sl.append(obj->text(i));

  KConfig *conf = KGlobal::config();
  conf->setGroup(group);
  conf->writeEntry(entry, sl, ',');
}
