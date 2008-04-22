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

#ifndef _KTREEVIEW_P_H_
#define _KTREEVIEW_P_H_

#include <QTimer>
#include <QObject>

#include "ktreeview.h"

class KTreeView::KTreeViewPrivate : public QObject
{
	Q_OBJECT



	public Q_SLOTS:
		void autoScrollTimeout();
		void considerAutoScroll();

	public:

		KTreeViewPrivate(KTreeView *parent);
		//~KTreeViewPrivate();
		KTreeView *parent;

		//Function for start scrolling towards a certain position
		void setScrollTowards( int scrollTowards );

		//Privates for doing the scrolling
		QTimer scrollTimer;
		QTimer considerDelayTimer;
		bool autoHorizontalScroll;
		int scrollTowards;

		//Constants
		const int scrollPixels;
		const int scrollDelay;
		const int leftSideMargin;
		const int considerDelay;
		const QPoint topLeftPoint;


};

#endif /* ifndef _KTREEVIEW_P_H_ */
