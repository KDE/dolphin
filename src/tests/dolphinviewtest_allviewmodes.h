/*****************************************************************************
 *   Copyright (C) 2010-2011 by Frank Reininghaus (frank78ac@googlemail.com) *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA              *
 *****************************************************************************/

#ifndef DOLPHINVIEWTEST_ALLVIEWMODES
#define DOLPHINVIEWTEST_ALLVIEWMODES

#include "testbase.h"

#include "views/dolphinview.h"

/**
 * DolphinViewTest_AllViewModes is used as a base class for tests that check the
 * basic functionality of DolphinView in all view modes. The derived classes
 * have to provide implementations for the virtual methods mode() and verifyCorrectViewMode(),
 * see below.
 *
 * Tests for DolphinView functionality that is specific to a particular view mode or
 * to switching between different view modes should not be added here, but to another
 * DolphinView unit test.
 */

class DolphinViewTest_AllViewModes : public TestBase
{
    Q_OBJECT

public:

    DolphinViewTest_AllViewModes();

private slots:

    void init();
    void cleanup();

    void testSelection();
    void testViewPropertySettings();
    void testZoomLevel();
    void testSaveAndRestoreState();

    void testKeyboardFocus();

public:

    /** Returns the view mode (Icons, Details, Columns) to be used in the test. */
    virtual DolphinView::Mode mode() const = 0;

    /** Should return true if the view mode is correct. */
    virtual bool verifyCorrectViewMode() const = 0;

    /**
     * Waits for the DolphinView's selectionChanged(const KFileItemList&) to be emitted
     * and verifies that the number of selected items is as expected.
     */
    void verifySelectedItemsCount(int) const;

};

#endif
