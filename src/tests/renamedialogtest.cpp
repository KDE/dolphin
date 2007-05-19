/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include <qtest_kde.h>

#include "renamedialogtest.h"
#include <renamedialog.h>

QTEST_KDEMAIN(RenameDialogTest, NoGUI)

void RenameDialogTest::testExtensionString()
{
    QString result;

    result = RenameDialog::extensionString("Image.gif");
    QCOMPARE(result, QString(".gif"));

    result = RenameDialog::extensionString("package.tar.gz");
    QCOMPARE(result, QString(".tar.gz"));

    result = RenameDialog::extensionString("cmake-2.4.5");
    QCOMPARE(result, QString());

    result = RenameDialog::extensionString("Image.1.12.gif");
    QCOMPARE(result, QString(".gif"));

    result = RenameDialog::extensionString("Image.tar.1.12.gz");
    QCOMPARE(result, QString(".tar.1.12.gz"));
}

#include "renamedialogtest.moc"
