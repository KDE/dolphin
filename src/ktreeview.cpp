/***************************************************************************
 *   Copyright (C) 2008 by <haraldhv (at) stud.ntnu.no>                    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include <KGlobalSettings>

#include <QDebug>
#include <QScrollBar>

#include "ktreeview.h"
#include "ktreeview_p.h"

KTreeView::KTreeViewPrivate::KTreeViewPrivate(KTreeView *parent)
	: parent(parent),
	autoHorizontalScroll(true),
	scrollTowards(0),
	scrollPixels(5),
	scrollDelay(50),
	leftSideMargin(30),
	considerDelay(500),
	topLeftPoint(QPoint(10,10))
{
	Q_ASSERT(parent->verticalScrollBar());

	considerDelayTimer.setInterval(considerDelay);

	connect( &considerDelayTimer,
			SIGNAL(timeout()),
			this,
			SLOT(considerAutoScroll())
		   );

	connect( parent->verticalScrollBar(),
			SIGNAL(rangeChanged(int, int)),
			&considerDelayTimer,
			SLOT(start())
		   );

	connect( parent->verticalScrollBar(),
			SIGNAL(valueChanged(int)),
			&considerDelayTimer,
			SLOT(start())
		   );

	connect( parent,
			SIGNAL( collapsed ( const QModelIndex &)),
			&considerDelayTimer,
			SLOT(start())
		   );

	connect( parent,
			SIGNAL( expanded ( const QModelIndex &)),
			&considerDelayTimer,
			SLOT(start())
		   );

}

void KTreeView::KTreeViewPrivate::considerAutoScroll()
{
	qDebug() << "Considering auto scroll";

	QModelIndex i = parent->indexAt(topLeftPoint);
	int smallest = parent->width();

	while (i.isValid())
	{
		QRect r = parent->visualRect(i);
		if (r.top() > parent->height())
			break;

		int leftSide = r.left();

		smallest = qMin(smallest, leftSide);
		i = parent->indexBelow(i);
	}

	int currentScroll = parent->horizontalScrollBar()->value();

	setScrollTowards(smallest + currentScroll - leftSideMargin);

	considerDelayTimer.stop();

}

void KTreeView::KTreeViewPrivate::autoScrollTimeout()
{

	Q_ASSERT(parent);

	QScrollBar *scrollBar = parent->horizontalScrollBar();
	if (scrollBar == NULL)
	{
		qDebug() << "Warning: no scrollbar present, but told to scroll.";
		scrollTimer.stop();
		return;
	}

	int currentScroll = scrollBar->value();

	int difference = currentScroll - scrollTowards;

	if (qAbs(difference) < scrollPixels)
	{
		scrollBar->setValue(scrollTowards);
		scrollTimer.stop();
		return;
	}

	if (difference < 0)
	{
		scrollBar->setValue(currentScroll + scrollPixels);
	}
	else
	{
		scrollBar->setValue(currentScroll - scrollPixels);
	}
}

void KTreeView::KTreeViewPrivate::setScrollTowards( int scrollTowards )
{
	if (scrollTowards < 0)
		scrollTowards = 0;
	this->scrollTowards = scrollTowards;
	scrollTimer.start(scrollDelay);
}

//************************************************

KTreeView::KTreeView(QWidget *parent)
	: QTreeView(parent)
	, d(new KTreeViewPrivate(this))
{
	/* The graphicEffectsLevel was not available in the 4.0.3 version of
	 * the libs I was compiling with, so this is left out for now and
	 * enabled by default...
	 */
	//if (KGlobalSettings::graphicEffectsLevel() >=
			//KGlobalSettings::SimpleAnimationEffects)
	//{
		setAutoHorizontalScroll(true);
	//}
		connect(
				&d->scrollTimer,
				SIGNAL(timeout()),
				d,
				SLOT(autoScrollTimeout())
			   );

}

void KTreeView::setAutoHorizontalScroll(bool value)
{
	d->autoHorizontalScroll = value;
}

bool KTreeView::autoHorizontalScroll( void )
{
	return d->autoHorizontalScroll;
}

#include "ktreeview.moc"
#include "ktreeview_p.moc"
