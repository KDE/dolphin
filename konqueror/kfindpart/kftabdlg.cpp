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

#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

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
  : QTabWidget( parent, name )
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

    rb1[0] = new QRadioButton(i18n("&All files"), pages[1]);
    rb1[1] = new QRadioButton(i18n("Find all files created or &modified:"), pages[1]);
    bg[0]  = new QButtonGroup();
    bg[1]  = new QButtonGroup();
    rb2[0] = new QRadioButton(i18n("&between"), pages[1] );
    rb2[1] = new QRadioButton(i18n("during the previou&s"), pages[1] );
    rb2[2] = new QRadioButton(i18n("&during the previous"), pages[1] );
    QLabel * andL   = new QLabel(i18n("and"), pages[1], "and");
    QLabel * monthL = new QLabel(i18n("month(s)"), pages[1], "months");
    QLabel * dayL   = new QLabel(i18n("day(s)"), pages[1], "days");
    le[0]  = new QLineEdit(pages[1], "lineEdit1" );
    le[1]  = new QLineEdit(pages[1], "lineEdit2" );
    le[2]  = new QLineEdit(pages[1], "lineEdit3" );
    le[3]  = new QLineEdit(pages[1], "lineEdit4" );

    rb2[0]->setChecked(true);

    // Setup

    le[0] ->setText(date2String(QDate(1980,1,1)));
    le[1] ->setText(date2String(QDate::currentDate()));

    le[2] ->setText("1");
    le[2] ->setValidator(digitV);

    le[3] ->setText("1");
    le[3] ->setValidator(digitV);

    rb1[0]->setChecked (TRUE);

    bg[0]->insert( rb1[0] );
    bg[0]->insert( rb1[1] );

    bg[1]->insert( rb2[0] );
    bg[1]->insert( rb2[1] );
    bg[1]->insert( rb2[2] );

    le[0]->setMaxLength(12);
    le[1]->setMaxLength(12);
    le[2]->setMaxLength(3);
    le[3]->setMaxLength(3);

    // Layout

    int tmp = le[0]->fontMetrics().width(" 00/00/0000 ");
    le[1]->setMinimumSize(tmp, le[1]->sizeHint().height());
    tmp = le[2]->fontMetrics().width(" 000 ");
    le[2]->setMinimumSize(tmp, le[2]->sizeHint().height());
    le[3]->setMinimumSize(tmp, le[3]->sizeHint().height());

    QGridLayout *grid1 = new QGridLayout( pages[1], 5,  6,
					  KDialog::marginHint(),
					  KDialog::spacingHint() );
    grid1->addMultiCellWidget(rb1[0], 0, 0, 0, 6 );
    grid1->addMultiCellWidget(rb1[1], 1, 1, 0, 6 );
    grid1->addColSpacing(0, KDialog::spacingHint());
    grid1->addWidget(rb2[0], 2, 1 );
    grid1->addWidget(le[0], 2, 2 );
    grid1->addWidget(andL, 2, 3 );
    grid1->addMultiCellWidget( le[1], 2, 2, 4, 5 );
    grid1->addMultiCellWidget( rb2[1], 3, 3, 1, 3 );
    grid1->addWidget(le[2], 3, 4 );
    grid1->addWidget(monthL, 3, 5 );
    grid1->addMultiCellWidget( rb2[2], 4, 4, 1, 3 );
    grid1->addWidget(le[3], 4, 4 );
    grid1->addWidget(dayL, 4, 5 );
    grid1->setColStretch(6, 1);

    // Connect

    connect( bg[0],  SIGNAL(clicked(int)),
             this,   SLOT(fixLayout()) );
    connect( bg[1],  SIGNAL(clicked(int)),
             this,   SLOT(fixLayout()) );

    addTab( pages[1], i18n(" Date Range ") );

    // ************ Page Three

    pages[2] = new QWidget( this, "page3" );

    typeBox =new QComboBox(FALSE, pages[2], "typeBox");
    QLabel * typeL   =new QLabel(typeBox, i18n("Of &type:"), pages[2], "type");
    textEdit=new QLineEdit(pages[2], "textEdit" );
    QLabel * textL   =new QLabel(textEdit, i18n("&Containing Text:"), pages[2], "text");

    caseContextCb  =new QCheckBox(i18n("Case S&ensitive (content)"), pages[2]);
    regexpContentCb  =new QCheckBox(i18n("Use &Regular Expression Matching"), pages[2]);

    sizeBox =new QComboBox(FALSE, pages[2], "sizeBox");
    QLabel * sizeL   =new QLabel(sizeBox,i18n("&Size is:"), pages[2],"size");
    sizeEdit=new QLineEdit(pages[2], "sizeEdit" );
    QLabel * kbL     =new QLabel(i18n("KB"), pages[2], "kb");

    // Setup

    typeBox->insertItem(i18n("All Files and Directories"));
    typeBox->insertItem(i18n("Files"));
    typeBox->insertItem(i18n("Directories"));
    typeBox->insertItem(i18n("Symbolic links"));
    typeBox->insertItem(i18n("Special files (sockets, device files...)"));
    typeBox->insertItem(i18n("Executable files"));
    typeBox->insertItem(i18n("SUID executable files"));

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

    sizeEdit->setText("1");
    sizeEdit->setMaxLength(5);
    sizeEdit->setValidator(digitV);

    // Connect
    connect( sizeBox, SIGNAL(highlighted(int)),
	     this, SLOT(slotSizeBoxChanged(int)));

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

    grid2->addWidget( caseContextCb, 2, 1 );
    grid2->addWidget( regexpContentCb, 2, 2);

    grid2->addWidget( sizeBox, 3, 1 );
    grid2->addWidget( sizeEdit, 3, 2 );
    grid2->addWidget( kbL, 3, 3 );
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
}

void KfindTabWidget::slotSizeBoxChanged(int index)
{
  sizeEdit->setEnabled((bool)(index != 0));
}

void KfindTabWidget::setDefaults()
  {
    le[0] ->setText(date2String(QDate(1980,1,1)));
    le[1] ->setText(date2String(QDate::currentDate()));
    le[2] ->setText("1");
    le[3] ->setText("1");

    typeBox ->setCurrentItem(0);
    sizeBox ->setCurrentItem(0);
    sizeEdit->setText("1");
  }

/*
  Checks if dates are correct and popups a error box
  if they are not.
*/
bool KfindTabWidget::isDateValid()
{
  // All files
  if ( !rb1[1]->isChecked() ) return TRUE;

  if ( rb2[1]->isChecked() || rb2[2]->isChecked() )
  {
    QString str;
    str = le[ rb2[1]->isChecked() ? 2 : 3 ]->text();
    if (str.toInt() > 0 ) return TRUE;

    KMessageBox::sorry(this, i18n("Can't search in a period which doesn't last a single day."));
    return FALSE;
  }

  // If we can not parse either of the dates or
  // "from" date is bigger than "to" date return FALSE.
  QDate hi1, hi2;

  QString str;
  if ( !string2Date(le[0]->text(), &hi1).isValid() ||
       !string2Date(le[1]->text(), &hi2).isValid() )
    str = i18n("The date is not valid!");
  else if ( hi1 > hi2 )
    str = i18n("Invalid date range!");
  else if ( QDate::currentDate() < hi1 )
    str = i18n("Can't search dates in the future!");

  if (!str.isNull()) {
    KMessageBox::sorry(0, str);
    return FALSE;
  }
  return TRUE;
}

void KfindTabWidget::setQuery(KQuery *query)
{
  // only start if we have valid dates
  if (!isDateValid()) return;

  query->setPath(KURL(dirBox->currentText()));

  query->setRegExp(QRegExp(nameBox->currentText(), caseSensCb->isChecked(), true));

  query->setRecursive(subdirsCb->isChecked());

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
  if (rb1[1]->isChecked()) { // Modified
    if (rb2[0]->isChecked()) { // Between dates
      QDate q1, q2;
      string2Date(le[0]->text(), &q1);
      string2Date(le[1]->text(), &q2);

      // do not generate negative numbers .. find doesn't handle that
      time_t time1 = epoch.secsTo(q1);
      time_t time2 = epoch.secsTo(q2);

      query->setTimeRange(time1, time2);
    }
    else
    {
      time_t cur = epoch.secsTo(QDateTime::currentDateTime());
      time_t days = cur;

      if (rb2[1]->isChecked()) // Previous mounth
	days = (time_t)(le[2]->text().toInt() * 30.41667);
      else if (rb2[2]->isChecked()) // Previous day
	days = le[3]->text().toInt();

      query->setTimeRange(cur - days * 24 * 60 * 60, 0);
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

  if(rb1[0]->isChecked())  {
    for(i=0; i<4; i++)
      le[i]->setEnabled(FALSE);

    for(i=0; i<3; i++)
      rb2[i]->setEnabled(FALSE);
  }
  else {
    for(i=0; i<3; i++)
      rb2[i]->setEnabled(TRUE);

    le[0]->setEnabled(rb2[0]->isChecked());
    le[1]->setEnabled(rb2[0]->isChecked());
    le[2]->setEnabled(rb2[1]->isChecked());
    le[3]->setEnabled(rb2[2]->isChecked());
  }

  // Size box on page three
  sizeEdit->setEnabled(sizeBox->currentItem() != 0);
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
{}

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
