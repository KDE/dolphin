//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002 David Faure <faure@kde.org>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include <qptrlist.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qregexp.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kdebug.h>
//#include <ktopwidget.h>
#include <kmainwindow.h>

#include "widgets.h"

#include <klocale.h>
#include <qdialog.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kfiledialog.h>

using namespace std;

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
    { "menu <text> [tag item] [tag item] ...", I18N_NOOP("Menu dialog"), 0 },
    { "checklist <text> [tag item status] ...", I18N_NOOP("Check List dialog"), 0 },
    { "radiolist <text> [tag item status] ...", I18N_NOOP("Radio List dialog"), 0 },
    { "getopenfilename [startDir] [filter]", I18N_NOOP("File dialog to open an existing file"), 0 },
    { "getsavefilename [startDir] [filter]", I18N_NOOP("File dialog to save a file"), 0 },
    { "getexistingdirectory [startDir]", I18N_NOOP("File dialog to select an existing directory"), 0 },
    { "getopenurl [startDir] [filter]", I18N_NOOP("File dialog to open an existing URL"), 0 },
    { "getsaveurl [startDir] [filter]", I18N_NOOP("File dialog to save a URL"), 0 },

    // TODO gauge stuff, reading values from stdin


    { "title <text>", I18N_NOOP("Dialog title"), 0 },
    { "separate-output", I18N_NOOP("Return list items on separate lines (for checklist option)"), 0 },

    { "+[arg]", I18N_NOOP("Arguments - depending on main option"), 0 },
    KCmdLineLastOption
};

// string to int, with default value
int convert(const QString &val, int def)
{
  bool ok;
  int result = val.toInt(&ok);
  if (!ok)
    result = def;
  return result;
}

int directCommand(KCmdLineArgs *args)
{
    QString title;
    bool separateOutput = FALSE;

    // --title text
    KCmdLineArgs *qtargs = KCmdLineArgs::parsedArgs("qt"); // --title is a qt option
    if(qtargs->isSet("title")) {
      title = QFile::decodeName(qtargs->getOption("title"));
    }

    // --separate-output
    if (args->isSet("separate-output"))
    {
      separateOutput = TRUE;
    }

    // --yesno and other message boxes
    KMessageBox::DialogType type = (KMessageBox::DialogType) 0;
    QCString option;
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
        QString text = QString::fromLocal8Bit(args->getOption( option ));
        int pos;
        while ((pos = text.find( QString::fromLatin1("\\n") )) >= 0)
        {
            text.replace(pos, 2, QString::fromLatin1("\n"));
        }

        int ret = KMessageBox::messageBox( 0, type, text, title /*, TODO configurable button texts*/ );
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
      cout << result.local8Bit().data() << endl;
      return retcode ? 0 : 1;
    }


    // --password text
    if (args->isSet("password"))
    {
      QCString result;
      bool retcode = Widgets::passwordBox(0, title, QString::fromLocal8Bit(args->getOption("password")), result);
      cout << result.data() << endl;
      return retcode ? 0 : 1;
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

    // --menu text [tag item] [tag item] ...
    if (args->isSet("menu")) {
        QStringList list;
        if (args->count() >= 2) {
            for (int i = 0; i < args->count(); i++) {
                list.append(QString::fromLocal8Bit(args->arg(i)));
            }
            QString text = QString::fromLocal8Bit(args->getOption("menu"));
            QString result;
            bool retcode = Widgets::listBox(0, title, text, list, result);
            cout << result.local8Bit().data() << endl;
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

            unsigned int i;
            for (i=0; i<result.count(); i++)
                cout << result[i].local8Bit().data() << endl;
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
            cout << result.local8Bit().data() << endl;
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
        QString result = KFileDialog::getOpenFileName( startDir, filter, 0, title );
        if (!result.isEmpty())  {
            cout << result.local8Bit().data() << endl;
            return 0;
        }
        return 1; // cancelled
    }

    // getsavefilename [startDir] [filter]
    if (args->isSet("getsavefilename")) {
        QString startDir;
        QString filter;
	startDir = QString::fromLocal8Bit(args->getOption("getsavefilename"));
	if (args->count() >= 1)  {
	    filter = QString::fromLocal8Bit(args->arg(0));
	}
        QString result = KFileDialog::getSaveFileName( startDir, filter, 0, title );
        if (!result.isEmpty())  {
            cout << result.local8Bit().data() << endl;
            return 0;
        }
        return 1; // cancelled
    }

    // getexistingdirectory [startDir]
    if (args->isSet("getexistingdirectory")) {
        QString startDir;
	startDir = QString::fromLocal8Bit(args->getOption("getexistingdirectory"));
        QString result = KFileDialog::getExistingDirectory( startDir, 0, title );
        if (!result.isEmpty())  {
            cout << result.local8Bit().data() << endl;
            return 0;
        }
        return 1; // cancelled
    }

    // getopenurl [startDir] [filter]
    if (args->isSet("getopenurl")) {
        QString startDir;
        QString filter;
	startDir = QString::fromLocal8Bit(args->getOption("getopenurl"));
	if (args->count() >= 1)  {
	    filter = QString::fromLocal8Bit(args->arg(0));
	}
        KURL result = KFileDialog::getOpenURL( startDir, filter, 0, title );
        if (!result.isEmpty())  {
            cout << result.url().local8Bit().data() << endl;
            return 0;
        }
        return 1; // cancelled
    }

    // getsaveurl [startDir] [filter]
    if (args->isSet("getsaveurl")) {
        QString startDir;
        QString filter;
	startDir = QString::fromLocal8Bit(args->getOption("getsaveurl"));
	if (args->count() >= 1)  {
	    filter = QString::fromLocal8Bit(args->arg(0));
	}
        KURL result = KFileDialog::getSaveURL( startDir, filter, 0, title );
        if (!result.isEmpty())  {
            cout << result.url().local8Bit().data() << endl;
            return 0;
        }
        return 1; // cancelled
    }

    KCmdLineArgs::usage();
    return -2; // NOTREACHED
}


int main(int argc, char *argv[])
{
  KAboutData aboutData( "kdialog", I18N_NOOP("KDialog"),
                        "0.9", I18N_NOOP( "KDialog can be used to show nice dialog boxes from shell scripts" ), KAboutData::License_BSD,
                        "(C) 2000, Nick Thompson");
  aboutData.addAuthor("David Faure", I18N_NOOP("Current maintainer"),"faure@kde.org");
  aboutData.addAuthor("Nick Thompson",0, 0/*"nickthompson@lucent.com" bounces*/);
  aboutData.addAuthor("Matthias Hölzer",0,"hoelzer@kde.org");
  aboutData.addAuthor("David Gümbel",0,"david.guembel@gmx.net");
  aboutData.addAuthor("Richard Moore",0,"rich@kde.org");

  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  // execute direct kdialog command
  return directCommand(args);
}
