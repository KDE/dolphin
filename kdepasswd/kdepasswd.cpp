/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <config.h>
#include <stdlib.h>

#include <qcstring.h>

#include <kapp.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>

#include "passwd.h"
#include "passwddlg.h"

static KCmdLineOptions options[] = 
{
    { "user", I18N_NOOP("Change password of this user."), 0 },
    { 0, 0, 0 }
};


int main(int argc, char **argv)
{
    KAboutData aboutData("kdepasswd", I18N_NOOP("KDE passwd"),
            VERSION, I18N_NOOP("Changes a Unix password."),
            KAboutData::License_Artistic, "Copyright (c) 2000 Geert Jansen");
    aboutData.addAuthor("Geert Jansen", I18N_NOOP("Maintainer"),
            "jansen@kde.org", "http://www.stack.nl/~geertj/");
 
    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app;

    QCString user;
    if (args->count())
	user = args->arg(0);

    QCString oldpass;
    int result = KDEpasswd1Dialog::getPassword(oldpass, user);
    if (result != KDEpasswd1Dialog::Accepted)
	exit(0);

    KDEpasswd2Dialog *dlg = new KDEpasswd2Dialog(oldpass, user);
    dlg->exec();

    if (dlg->result() == KDEpasswd2Dialog::Accepted)
	KMessageBox::information(0L, i18n("Your password has been changed."));
    return 0;
}

