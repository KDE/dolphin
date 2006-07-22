/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __kebsearchline_h
#define __kebsearchline_h

#include <k3listviewsearchline.h>
#include <kglobal.h>
#include <klocale.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <QTimer>
#include <QAbstractItemView>
#include <QListView>
#include <QTreeView>
#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QApplication>
#include <QLabel>
#include <QLinkedList>



class KViewSearchLinePrivate;
/**
 * This class makes it easy to add a search line for filtering the items in a
 * QListView/QTreeView based on a simple text search.
 *
 * No changes to the application other than instantiating this class with an
 * appropriate QListView/QTreeView should be needed.
 *
 * This class automatically responds to the dataChanged(), rowsInserted(),
 * rowsRemoved() and similar signals.
 * This means that the view needs to be bound to a model().
 *
 * Note: Don't call setModel() on the view while a KViewSearchLine filters
 * the view. (Instead call setView(0) before and setView(view) after calling
 * setModel()
 * 
 *
 * Note: You need to call updateSearch() if you called QListView::setModelColumn()
 */

//FIXME delete KViewSearchLine if there is a replacement in kdelibs
class KViewSearchLine : public KLineEdit
{
    Q_OBJECT

public:
    /**
     * Constructs a KViewSearchLine with \a view being the QTreeView/QListView
     * to be filtered.
     *
     * If \a view is null then the widget will be disabled until a listview
     * is set with setListView().
     */
    KViewSearchLine(QWidget *parent = 0, QAbstractItemView *view = 0);

    /**
     * Constructs a KViewSearchLine without any QListView/QTreeView to filter. The
     * QListView/QTreeView object has to be set later with setListView(). 
     */
    KViewSearchLine(QWidget *parent);

    /**
     * Destroys the KViewSearchLine.
     */
    virtual ~KViewSearchLine();

    /**
     * Returns true if the search is case sensitive.  This defaults to false.
     *
     * @see setCaseSensitive()
     */
    bool caseSensitive() const;

    /**
     * Returns the current list of columns that will be searched.  If the
     * returned list is empty all visible columns will be searched.
     *
     * @see setSearchColumns
     */
    QLinkedList<int> searchColumns() const;

    /**
     * If this is true (the default) then the parents of matched items will also
     * be shown.
     *
     * @see setKeepParentsVisible()
     */
    bool keepParentsVisible() const;

    /**
     * Returns the view that is currently filtered by the search.
     *
     * @see setView()
     */
    QAbstractItemView *view() const;

public Q_SLOTS:
    /**
     * Updates search to only make visible the items that match \a s.  If
     * \a s is null then the line edit's text will be used.
     */
    virtual void updateSearch(const QString &s = QString());

    /**
     * Make the search case sensitive or case insensitive.
     *
     * @see caseSenstive()
     */
    void setCaseSensitive(bool cs);

    /**
     * When a search is active on a list that's organized into a tree view if
     * a parent or ancesestor of an item is does not match the search then it
     * will be hidden and as such so too will any children that match.
     *
     * If this is set to true (the default) then the parents of matching items
     * will be shown.
     *
     * This applys only to QTreeViews.
     *
     * @see keepParentsVisible
     */
    void setKeepParentsVisible(bool v);

    /**
     * Sets the list of columns to be searched.  The default is to search all,
     * visible columns which can be restored by passing \a columns as an empty
     * list.
     * This has no effect if the view is a QListView.
     *
     * @see searchColumns
     */
    void setSearchColumns(const QLinkedList<int> &columns);

    /**
     * Sets the view that is filtered by this search line.  
     * If \a v is null then the widget will be disabled.
     * v must be either a QListView or a QTreeView
     * (That includes QListWidget and QTreeWidget)
     * @see view()
     */
    void setView(QAbstractItemView *v);

protected:

    /**
     * Returns true if the row including \a item matches the search \a s.
     * This will be evaluated based on the value of caseSensitive() and
     * searchColumns().  This can be overridden in subclasses to implement 
     * more complicated matching schemes.
     */
    virtual bool itemMatches(const QModelIndex & item, const QString &s) const;

    /**
    * Re-implemented for internal reasons.  API not affected.
    */
    virtual void contextMenuEvent( QContextMenuEvent*e );

protected Q_SLOTS:
    /**
     * When keys are pressed a new search string is created and a timer is
     * activated.  The most recent search is activated when this timer runs out
     * if another key has not yet been pressed.
     *
     * This method makes @param search the most recent search and starts the
     * timer.
     *
     * Together with activateSearch() this makes it such that searches are not
     * started until there is a short break in the users typing.
     *
     * @see activateSearch()
     */
    void queueSearch(const QString &search);

    /**
     * When the timer started with queueSearch() expires this slot is called.
     * If there has been another timer started then this slot does nothing.
     * However if there are no other pending searches this starts the list view
     * search.
     *
     * @see queueSearch()
     */
    void activateSearch();

private:
    /**
     * QListView's and QTreeView's setRowHidden are slightly different.
     */
    void setVisible(QModelIndex index, bool v);

    /**
     * This is used in case parent items of matching items shouldn't be
     * visible.  It hides all items that don't match the search string.
     */
    void checkItemParentsNotVisible();

    /**
     * This is used in case parent items of matching items should be visible.
     * It makes a recursive call to all children.  It returns true if at least
     * one item in the subtree with the given root item is visible.
     */
    bool checkItemParentsVisible(QModelIndex index);

    /**
     * returns whether any row between first and last is visible
     */
    bool anyVisible(const QModelIndex & first, const QModelIndex & last);

    /**
     * rechecks indecies first-last after a dataChanged() signal
     * sets their visibility and returns true if any item should be
     * visible
     */
    bool recheck(const QModelIndex & first, const QModelIndex & last);

    /**
     * Hide QListView/QTreeView's different isRowHidden
     */
    bool isVisible(const QModelIndex & index);

    /**
     * returns the model() of the view()
     */
    QAbstractItemModel * model() const;

    /**
     * returns the index exactly one row below \p index
     */
    QModelIndex nextRow(const QModelIndex & index);

private Q_SLOTS:
    void listViewDeleted();
    void slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void slotRowsInserted(const QModelIndex &parent, int first, int last);
    void slotRowsRemoved(const QModelIndex &parent, int first, int last);
    void slotColumnsInserted(const QModelIndex &parent, int first, int last);
    void slotColumnsRemoved(const QModelIndex &parent, int first, int last);
    void slotModelReset();
    void searchColumnsMenuActivated(QAction * act);

private:
    class KViewSearchLinePrivate;
    KViewSearchLinePrivate *d;
        QVector<QAction *> actions;
};

/**
 * Creates a widget featuring a KViewSearchLine, a label with the text
 * "Search" and a button to clear the search.
 */
class KViewSearchLineWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Creates a KListViewSearchLineWidget for \a view with \a parent as the
     * parent
     */
    KViewSearchLineWidget(QAbstractItemView *view = 0, QWidget *parent = 0);

    /**
     * Destroys the KListViewSearchLineWidget
     */
    ~KViewSearchLineWidget();

    /**
     * Creates the search line.  This can be useful to reimplement in cases where
     * a K3ListViewSearchLine subclass is used.
     */
    virtual KViewSearchLine *createSearchLine(QAbstractItemView *view);

    /**
     * Returns a pointer to the search line.
     */
    KViewSearchLine *searchLine() const;

protected Q_SLOTS:
    /**
     * Creates the widgets inside of the widget.  This is called from the
     * constructor via a single shot timer so that it it guaranteed to run
     * after construction is complete.  This makes it suitable for overriding in
     * subclasses.
     */
    virtual void createWidgets();

private:
    class KViewSearchLineWidgetPrivate;
    KViewSearchLineWidgetPrivate *d;
};


//FIXME reimplement on top of the searchline
class KEBSearchLine : public K3ListViewSearchLine
{
public:
    KEBSearchLine(QWidget *parent = 0, K3ListView *listView = 0);

    KEBSearchLine(QWidget *parent);

    virtual ~KEBSearchLine();

    enum modes { EXACTLY, AND, OR } mmode;
    modes mode();
    void setMode(modes m);

protected:

    virtual bool itemMatches(const Q3ListViewItem *item, const QString &s) const;

private:
    mutable QString lastpattern; // what was cached
    mutable QStringList splitted; // cache of the splitted string
};

#endif
