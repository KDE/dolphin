/*
   Copyright (C) 2005 Ivor Hewitt <ivor@ivor.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kaboutdata.h>
#include <kfiledialog.h>
#include <Q3ListBox>
#include <QLayout>
#include <QPushButton>
#include <q3groupbox.h>
#include <khbox.h>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QTextStream>
#include <QRegExp>
#include <QtDBus/QtDBus>

#include "filteropts.h"
#include "filteropts.moc"

#include <kgenericfactory.h>
typedef KGenericFactory<KCMFilter, QWidget> KCMFilterFactory;
K_EXPORT_COMPONENT_FACTORY( khtml_filter, KCMFilterFactory("kcmkonqhtml") )

KCMFilter::KCMFilter( QWidget *parent, const QStringList& )
    : KCModule( KCMFilterFactory::instance(), parent ),
      mGroupname( "Filter Settings" ),
      mSelCount(0)
{
    mConfig = KSharedConfig::openConfig( "khtmlrc", false, false );
    setButtons(Default|Apply);

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    mEnableCheck = new QCheckBox(i18n("Enable filters"), this);
    topLayout->addWidget( mEnableCheck );

    mKillCheck = new QCheckBox(i18n("Hide filtered images"), this);
    topLayout->addWidget( mKillCheck );

    QGroupBox *topBox = new QGroupBox( i18n("URL Expressions to Filter") );
    topLayout->addWidget( topBox );

    QVBoxLayout *vbox = new QVBoxLayout;

    mListBox = new Q3ListBox;
    mListBox->setSelectionMode(Q3ListBox::Extended);
    vbox->addWidget(mListBox);
    vbox->addWidget(new QLabel( i18n("Expression (e.g. http://www.site.com/ad/*):")));
    mString = new QLineEdit;
    vbox->addWidget(mString);

    KHBox *buttonBox = new KHBox;
    vbox->addWidget(buttonBox);
    buttonBox->setSpacing( KDialog::spacingHint() );

    topBox->setLayout(vbox);
    mInsertButton = new QPushButton( i18n("Insert"), buttonBox );
    connect( mInsertButton, SIGNAL( clicked() ), SLOT( insertFilter() ) );
    mUpdateButton = new QPushButton( i18n("Update"), buttonBox );
    connect( mUpdateButton, SIGNAL( clicked() ), SLOT( updateFilter() ) );
    mRemoveButton = new QPushButton( i18n("Remove"), buttonBox );
    connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removeFilter() ) );

    mImportButton = new QPushButton(i18n("Import..."),buttonBox);
    connect( mImportButton, SIGNAL( clicked() ), SLOT( importFilters() ) );
    mExportButton = new QPushButton(i18n("Export..."),buttonBox);
    connect( mExportButton, SIGNAL( clicked() ), SLOT( exportFilters() ) );

    connect( mEnableCheck, SIGNAL( clicked()), this, SLOT( slotEnableChecked()));
    connect( mKillCheck, SIGNAL( clicked()), this, SLOT( slotKillChecked()));
    connect( mListBox, SIGNAL( selectionChanged ()),this, SLOT( slotItemSelected()));

/*
 * Whats this items
 */
    mEnableCheck->setWhatsThis( i18n("Enable or disable AdBlocK filters. When enabled a set of expressions "
                                        "to be blocked should be defined in the filter list for blocking to "
                                        "take effect."));
    mKillCheck->setWhatsThis( i18n("When enabled blocked images will be removed from the page completely "
                                      "otherwise a placeholder 'blocked' image will be used."));
    mListBox->setWhatsThis( i18n("This is the list of URL filters that will be applied to all linked "
                                    "images and frames. The filters are processed in order so place "
                                    "more generic filters towards the top of the list."));
    mString->setWhatsThis( i18n("Enter an expression to filter. Expressions can be defined as either "
                                   "a filename style wildcard e.g. http://www.site.com/ads* or as a full "
                                   "regular expression by surrounding the string with '/' e.g. "
                                   " //(ad|banner)\\./"));
    load();
    updateButton();
}

KCMFilter::~KCMFilter()
{
}

void KCMFilter::slotKillChecked()
{
    emit changed( true );
}

void KCMFilter::slotEnableChecked()
{
    updateButton();
    emit changed( true );
}

void KCMFilter::slotItemSelected()
{
    int currentId=-1;
    unsigned int i;
    for( i=0,mSelCount=0; i < mListBox->count() && mSelCount<2; ++i )
    {
        if (mListBox->isSelected(i))
        {
            currentId=i;
            mSelCount++;
        }
    }

    if ( currentId >= 0 )
    {
        mString->setText(mListBox->text(currentId));
    }
    updateButton();
}

void KCMFilter::updateButton()
{
    bool state = mEnableCheck->isChecked();

    mUpdateButton->setEnabled(state && (mSelCount == 1));
    mRemoveButton->setEnabled(state && (mSelCount > 0));
    mInsertButton->setEnabled(state);
    mImportButton->setEnabled(state);
    mExportButton->setEnabled(state);

    mListBox->setEnabled(state);
    mString->setEnabled(state);
    mKillCheck->setEnabled(state);
}

void KCMFilter::importFilters()
{
    QString inFile = KFileDialog::getOpenFileName();
    if (inFile.length() > 0)
    {
        QFile f(inFile);
        if ( f.open( QIODevice::ReadOnly ) )
        {
            QTextStream ts( &f );
            QStringList paths;
            QString line;
            while (!ts.atEnd())
            {
                line = ts.readLine();
                if (line.toLower().compare("[adblock]") == 0)
                    continue;

                // Treat leading ! as filter comment, otherwise check expressions
                // are valid.
                if (!line.startsWith("!"))
                {
                    if (line.length()>2 && line[0]=='/' && line[line.length()-1] == '/')
                    {
                        QString inside = line.mid(1, line.length()-2);
                        QRegExp rx(inside);
                        if (!rx.isValid())
                            continue;
                    }
                    else
                    {
                        QRegExp rx(line);
                        rx.setPatternSyntax(QRegExp::Wildcard);
                        if (!rx.isValid())
                            continue;
                    }
                }

                if (!line.isEmpty() && mListBox->findItem(line, Q3ListBox::CaseSensitive|Q3ListBox::ExactMatch) == 0)
                    paths.append(line);
            }
            f.close();

            mListBox->insertStringList( paths );
            emit changed(true);
        }
    }
}

void KCMFilter::exportFilters()
{
  QString outFile = KFileDialog::getSaveFileName();
  if (outFile.length() > 0)
  {
    QFile f(outFile);
    if ( f.open( QIODevice::WriteOnly ) )
    {
      QTextStream ts( &f );
      ts.setCodec( "UTF-8" );
      ts << "[AdBlock]" << endl;

      uint i;
      for( i = 0; i < mListBox->count(); ++i )
        ts << mListBox->text(i) << endl;

      f.close();
    }
  }
}

void KCMFilter::defaults()
{
    mListBox->clear();
    updateButton();
}

void KCMFilter::save()
{
    mConfig->deleteGroup(mGroupname);
    mConfig->setGroup(mGroupname);

    mConfig->writeEntry("Enabled",mEnableCheck->isChecked());
    mConfig->writeEntry("Shrink",mKillCheck->isChecked());

    uint i;
    for( i = 0; i < mListBox->count(); ++i )
    {
        QString key = "Filter-" + QString::number(i);
        mConfig->writeEntry(key, mListBox->text(i));
    }
    mConfig->writeEntry("Count",mListBox->count());

    mConfig->sync();

    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);
}

void KCMFilter::load()
{
    QStringList paths;

    mConfig->setGroup( mGroupname );
    mEnableCheck->setChecked( mConfig->readEntry("Enabled", QVariant(false)).toBool());
    mKillCheck->setChecked( mConfig->readEntry("Shrink", QVariant(false)).toBool());

    QMap<QString,QString> entryMap = mConfig->entryMap( mGroupname );
    QMap<QString,QString>::ConstIterator it;
    int num = mConfig->readEntry("Count",0);
    for (int i=0; i<num; ++i)
    {
        QString key = "Filter-" + QString::number(i);
        it = entryMap.find(key);
        if (it != entryMap.end())
            paths.append(it.value());
    }

    mListBox->insertStringList( paths );
}

void KCMFilter::insertFilter()
{
    if ( !mString->text().isEmpty() )
    {
        mListBox->insertItem( mString->text() );
        int id=mListBox->count()-1;
        mListBox->clearSelection();
        mListBox->setSelected(id,true);
        mListBox->setCurrentItem(id);
        mListBox->ensureCurrentVisible();
        emit changed( true );
    }
    updateButton();
}

void KCMFilter::removeFilter()
{
    for( int i = mListBox->count(); i >= 0; --i )
    {
        if (mListBox->isSelected(i))
            mListBox->removeItem(i);
    }
    emit changed( true );
    updateButton();
}

void KCMFilter::updateFilter()
{
    if ( !mString->text().isEmpty() )
    {
        int index = mListBox->currentItem();
        if ( index >= 0 )
        {
            mListBox->changeItem( mString->text(), index );
            emit changed( true );
        }
    }
    updateButton();
}

QString KCMFilter::quickHelp() const
{
    return i18n("<h1>Konqueror AdBlocK</h1> Konqueror AdBlocK allows you to create a list of filters"
                " that are checked against linked images and frames. URL's that match are either discarded or"
                " replaced with a placeholder image. ");
}
