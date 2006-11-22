/***************************************************************************
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
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

#include "sidebar.h"

#include <qlayout.h>
#include <qpixmap.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <kiconloader.h>
#include <klocale.h>
#include <qcombobox.h>

#include "dolphinsettings.h"
#include "sidebarsettings.h"
#include "bookmarkssidebarpage.h"
#include "infosidebarpage.h"

#include <assert.h>

Sidebar::Sidebar(QWidget* parent) :
    QWidget(parent),
    m_pagesSelector(0),
    m_page(0),
    m_layout(0)
{
    m_layout = new Q3VBoxLayout(this);

    m_pagesSelector = new QComboBox(this);
    m_pagesSelector->insertItem(i18n("Information"));
    m_pagesSelector->insertItem(i18n("Bookmarks"));

    // Assure that the combo box has the same height as the Url navigator for
    // a clean layout.
    // TODO: the following 2 lines have been copied from the UrlNavigator
    // constructor (-> provide a shared height setting?)
    QFontMetrics fontMetrics(font());
    m_pagesSelector->setMinimumHeight(fontMetrics.height() + 8);

    SidebarSettings* settings = DolphinSettings::instance().sidebarSettings();
    const int selectedIndex = indexForName(settings->selectedPage());
    m_pagesSelector->setCurrentItem(selectedIndex);
    m_layout->addWidget(m_pagesSelector);

    createPage(selectedIndex);

    connect(m_pagesSelector, SIGNAL(activated(int)),
            this, SLOT(createPage(int)));
}

Sidebar::~Sidebar()
{
}

QSize Sidebar::sizeHint() const
{
    QSize size(QWidget::sizeHint());

    SidebarSettings* settings = DolphinSettings::instance().sidebarSettings();
    size.setWidth(settings->width());
    return size;
}

void Sidebar::createPage(int index)
{
    if (m_page != 0) {
        m_page->deleteLater();
        m_page = 0;
    }

    switch (index) {
        case 0: m_page = new InfoSidebarPage(this); break;
        case 1: m_page = new BookmarksSidebarPage(this); break;
        default: break;
    }

    m_layout->addWidget(m_page);
    m_page->show();

    SidebarSettings* settings = DolphinSettings::instance().sidebarSettings();
    settings->setSelectedPage(m_pagesSelector->text(index));
}

int Sidebar::indexForName(const QString& name) const
{
    const int count = m_pagesSelector->count();
    for (int i = 0; i < count; ++i) {
        if (m_pagesSelector->text(i) == name) {
            return i;
        }
    }

    return 0;
}

#include "sidebar.moc"
