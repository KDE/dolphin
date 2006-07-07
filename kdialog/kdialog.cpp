//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002 David Faure <faure@kde.org>
//  Copyright (C) 2005 Brad Hards <bradh@frogmouth.net>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the7 implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include <QFile>
#include <QDataStream>
#include <QRegExp>
#include <QTimer>
#include <qdesktopwidget.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kdebug.h>
//#include <ktopwidget.h>
#include <kmainwindow.h>
#include <kpassivepopup.h>
#include <krecentdocument.h>
#include <kapplication.h>

#include "widgets.h"

#include <klocale.h>
#include <QDialog>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kfiledialog.h>
#include <kicondialog.h>
#include <kdirselectdialog.h>

#if defined Q_WS_X11 && ! defined K_WS_QTONLY
#include <netwm.h>
#endif

using namespace std;

#if defined(Q_WS_X11)
extern "C" { int XSetTransientForHint( Display *, unsigned long, unsigned long ); }
#endif // Q_WS_X11

static KCmdLineOptions options[] =
{
    { "yesno <text>", I18N_NOOP("Question message box with yes/no buttons"), 0 },
    { "yesnocancel <text>", I18N_NOOP("Question message box with yes/no/cancel buttons"), 0 },
    { "warningyesno <text>", I18N_NOOP("Warning message box with yes/no buttons"), 0 },
    { "warningcontinuecancel <text>", I18N_NOOP("Warning message box with continue/cancel buttons"), 0 },
    { "warningyesnocancel <text>", I18N_NOOP("Warning message box with yes/no/cancel buttons"), 0 },
    { "sorry <text>", I18N_NOOP("'Sorry' message box"), 0 },
    { "error <text>", I18N_NOOP("'Error' message box"), 0 },
    { "msgbox <text>", I18N_NOOP("Message Box dialog"), 0 },
    { "inputbox <text> <init>", I18N_NOOP("Input Box dialog"), 0 },
    { "password <text>", I18N_NOOP("Password dialog"), 0 },
    { "textbox <file> [width] [height]", I18N_NOOP("Text Box dialog"), 0 },
    { "textinputbox <text> <init> [width] [height]", I18N_NOOP("Text Input Box dialog"), 0 },
    { "combobox <text> [tag item] [tag item] ...", I18N_NOOP("ComboBox dialog"), 0 },
    { "menu <text> [tag item] [tag item] ...", I18N_NOOP("Menu dialog"), 0 },
    { "checklist <text> [tag item status] ...", I18N_NOOP("Check List dialog"), 0 },
    { "radiolist <text> [tag item status] ...", I18N_NOOP("Radio List dialog"), 0 },
    { "passivepopup <text> <timeout>", I18N_NOOP("Passive Popup"), 0 },
    { "getopenfilename [startDir] [filter]", I18N_NOOP("File dialog to open an existing file"), 0 },
    { "getsavefilename [startDir] [filter]", I18N_NOOP("File dialog to save a file"), 0 },
    { "getexistingdirectory [startDir]", I18N_NOOP("File dialog to select an existing directory"), 0 },
    { "getopenurl [startDir] [filter]", I18N_NOOP("File dialog to open an existing URL"), 0 },
    { "getsaveurl [startDir] [filter]", I18N_NOOP("File dialog to save a URL"), 0 },
    { "geticon [group] [context]", I18N_NOOP("Icon chooser dialog"), 0 },
    { "progressbar <text> [totalsteps]", I18N_NOOP("Progress bar dialog, returns a DCOP reference for communication"), 0},

    // TODO gauge stuff, reading values from stdin

    { "title <text>", I18N_NOOP("Dialog title"), 0 },
    { "default <text>", I18N_NOOP("Default entry to use for combobox and menu"), 0 },
    { "multiple", I18N_NOOP("Allows the --getopenurl and --getopenfilename options to return multiple files"), 0 },
    { "separate-output", I18N_NOOP("Return list items on separate lines (for checklist option and file open with --multiple)"), 0 },
    { "print-winid", I18N_NOOP("Outputs the winId of each dialog"), 0 },
    { "embed <winid>", I18N_NOOP("Makes the dialog transient for an X app specified by winid"), 0 },
    { "dontagain <file:entry>", I18N_NOOP("Config file and option name for saving the \"dont-show/ask-again\" state"), 0 },

    { "+[arg]", I18N_NOOP("Arguments - depending on main option"), 0 },
    KCmdLineLastOption
};

// this class hooks into the eventloop and outputs the id
// of shown dialogs or makes the dialog transient for other winids.
// Will destroy itself on app exit.
class WinIdEmbedder: public QObject
{
public:
    WinIdEmbedder(bool printID, WId winId):
        QObject(kapp), print(printID), id(winId)
    {
        if (kapp)
            kapp->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *o, QEvent *e);
private:
    bool print;
    WId id;
};

bool WinIdEmbedder::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Show && o->isWidgetType()
        && o->inherits("KDialog"))
    {
        QWidget *w = static_cast<QWidget*>(o);
        if (print)
            cout << "winId: " << w->winId() << endl;
#ifdef Q_WS_X11
        if (id)
            XSetTransientForHint(w->x11Info().display(), w->winId(), id);
#endif
        deleteLater(); // WinIdEmbedder is not needed anymore after the first dialog was shown
        return false;
    }
    return QObject::eventFilter(o, e);
}

static void outputStringList(QStringList list, bool separateOutput)
{
    if ( separateOutput) {
	for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
	    cout << (*it).toLocal8Bit().data() << endl;
	}
    } else {
	for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
	    cout << (*it).toLocal8Bit().data() << " ";
	}
	cout << endl;
    }
}

static int directCommand(KCmdLineArgs *args)
{
    QString title;
    bool separateOutput = false;
    bool printWId = args->isSet("print-winid");
    bool embed = args->isSet("embed");
    QString defaultEntry;

    // --title text
    KCmdLineArgs *qtargs = KCmdLineArgs::parsedArgs("qt"); // --title is a qt option
    if(qtargs->isSet("title")) {
      title = QFile::decodeName(qtargs->getOption("title"));
    }

    // --separate-output
    if (args->isSet("separate-output"))
    {
      separateOutput = true;
    }
    if (printWId || embed)
    {
      WId id = 0;
      if (embed) {
          bool ok;
          long l = QString(args->getOption("embed")).toLong(&ok);
          if (ok)
              id = (WId)l;
      }
      (void)new WinIdEmbedder(printWId, id);
    }

    // --yesno and other message boxes
    KMessageBox::DialogType type = (KMessageBox::DialogType) 0;
    QByteArray option;
    if (args->isSet("yesno")) {
        option = "yesno";
        type = KMessageBox::QuestionYesNo;
    }
    else if (args->isSet("yesnocancel")) {
        option = "yesnocancel";
        type = KMessageBox::QuestionYesNoCancel;
    }
    else if (args->isSet("warningyesno")) {
        option = "warningyesno";
        type = KMessageBox::WarningYesNo;
    }
    else if (args->isSet("warningcontinuecancel")) {
        option = "warningcontinuecancel";
        type = KMessageBox::WarningContinueCancel;
    }
    else if (args->isSet("warningyesnocancel")) {
        option = "warningyesnocancel";
        type = KMessageBox::WarningYesNoCancel;
    }
    else if (args->isSet("sorry")) {
        option = "sorry";
        type = KMessageBox::Sorry;
    }
    else if (args->isSet("error")) {
        option = "error";
        type = KMessageBox::Error;
    }
    else if (args->isSet("msgbox")) {
        option = "msgbox";
        type = KMessageBox::Information;
    }

    if ( !option.isEmpty() )
    {
        KConfig* dontagaincfg = NULL;
        // --dontagain
        QString dontagain; // QString()
        if (args->isSet("dontagain"))
        {
          QString value = args->getOption("dontagain");
          QStringList values = value.split( ':', QString::SkipEmptyParts );
          if( values.count() == 2 )
          {
            dontagaincfg = new KConfig( values[ 0 ] );
            KMessageBox::setDontShowAskAgainConfig( dontagaincfg );
            dontagain = values[ 1 ];
          }
          else
            qDebug( "Incorrect --dontagain!" );
        }
        int ret;

        QString text = QString::fromLocal8Bit(args->getOption( option ));
        int pos;
        while ((pos = text.indexOf( QLatin1String("\\n") )) >= 0)
        {
            text.replace(pos, 2, QLatin1String("\n"));
        }

        if ( type == KMessageBox::WarningContinueCancel ) {
            /* TODO configurable button texts*/
            ret = KMessageBox::messageBox( 0, type, text, title, KStdGuiItem::cont(),
                KStdGuiItem::no(), dontagain );
        } else {
            ret = KMessageBox::messageBox( 0, type, text, title /*, TODO configurable button texts*/,
                KStdGuiItem::yes(), KStdGuiItem::no(), dontagain );
        }
        delete dontagaincfg;
        // ret is 1 for Ok, 2 for Cancel, 3 for Yes, 4 for No and 5 for Continue.
        // We want to return 0 for ok, yes and continue, 1 for no and 2 for cancel
        return (ret == KMessageBox::Ok || ret == KMessageBox::Yes || ret == KMessageBox::Continue) ? 0
                     : ( ret == KMessageBox::No ? 1 : 2 );
    }

    // --inputbox text [init]
    if (args->isSet("inputbox"))
    {
      QString result;
      QString init;

      if (args->count() > 0)
          init = QString::fromLocal8Bit(args->arg(0));

      bool retcode = Widgets::inputBox(0, title, QString::fromLocal8Bit(args->getOption("inputbox")), init, result);
      cout << result.toLocal8Bit().data() << endl;
      return retcode ? 0 : 1;
    }


    // --password text
    if (args->isSet("password"))
    {
      QString result;
      bool retcode = Widgets::passwordBox(0, title, QString::fromLocal8Bit(args->getOption("password")), result);
      cout << result.data() << endl;
      return retcode ? 0 : 1;
    }

    // --passivepopup
    if (args->isSet("passivepopup"))
      {
        int duration = 0;
        if (args->count() > 0)
            duration = 1000 * QString::fromLocal8Bit(args->arg(0)).toInt();
        if (duration == 0)
            duration = 10000;
	KPassivePopup *popup = KPassivePopup::message( KPassivePopup::Balloon, // style
						       title,
						       QString::fromLocal8Bit( args->getOption("passivepopup") ),
						       QPixmap() /* don't crash 0*/, // icon
						       (QWidget*)0UL, // parent
						       duration );
	QTimer *timer = new QTimer();
	QObject::connect( timer, SIGNAL( timeout() ), kapp, SLOT( quit() ) );
	QObject::connect( popup, SIGNAL( clicked() ), kapp, SLOT( quit() ) );
        timer->setSingleShot( true );
	timer->start( duration );

#ifdef Q_WS_X11
	QString geometry;
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs("kde");
	if (args && args->isSet("geometry"))
		geometry = args->getOption("geometry");
	if ( !geometry.isEmpty()) {
	    int x, y;
	    int w, h;
	    int m = XParseGeometry( geometry.toLatin1(), &x, &y, (unsigned int*)&w, (unsigned int*)&h);
	    if ( (m & XNegative) )
		x = KApplication::desktop()->width()  + x - w;
	    if ( (m & YNegative) )
		y = KApplication::desktop()->height() + y - h;
	    popup->setAnchor( QPoint(x, y) );
	}
#endif
	kapp->exec();
	return 0;
      }

    // --textbox file [width] [height]
    if (args->isSet("textbox"))
    {
        int w = 0;
        int h = 0;

        if (args->count() == 2) {
            w = QString::fromLocal8Bit(args->arg(0)).toInt();
            h = QString::fromLocal8Bit(args->arg(1)).toInt();
        }

        return Widgets::textBox(0, w, h, title, QString::fromLocal8Bit(args->getOption("textbox")));
    }

    // --textinputbox file [width] [height]
    if (args->isSet("textinputbox"))
    {
      int w = 400;
      int h = 200;

      if (args->count() == 4) {
	w = QString::fromLocal8Bit(args->arg(2)).toInt();
	h = QString::fromLocal8Bit(args->arg(3)).toInt();
      }

      QStringList list;
      list.append(QString::fromLocal8Bit(args->getOption("textinputbox")));

      if (args->count() >= 1) {
	for (int i = 0; i < args->count(); i++)
	  list.append(QString::fromLocal8Bit(args->arg(i)));
      }

      QString result;
      int ret = Widgets::textInputBox(0, w, h, title, list, result);
      cout << result.data() << endl;
      return ret;
    }

    // --combobox <text> [tag item] [tag item] ..."
    if (args->isSet("combobox")) {
        QStringList list;
        if (args->count() >= 2) {
            for (int i = 0; i < args->count(); i++) {
                list.append(QString::fromLocal8Bit(args->arg(i)));
            }
            QString text = QString::fromLocal8Bit(args->getOption("combobox"));
	    if (args->isSet("default")) {
	        defaultEntry = args->getOption("default");
	    }
            QString result;
	    bool retcode = Widgets::comboBox(0, title, text, list, defaultEntry, result);
            cout << result.toLocal8Bit().data() << endl;
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // --menu text [tag item] [tag item] ...
    if (args->isSet("menu")) {
        QStringList list;
        if (args->count() >= 2) {
            for (int i = 0; i < args->count(); i++) {
                list.append(QString::fromLocal8Bit(args->arg(i)));
            }
            QString text = QString::fromLocal8Bit(args->getOption("menu"));
	    if (args->isSet("default")) {
	        defaultEntry = args->getOption("default");
	    }
            QString result;
            bool retcode = Widgets::listBox(0, title, text, list, defaultEntry, result);
            if (1 == retcode) { // OK was selected
	        cout << result.toLocal8Bit().data() << endl;
	    }
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // --checklist text [tag item status] [tag item status] ...
    if (args->isSet("checklist")) {
        QStringList list;
        if (args->count() >= 3) {
            for (int i = 0; i < args->count(); i++) {
                list.append(QString::fromLocal8Bit(args->arg(i)));
            }

            QString text = QString::fromLocal8Bit(args->getOption("checklist"));
            QStringList result;

            bool retcode = Widgets::checkList(0, title, text, list, separateOutput, result);

            int i;
            for (i=0; i<result.count(); i++)
                if (!result.at(i).toLocal8Bit().isEmpty()) {
		    cout << result.at(i).toLocal8Bit().data() << endl;
		}
            exit( retcode ? 0 : 1 );
        }
        return -1;
    }

    // --radiolist text width height menuheight [tag item status]
    if (args->isSet("radiolist")) {
        QStringList list;
        if (args->count() >= 3) {
            for (int i = 0; i < args->count(); i++) {
                list.append(QString::fromLocal8Bit(args->arg(i)));
            }

            QString text = QString::fromLocal8Bit(args->getOption("radiolist"));
            QString result;
            bool retcode = Widgets::radioBox(0, title, text, list, result);
            cout << result.toLocal8Bit().data() << endl;
            exit( retcode ? 0 : 1 );
        }
        return -1;
    }

    // getopenfilename [startDir] [filter]
    if (args->isSet("getopenfilename")) {
        QString startDir;
        QString filter;
        startDir = QString::fromLocal8Bit(args->getOption("getopenfilename"));
        if (args->count() >= 1)  {
            filter = QString::fromLocal8Bit(args->arg(0));
        }
	KFileDialog dlg( startDir, filter, 0 );
	dlg.setOperationMode( KFileDialog::Opening );

	if (args->isSet("multiple")) {
	    dlg.setMode(KFile::Files | KFile::LocalOnly);
	} else {
	    dlg.setMode(KFile::File | KFile::LocalOnly);
	}
	Widgets::handleXGeometry(&dlg);
	kapp->setTopWidget( &dlg );
	dlg.setCaption(title.isNull() ? i18n("Open") : title);
	dlg.exec();

        if (args->isSet("multiple")) {
	    QStringList result = dlg.selectedFiles();
	    if ( !result.isEmpty() ) {
		outputStringList( result, separateOutput );
		return 0;
	    }
	} else {
	    QString result = dlg.selectedFile();
	    if (!result.isEmpty())  {
		cout << result.toLocal8Bit().data() << endl;
		return 0;
	    }
	}
        return 1; // canceled
    }


    // getsaveurl [startDir] [filter]
    // getsavefilename [startDir] [filter]
    if ( (args->isSet("getsavefilename") ) || (args->isSet("getsaveurl") ) ) {
        QString startDir;
        QString filter;
	if ( args->isSet("getsavefilename") ) {
	    startDir = QString::fromLocal8Bit(args->getOption("getsavefilename"));
	} else {
	    startDir = QString::fromLocal8Bit(args->getOption("getsaveurl"));
	}
        if (args->count() >= 1)  {
            filter = QString::fromLocal8Bit(args->arg(0));
        }
	// copied from KFileDialog::getSaveFileName(), so we can add geometry
	bool specialDir = ( startDir.at(0) == ':' );
	KFileDialog dlg( specialDir ? startDir : QString(), filter, 0 );
	if ( !specialDir )
	    dlg.setSelection( startDir );
	dlg.setOperationMode( KFileDialog::Saving );
	Widgets::handleXGeometry(&dlg);
	kapp->setTopWidget( &dlg );
	dlg.setCaption(title.isNull() ? i18n("Save As") : title);
	dlg.exec();

	if ( args->isSet("getsaveurl") ) {
	    KUrl result = dlg.selectedUrl();
	    if ( result.isValid())  {

		cout << result.url().toLocal8Bit().data() << endl;
		return 0;
	    }
	} else { // getsavefilename
	    QString result = dlg.selectedFile();
	    if (!result.isEmpty())  {
		KRecentDocument::add(result);
		cout << result.toLocal8Bit().data() << endl;
		return 0;
	    }
	}
        return 1; // canceled
    }

    // getexistingdirectory [startDir]
    if (args->isSet("getexistingdirectory")) {
        QString startDir;
        startDir = QString::fromLocal8Bit(args->getOption("getexistingdirectory"));
	QString result;
#ifdef Q_WS_WIN
	result = QFileDialog::getExistingDirectory(startDir, 0, "getExistingDirectory",
							   title, true, true);
#else
	KUrl url;
	KDirSelectDialog myDialog( startDir, true, 0 );

	kapp->setTopWidget( &myDialog );

	Widgets::handleXGeometry(&myDialog);
	if ( !title.isNull() )
	    myDialog.setCaption( title );

	if ( myDialog.exec() == QDialog::Accepted )
	    url =  myDialog.url();

	if ( url.isValid() )
	    result = url.path();
#endif
        if (!result.isEmpty())  {
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }

    // getopenurl [startDir] [filter]
    if (args->isSet("getopenurl")) {
        QString startDir;
        QString filter;
        startDir = QString::fromLocal8Bit(args->getOption("getopenurl"));
        if (args->count() >= 1)  {
            filter = QString::fromLocal8Bit(args->arg(0));
        }
	KFileDialog dlg( startDir, filter, 0 );
	dlg.setOperationMode( KFileDialog::Opening );

	if (args->isSet("multiple")) {
	    dlg.setMode(KFile::Files);
	} else {
	    dlg.setMode(KFile::File);
	}
	Widgets::handleXGeometry(&dlg);
	kapp->setTopWidget( &dlg );
	dlg.setCaption(title.isNull() ? i18n("Open") : title);
	dlg.exec();

        if (args->isSet("multiple")) {
	    KUrl::List result = dlg.selectedUrls();
	    if ( !result.isEmpty() ) {
		outputStringList( result.toStringList(), separateOutput );
		return 0;
	    }
	} else {
	    KUrl result = dlg.selectedUrl();
	    if (!result.isEmpty())  {
		cout << result.url().toLocal8Bit().data() << endl;
		return 0;
	    }
	}
        return 1; // canceled
    }

    // geticon [group] [context]
    if (args->isSet("geticon")) {
        QString groupStr, contextStr;
        groupStr = QString::fromLocal8Bit(args->getOption("geticon"));
        if (args->count() >= 1)  {
            contextStr = QString::fromLocal8Bit(args->arg(0));
        }
        K3Icon::Group group = K3Icon::NoGroup;
        if ( groupStr == QLatin1String( "Desktop" ) )
            group = K3Icon::Desktop;
        else if ( groupStr == QLatin1String( "Toolbar" ) )
            group = K3Icon::Toolbar;
        else if ( groupStr == QLatin1String( "MainToolbar" ) )
            group = K3Icon::MainToolbar;
        else if ( groupStr == QLatin1String( "Small" ) )
            group = K3Icon::Small;
        else if ( groupStr == QLatin1String( "Panel" ) )
            group = K3Icon::Panel;
        else if ( groupStr == QLatin1String( "User" ) )
            group = K3Icon::User;
        K3Icon::Context context = K3Icon::Any;
        // From kicontheme.cpp
        if ( contextStr == QLatin1String( "Devices" ) )
            context = K3Icon::Device;
        else if ( contextStr == QLatin1String( "MimeTypes" ) )
            context = K3Icon::MimeType;
        else if ( contextStr == QLatin1String( "FileSystems" ) )
            context = K3Icon::FileSystem;
        else if ( contextStr == QLatin1String( "Applications" ) )
            context = K3Icon::Application;
        else if ( contextStr == QLatin1String( "Actions" ) )
            context = K3Icon::Action;

	KIconDialog dlg((QWidget*)0L);
	kapp->setTopWidget( &dlg );
	dlg.setup( group, context);
	if (!title.isNull())
	    dlg.setCaption(title);
	Widgets::handleXGeometry(&dlg);

	QString result = dlg.openDialog();

        if (!result.isEmpty())  {
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }

    // --progressbar text totalsteps
    if (args->isSet("progressbar"))
    {
       cout << "org.kde.kdialog_" << getpid() << " /ProgressDialog" << endl;
       if (fork())
           exit(0);
       close(1);

       int totalsteps = 100;
       QString text = QString::fromLocal8Bit(args->getOption("progressbar"));

       if (args->count() == 1)
           totalsteps = QString::fromLocal8Bit(args->arg(0)).toInt();

       return Widgets::progressBar(0, title, text, totalsteps) ? 1 : 0;
    }

    KCmdLineArgs::usage();
    return -2; // NOTREACHED
}


int main(int argc, char *argv[])
{
  KAboutData aboutData( "kdialog", I18N_NOOP("KDialog"),
                        "1.0", I18N_NOOP( "KDialog can be used to show nice dialog boxes from shell scripts" ),
			KAboutData::License_GPL,
                        "(C) 2000, Nick Thompson");
  aboutData.addAuthor("David Faure", I18N_NOOP("Current maintainer"),"faure@kde.org");
  aboutData.addAuthor("Brad Hards", 0, "bradh@frogmouth.net");
  aboutData.addAuthor("Nick Thompson",0, 0/*"nickthompson@lucent.com" bounces*/);
  aboutData.addAuthor("Matthias Hölzer",0,"hoelzer@kde.org");
  aboutData.addAuthor("David Gümbel",0,"david.guembel@gmx.net");
  aboutData.addAuthor("Richard Moore",0,"rich@kde.org");
  aboutData.addAuthor("Dawit Alemayehu",0,"adawit@kde.org");

  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  // execute direct kdialog command
  return directCommand(args);
}

