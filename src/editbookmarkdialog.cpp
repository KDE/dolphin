/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "editbookmarkdialog.h"
#include <q3grid.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <klocale.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <kiconloader.h>
#include <qpushbutton.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kicondialog.h>
#include <kvbox.h>


EditBookmarkDialog::~EditBookmarkDialog()
{
}

KBookmark EditBookmarkDialog::getBookmark(const QString& title,
                                          const QString& name,
                                          const KUrl& url,
                                          const QString& icon)
{
    EditBookmarkDialog dialog(title, name, url, icon);
    dialog.exec();
    return dialog.m_bookmark;
}

void EditBookmarkDialog::slotButtonClicked(int button)
{
    if (button==Ok) {
        m_bookmark = KBookmark::standaloneBookmark(m_name->text(),
                                                   KUrl(m_location->text()),
                                                   m_iconName);
    }

    KDialog::slotButtonClicked(button);
}

EditBookmarkDialog::EditBookmarkDialog(const QString& title,
                                       const QString& name,
                                       const KUrl& url,
                                       const QString& icon) :
    KDialog(),
    m_iconButton(0),
    m_name(0),
    m_location(0)
{
    setCaption(title);
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    Q3VBoxLayout* topLayout = new Q3VBoxLayout(page, 0, spacingHint());

    Q3Grid* grid = new Q3Grid(2, Qt::Horizontal, page);
    grid->setSpacing(spacingHint());

    // create icon widgets
    new QLabel(i18n("Icon:"), grid);
    m_iconName = icon;
    m_iconButton = new QPushButton(SmallIcon(m_iconName), QString::null, grid);
    m_iconButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_iconButton, SIGNAL(clicked()),
            this, SLOT(selectIcon()));

    // create name widgets
    new QLabel(i18n("Name:"), grid);
    m_name = new QLineEdit(name, grid);
    m_name->selectAll();
    m_name->setFocus();

    // create location widgets
    new QLabel(i18n("Location:"), grid);

    KHBox* locationBox = new KHBox(grid);
    locationBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    locationBox->setSpacing(spacingHint());
    m_location = new QLineEdit(url.prettyUrl(), locationBox);
    m_location->setMinimumWidth(320);

    QPushButton* selectLocationButton = new QPushButton(SmallIcon("folder"), QString::null, locationBox);
    selectLocationButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(selectLocationButton, SIGNAL(clicked()),
            this, SLOT(selectLocation()));

    topLayout->addWidget(grid);
}

void EditBookmarkDialog::selectIcon()
{
    const QString iconName(KIconDialog::getIcon(K3Icon::Small, K3Icon::FileSystem));
    if (!iconName.isEmpty()) {
        m_iconName = iconName;
        m_iconButton->setIconSet(SmallIcon(iconName));
    }
}

void EditBookmarkDialog::selectLocation()
{
    const QString location(m_location->text());
    KUrl url(KFileDialog::getExistingUrl(location));
    if (!url.isEmpty()) {
        m_location->setText(url.prettyUrl());
    }
}

#include "editbookmarkdialog.moc"
